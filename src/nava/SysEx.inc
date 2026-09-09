//-------------------------------------------------
//                  NAVA v1.x
//            SysEx pattern dump / restore
//-------------------------------------------------
//
// v1.028beta had no way at all to get patterns off the unit - they live in the
// external 24LC1025, which the ISP programmer cannot reach, so a firmware
// update with a chip erase left them stranded. This adds a librarian protocol.
//
// Wire format
//   F0 7D 4E <cmd> [payload...] F7
//
//   7D is the MIDI "non-commercial / educational" manufacturer ID, reserved for
//   exactly this kind of private work, so it cannot collide with a real
//   manufacturer's SysEx. 4E ('N') identifies NAVA inside it.
//
// Host -> device
//   01                          dump every pattern, then the global setup
//   02 <ptrn>                   dump one pattern (0..127)
//   10 <ptrn> <page> <74 enc>   write one 64-byte EEPROM page of a pattern
//   11 00 00 <74 enc>           write the 64-byte global setup page
//
// Device -> host
//   10 / 11 in the same layout, then 7F to mark the end of a dump.
//
// One message carries exactly one 64-byte EEPROM page. That is deliberate: the
// 24LC1025 wraps writes within a page rather than spilling into the next one,
// so page-aligned messages make a restore incapable of straddling a boundary.
//
// Payload encoding: MIDI data bytes carry 7 bits, EEPROM bytes carry 8. Each
// group of up to 7 source bytes is sent as one byte holding their high bits
// followed by the 7-bit remainders, so 64 bytes become 74.
//
// Sizes follow PTRN_SIZE, so format v2 needed no protocol change: a full dump
// is 128 patterns x 14 pages = 1792 messages of 80 bytes, about 143 KB, which
// at the 31250 baud MIDI rate takes roughly 46 seconds.

#define SYSEX_ID            0x7D      // non-commercial / educational
#define SYSEX_DEV           0x4E      // 'N'
#define SYSEX_CMD_DUMP_ALL  0x01
#define SYSEX_CMD_DUMP_ONE  0x02
#define SYSEX_CMD_ID_REQ    0x03      // "what are you?"
#define SYSEX_CMD_PTRN      0x10
#define SYSEX_CMD_SETUP     0x11
#define SYSEX_CMD_TRK       0x12      // a track record, one page at a time
#define SYSEX_CMD_ID        0x13      // the answer to ID_REQ
#define SYSEX_CMD_END       0x7F
#define SYSEX_PTRN_PAGES    (byte)(PTRN_SIZE / MAX_PAGE_SIZE)   // v2: 896 / 64 = 14
#define SYSEX_TRK_PAGES     (byte)(TRACK_SIZE / MAX_PAGE_SIZE)  // v2: 512 / 64 = 8
#define SYSEX_ENC_MAX       74        // 64 raw bytes -> 74 encoded

/////////////////////Function//////////////////////

// [3B] Page I/O moved to EEprom.inc so there is one implementation of the
// device-select rule instead of two. A restore also gets ACK polling for free,
// which is worth ~2 ms on each of its 1792 page writes.
#define EEpromReadPage(adress, dst)   EepRead((adress),  (dst), (byte)MAX_PAGE_SIZE)
#define EEpromWritePage(adress, src)  EepWrite((adress), (src), (byte)MAX_PAGE_SIZE)

//8-bit -> 7-bit, returns the encoded length----------------------------------
byte SysExEncode(const byte *src, byte n, byte *dst)
{
  byte o = 0;
  byte i = 0;
  while (i < n){
    const byte g = ((byte)(n - i) < 7) ? (byte)(n - i) : 7;
    byte msb = 0;
    for (byte k = 0; k < g; k++) if (src[i + k] & 0x80) msb |= (1 << k);
    dst[o++] = msb;
    for (byte k = 0; k < g; k++) dst[o++] = src[i + k] & 0x7F;
    i += g;
  }
  return o;
}

//7-bit -> 8-bit, returns the decoded length----------------------------------
byte SysExDecode(const byte *src, byte n, byte *dst, byte maxOut)
{
  byte o = 0;
  byte i = 0;
  while (i < n && o < maxOut){
    const byte msb = src[i++];
    for (byte k = 0; k < 7 && i < n && o < maxOut; k++){
      byte v = src[i++] & 0x7F;
      if (msb & (1 << k)) v |= 0x80;
      dst[o++] = v;
    }
  }
  return o;
}

//Send one page as a SysEx message--------------------------------------------
void SysExSendPage(byte cmd, byte a, byte b, const byte *data)
{
  byte msg[5 + SYSEX_ENC_MAX];
  byte n = 0;
  msg[n++] = SYSEX_ID;
  msg[n++] = SYSEX_DEV;
  msg[n++] = cmd;
  msg[n++] = a;
  msg[n++] = b;
  n += SysExEncode(data, MAX_PAGE_SIZE, &msg[n]);
  MIDI.sendSysEx(n, msg, false);
}

//End-of-dump marker----------------------------------------------------------
void SysExSendEnd()
{
  byte msg[3];
  msg[0] = SYSEX_ID;
  msg[1] = SYSEX_DEV;
  msg[2] = SYSEX_CMD_END;
  MIDI.sendSysEx(3, msg, false);
}

//Dump one pattern------------------------------------------------------------
void SysExDumpPattern(byte n)
{
  if (n >= MAX_PTRN) return;
  byte page[MAX_PAGE_SIZE];
  for (byte p = 0; p < SYSEX_PTRN_PAGES; p++){
    EEpromReadPage((unsigned long)PTRN_OFFSET
                 + (unsigned long)n * PTRN_SIZE
                 + (unsigned long)p * MAX_PAGE_SIZE, page);
    SysExSendPage(SYSEX_CMD_PTRN, n, p, page);
  }
}

//Dump one track--------------------------------------------------------------
// Tracks were missing from the dump entirely, which made "back up everything" a
// promise the format did not keep: a restored machine came back with its
// patterns and none of the songs built out of them.
void SysExDumpTrack(byte n)
{
  if (n >= MAX_TRACK) return;
  byte page[MAX_PAGE_SIZE];
  for (byte p = 0; p < SYSEX_TRK_PAGES; p++){
    EEpromReadPage(TrackAddr(n) + (unsigned long)p * MAX_PAGE_SIZE, page);
    SysExSendPage(SYSEX_CMD_TRK, n, p, page);
  }
}

//Say what this is------------------------------------------------------------
// The handshake any host-side tool needs before it sends anything: which device,
// which record format, which firmware. A restore aimed at the wrong format is
// the one mistake that costs somebody their bank, and it is cheap to make
// impossible to walk into.
void SysExSendIdentity()
{
  byte msg[8 + sizeof(FW_VERSION)];
  byte n = 0;
  msg[n++] = SYSEX_ID;
  msg[n++] = SYSEX_DEV;
  msg[n++] = SYSEX_CMD_ID;
  msg[n++] = PTRN_FORMAT;                       // record format
  msg[n++] = (byte)MAX_PTRN;                    // patterns in a bank
  msg[n++] = (byte)MAX_TRACK;                   // tracks
  msg[n++] = (byte)(PTRN_SIZE >> 7);            // record size, 7-bit safe
  msg[n++] = (byte)(PTRN_SIZE & 0x7F);
  for (byte i = 0; i < sizeof(FW_VERSION) - 1; i++) msg[n++] = (byte)(FW_VERSION[i] & 0x7F);
  MIDI.sendSysEx(n, msg, false);
}

//Dump the global setup page--------------------------------------------------
void SysExDumpSetup()
{
  byte page[MAX_PAGE_SIZE];
  EEpromReadPage(OFFSET_SETUP, page);
  SysExSendPage(SYSEX_CMD_SETUP, 0, 0, page);
}

//Dump everything-------------------------------------------------------------
// Blocks the main loop for ~46 s. The sequencer keeps running - it lives in the
// Timer1 ISR - but the panel does not respond until this returns.
void SysExDumpAll()
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(F(" SysEx dump...  "));
  lcd.setCursor(0,1);
  lcd.print(F("  do not power  "));
  LcdFlush();                                     // [3A] must reach the panel before we block

  for (unsigned int n = 0; n < MAX_PTRN; n++){
    SysExDumpPattern((byte)n);
    SetDoutLed((unsigned int)1 << (n / 8), 0, 0);   // progress across the 16 step LEDs
  }
  for (byte n = 0; n < MAX_TRACK; n++){
    SysExDumpTrack(n);
    SetDoutLed((unsigned int)1 << (n % 16), 0, 0);
  }
  SysExDumpSetup();
  SysExSendEnd();

  SetDoutLed(0, 0, 0);
  needLcdUpdate = TRUE;
}

//Handle an incoming SysEx message--------------------------------------------
// Called from MIDI.read() in the main loop. System messages bypass the channel
// filter in this library version, so this works whatever RX channel is set.
void HandleSystemExclusive(byte *array, byte size)
{
  byte i = 0;
  if (size && array[0] == 0xF0) i = 1;          // the library includes the status byte
  if (size < (byte)(i + 3)) return;
  if (array[i] != SYSEX_ID || array[i + 1] != SYSEX_DEV) return;

  const byte  cmd  = array[i + 2];
  const byte *p    = &array[i + 3];
  byte        plen = (byte)(size - i - 3);
  if (plen && p[plen - 1] == 0xF7) plen--;      // trim EOX if the library kept it

  byte page[MAX_PAGE_SIZE];

  switch (cmd){
  case SYSEX_CMD_DUMP_ALL:
    SysExDumpAll();
    break;

  case SYSEX_CMD_DUMP_ONE:
    if (plen >= 1){
      SysExDumpPattern(p[0]);
      SysExSendEnd();
    }
    break;

  case SYSEX_CMD_ID_REQ:
    SysExSendIdentity();
    break;

  case SYSEX_CMD_PTRN:
    // p[0] = pattern, p[1] = page, rest = encoded page
    if (plen >= 3 && p[0] < MAX_PTRN && p[1] < SYSEX_PTRN_PAGES){
      if (SysExDecode(&p[2], (byte)(plen - 2), page, MAX_PAGE_SIZE) == MAX_PAGE_SIZE){
        EEpromWritePage((unsigned long)PTRN_OFFSET
                      + (unsigned long)p[0] * PTRN_SIZE
                      + (unsigned long)p[1] * MAX_PAGE_SIZE, page);
        patternRestored = TRUE;
        timeSinceRestore = millis();
      }
    }
    break;

  case SYSEX_CMD_TRK:
    // p[0] = track, p[1] = page, rest = encoded page
    if (plen >= 3 && p[0] < MAX_TRACK && p[1] < SYSEX_TRK_PAGES){
      if (SysExDecode(&p[2], (byte)(plen - 2), page, MAX_PAGE_SIZE) == MAX_PAGE_SIZE){
        EEpromWritePage(TrackAddr(p[0]) + (unsigned long)p[1] * MAX_PAGE_SIZE, page);
        trackRestored   = TRUE;
        patternRestored = TRUE;         // the same quiet-then-reload path
        timeSinceRestore = millis();
      }
    }
    break;

  case SYSEX_CMD_SETUP:
    if (plen >= 3){
      if (SysExDecode(&p[2], (byte)(plen - 2), page, MAX_PAGE_SIZE) == MAX_PAGE_SIZE){
        EEpromWritePage(OFFSET_SETUP, page);
        patternRestored = TRUE;
        timeSinceRestore = millis();
      }
    }
    break;
  }
}

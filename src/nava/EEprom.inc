//-------------------------------------------------
//                  NAVA v2.x
//               EEprom 1024K  -  format v2
//
// Memory map
//   0        .. 114687   patterns      128 x 896   (14 pages each)
//   114688   .. 122879   tracks         16 x 512   (8 pages each)
//   122880   .. 122943   bank header               (1 page)
//   122944   .. 123007   global setup              (1 page)
//   123008   .. 131071   free / scratch            (8,064 B)
//
// Pattern 73 onward lives above the 65536 device boundary. Harmless: every
// record and every page is 64-byte aligned at offset 0 and 65536 is a multiple
// of 64, so no single page straddles it, and EepDevice() picks the right half
// per access. That was [B-27] - v1 hardcoded the lower device in four places
// and only got away with it because patterns ended at 57344.
//
// The struct is the record: see struct Pattern in define.h. Load and save are
// page copies, so there is no serialiser to fall out of step with the layout.
//
// Speed, against v1 at 100 kHz with blind 5 ms write delays:
//   pattern load  ~42 ms -> ~24 ms      pattern save  ~80 ms -> ~66 ms
//   track load   ~100 ms -> ~2 ms typical (one page for a song under 48 long)
// Twice the record, and still faster in every path, because 3B-1 raised the bus
// to 400 kHz and replaced the write delays with ACK polling first. That is also
// why there is no dirty-page tracking here: it would be optimising a path that
// already beats what it replaced.
//-------------------------------------------------

#define MAX_PAGE_SIZE      (unsigned long)(64)
#define HRDW_ADDRESS       0x50
#define HRDW_ADDRESS_UP    (0x50 | B100)
#define DELAY_WR           5

#define PTRN_SIZE          (unsigned long)(896)
#define PTRN_OFFSET        (unsigned long)(0)
#define PTRN_PAGES         (byte)(PTRN_SIZE / MAX_PAGE_SIZE)          // 14
#define TRACK_SIZE         (unsigned long)(512)
#define TRACK_PAGES        (byte)(TRACK_SIZE / MAX_PAGE_SIZE)         // 8
#define TRACK_OFFSET       (PTRN_SIZE * MAX_PTRN)                     // 114688
#define BANK_OFFSET        (TRACK_OFFSET + (TRACK_SIZE * MAX_TRACK))  // 122880
#define OFFSET_SETUP       (BANK_OFFSET + MAX_PAGE_SIZE)              // 122944
#define SETUP_SIZE         (unsigned long)(64)
#define EEP_FREE_OFFSET    (OFFSET_SETUP + SETUP_SIZE)                // 123008

// groupPos / groupLength inside a v2 record - see struct Pattern
#define OFFSET_GROUP       (unsigned long)(23)
#define GROUP_SIZE         (unsigned long)(2)

// Bank header. Makes the chip self-describing, so the firmware can tell a v2
// bank from a v1 one or from a blank chip instead of playing whatever it finds.
#define BANK_MAGIC0        'N'
#define BANK_MAGIC1        'V'
#define BANK_MAGIC2        'B'
#define BANK_MAGIC3        'K'
#define BANK_STATE_OK      0x00
#define BANK_STATE_MIGRATE 0x4D   // 'M': a v1 -> v2 conversion was interrupted
#define BANK_STATE_UTIL    0x55   // 'U': a range clear or move was interrupted
#define MIGRATE_FROM_START 0xFFFF // convert setup and tracks too, not just a resume

// v1 geometry, needed only by the migration below.
#define V1_PTRN_SIZE       (unsigned long)(448)
#define V1_TRACK_SIZE      (unsigned long)(1024)
#define V1_TRACK_OFFSET    (V1_PTRN_SIZE * MAX_PTRN)                    // 57344
#define V1_SETUP_OFFSET    (V1_TRACK_OFFSET + (V1_TRACK_SIZE * MAX_TRACK))  // 73728
#define V1_EXT_OFFSET      (unsigned long)(64)
#define V1_VEL_OFFSET      (unsigned long)(192)

//===========================================================================
// [3B] Page-level I/O.
//
// Every access picks its device from its own address. Transfers are capped at
// 64 bytes and callers keep to page boundaries: WireN's BUFFER_LENGTH is 66 =
// 2 address + 64 data with no headroom, and a 24LC1025 write wraps within its
// page rather than carrying into the next.
//===========================================================================

// The 24LC1025 is two 512 Kbit halves; A16 is a block-select bit in the
// control byte, not part of the 16-bit word address.
static inline byte EepDevice(unsigned long addr)
{
  return (addr > 65535UL) ? HRDW_ADDRESS_UP : HRDW_ADDRESS;
}

static void EepAddr(unsigned long addr)
{
  Wire.beginTransmission(EepDevice(addr));
  Wire.write((byte)((addr >> 8) & 0xFF));
  Wire.write((byte)(addr & 0xFF));
}

// Wait out the internal write cycle by ACK polling: the device NAKs its own
// address until the write finishes. Typically ~3 ms against the datasheet's
// 5 ms maximum, so this is faster than a blind delay and still correct on a
// slow part. Bounded, so a dead chip cannot hang the panel.
static void EepWaitReady(unsigned long addr)
{
  const byte dev = EepDevice(addr);
  for (unsigned int tries = 0; tries < 400; tries++){
    Wire.beginTransmission(dev);
    if (Wire.endTransmission() == 0) return;
  }
}

void EepRead(unsigned long addr, byte *dst, byte n)
{
  EepAddr(addr);
  Wire.endTransmission();
  Wire.requestFrom(EepDevice(addr), n);
  for (byte i = 0; i < n; i++) dst[i] = (byte)Wire.read();
}

void EepWrite(unsigned long addr, const byte *src, byte n)
{
  EepAddr(addr);
  for (byte i = 0; i < n; i++) Wire.write(src[i]);
  Wire.endTransmission();
  EepWaitReady(addr);
}

////////////////////////Function//////////////////////

static inline unsigned long PatternAddr(byte n) { return PTRN_OFFSET + (unsigned long)n * PTRN_SIZE; }
static inline unsigned long TrackAddr(byte n)   { return TRACK_OFFSET + (unsigned long)n * TRACK_SIZE; }

//save pattern
void SavePattern(byte patternNbr)
{
  if (patternNbr >= MAX_PTRN) return;
  const byte *src = (const byte *)&pattern[ptrnBuffer];
  const unsigned long base = PatternAddr(patternNbr);

  for (byte p = 0; p < PTRN_PAGES; p++){
    MIDI.read();                       // a save is ~66 ms; do not stall incoming MIDI
    EepWrite(base + (unsigned long)p * MAX_PAGE_SIZE,
             src + ((unsigned int)p * MAX_PAGE_SIZE), (byte)MAX_PAGE_SIZE);
  }
}

//Load Pattern
void LoadPattern(byte patternNbr)
{
  if (patternNbr >= MAX_PTRN) return;
  byte *dst = (byte *)&pattern[!ptrnBuffer];
  const unsigned long base = PatternAddr(patternNbr);

  for (byte p = 0; p < PTRN_PAGES; p++){
    MIDI.read();
    EepRead(base + (unsigned long)p * MAX_PAGE_SIZE,
            dst + ((unsigned int)p * MAX_PAGE_SIZE), (byte)MAX_PAGE_SIZE);
  }

  // Refuse to play a record we do not recognise. Without this a v1 bank, or a
  // chip that lost a page, would be read as though its bytes meant what v2
  // means by them - which is not silence, it is noise.
  if (!PatternRecordValid(!ptrnBuffer)){                          // [4T]
    InitPatternRecord(!ptrnBuffer, patternNbr);
    bankFormatBad = TRUE;
  }

  // [6I] The header says the record is ours. It does not say the bytes inside it
  // are in range, and these two are the ones the ISR divides and compares by
  // every tick - so an out-of-range value is not a wrong setting, it is a
  // sequencer running at a rate no gesture on the panel can name or undo.
  //
  // The same argument as PatternRecordValid() above, one level down: refuse to
  // play bytes that cannot mean what they say. Cheap enough to do on every load,
  // and the alternative is trusting 896 bytes of external I2C EEprom because
  // three of them looked right.
  if (pattern[!ptrnBuffer].length >= NBR_STEP) pattern[!ptrnBuffer].length = NBR_STEP - 1;
  {
    const byte s = pattern[!ptrnBuffer].scale;
    if (s != SCALE_16 && s != SCALE_32 && s != SCALE_8t && s != SCALE_16t)
      pattern[!ptrnBuffer].scale = SCALE_16;
  }

  // [4Z] Chains are not stored any more. Zeroing the two bytes as the record
  // comes in means nothing downstream can read them as a chain, and the next
  // save writes them out clear - so a bank carrying groups from an older build
  // cleans itself up as it is used rather than needing a pass over all 128.
  pattern[!ptrnBuffer].groupPos    = 0;
  pattern[!ptrnBuffer].groupLength = 0;

  prevShuf = pattern[!ptrnBuffer].shuffle;                        // [zabox] [1.027] flam
  prevFlam = pattern[!ptrnBuffer].flam;                           //
}

//Track save
// [3B] Writes the header page plus only as many pages as the track's length
// actually reaches. A song under 48 patterns is one page.
void SaveTrack(byte trackNbr)
{
  if (trackNbr >= MAX_TRACK) return;
  const byte *src = (const byte *)&track[trkBuffer];
  const unsigned long base = TrackAddr(trackNbr);

  unsigned int used = track[trkBuffer].length + 1;
  if (used > MAX_PTRN_TRACK) used = MAX_PTRN_TRACK;
  byte pages = (byte)((16 + used + MAX_PAGE_SIZE - 1) / MAX_PAGE_SIZE);
  if (pages < 1) pages = 1;
  if (pages > TRACK_PAGES) pages = TRACK_PAGES;

  for (byte p = 0; p < pages; p++){
    MIDI.read();
    EepWrite(base + (unsigned long)p * MAX_PAGE_SIZE,
             src + ((unsigned int)p * MAX_PAGE_SIZE), (byte)MAX_PAGE_SIZE);
  }
}

//Load track
void LoadTrack(byte trackNbr)
{
  if (trackNbr >= MAX_TRACK) return;
  byte *dst = (byte *)&track[trkBuffer];
  const unsigned long base = TrackAddr(trackNbr);

  // Page 0 carries the length, which is the whole point of moving it out of
  // bytes 1022-1023: v1 had to read all sixteen pages to discover it.
  EepRead(base, dst, (byte)MAX_PAGE_SIZE);

  if (track[trkBuffer].formatVer != PTRN_FORMAT || track[trkBuffer].length > MAX_PTRN_TRACK){
    for (unsigned int i = 0; i < TRACK_SIZE; i++) dst[i] = 0;
    track[trkBuffer].formatVer = PTRN_FORMAT;
    track[trkBuffer].length = 0;
    for (byte i = 0; i < 12; i++) track[trkBuffer].name[i] = ' ';
    return;
  }

  unsigned int used  = track[trkBuffer].length + 1;
  byte         pages = (byte)((16 + used + MAX_PAGE_SIZE - 1) / MAX_PAGE_SIZE);
  if (pages > TRACK_PAGES) pages = TRACK_PAGES;

  for (byte p = 1; p < pages; p++){
    EepRead(base + (unsigned long)p * MAX_PAGE_SIZE,
            dst + ((unsigned int)p * MAX_PAGE_SIZE), (byte)MAX_PAGE_SIZE);
  }
}

//Save Setup
void SaveSeqSetup()
{
  byte page[MAX_PAGE_SIZE];
  for (byte i = 0; i < MAX_PAGE_SIZE; i++) page[i] = 0;
  page[0] = (byte)seq.sync;
  page[1] = (byte)seq.defaultBpm;
  page[2] = (byte)seq.TXchannel;
  page[3] = (byte)seq.RXchannel;
  page[4] = (byte)seq.ptrnChangeSync;
  page[5] = (byte)seq.muteModeHH;                                      // [zabox]
  page[6] = (byte)seq.hhPriority;                                      // [3B]
  page[7] = (byte)seq.pasteMask;                                       // [F29]
  page[8] = (byte)seq.singleTrig;                                      // [F7]
  page[9] = (byte)seq.bpmFine;                                         // [F30]
  page[14] = (byte)seq.tapOverMute;                                    // [4B]
  page[19] = (byte)(seq.ohSelect + 1);                                 // [4W]
  page[20] = (byte)seq.cymAccent;                                      // [6Z]
  EepWrite(OFFSET_SETUP, page, (byte)MAX_PAGE_SIZE);
}

//Load Setup
void LoadSeqSetup()
{
  byte page[MAX_PAGE_SIZE];
  EepRead(OFFSET_SETUP, page, (byte)MAX_PAGE_SIZE);

  // Every field is constrained, so an uninitialised or corrupt setup page
  // degrades to sane defaults instead of an unusable machine.
  seq.sync = constrain(page[0], 0, 2);                                 // [zabox] [1.028] + expander mode
  seq.defaultBpm = constrain((unsigned int)page[1], (unsigned int)MIN_BPM, (unsigned int)MAX_BPM);
  // [5I] Up to 17, which is OFF. A blank byte reads 0 and still constrains up to
  // channel 1, so nothing about the stored format changes and no upgrade check
  // is needed.
  seq.TXchannel = constrain(page[2], 1, MIDI_CHAN_OFF);
  seq.RXchannel = constrain(page[3], 1, MIDI_CHAN_OFF);
  seq.ptrnChangeSync = constrain(page[4], 0, 1);
  seq.tapOverMute  = constrain(page[14], 0, 1);
  // [6Z] Zero on a blank chip is OFF, which is the behaviour the machine has
  // always had - so this one needs no plus-one dance.
  seq.cymAccent    = constrain(page[20], 0, 1);
  // [4W] Plus one, so an unwritten byte still means "never set". The default is
  // one pad per voice: it is what the other ten voices already do, and a panel
  // where one entry in eleven needs a second finger is a rule with an exception
  // in it. Anyone who prefers the chord has a switch.
  seq.ohSelect = page[19] ? (byte)constrain(page[19] - 1, 0, 1) : OH_SEL_2ND;

  seq.muteModeHH = constrain(page[5], 0, 1);                           // [zabox]
  seq.hhPriority = constrain(page[6], 0, 1);                           // [3B]
  // [F29] A blank or pre-F29 setup page reads 0 here, which would mean "paste
  // nothing" - a silently useless paste. Treat 0 as "not set yet".
  seq.pasteMask = page[7] ? page[7] : PASTE_DEFAULT;
  // [F30] A setup page written before this existed reads 0 here, which is
  // exactly "no tenths" - the old behaviour, so no migration is needed.
  seq.bpmFine = constrain(page[9], 0, 9);
  seq.singleTrig = constrain(page[8], 0, 1);                           // [F7]
}

// [4Z] SavePatternGroup(), ClearPatternGroup() and LoadPatternGroup() removed.
// Chains are no longer stored, so the two bytes they wrote have no reader - and
// helpers that only write are worse than none, because the next person to find
// them assumes something downstream cares.
//
// OFFSET_GROUP and GROUP_SIZE stay: the bytes are still part of the record,
// still zeroed by InitPatternRecord() and by the utilities page, and the format
// has to keep the same shape as stock. They are simply always zero now.


//==================================================================================================
// Bank header
//==================================================================================================

void BankWriteHeader(byte state, unsigned int index)
{
  byte page[MAX_PAGE_SIZE];
  for (byte i = 0; i < MAX_PAGE_SIZE; i++) page[i] = 0;
  page[0] = BANK_MAGIC0;
  page[1] = BANK_MAGIC1;
  page[2] = BANK_MAGIC2;
  page[3] = BANK_MAGIC3;
  page[4] = PTRN_FORMAT;
  page[5] = MAX_PTRN;
  page[6] = MAX_TRACK;
  page[7] = state;
  page[8] = (byte)(index >> 8);
  page[9] = (byte)(index & 0xFF);
  for (byte i = 0; i < 12; i++) page[10 + i] = ' ';       // bank name, F26
  EepWrite(BANK_OFFSET, page, (byte)MAX_PAGE_SIZE);
}

// Does this pattern record carry a v2 header? Used to tell "the bank header is
// missing" from "this is not a v2 bank", which are very different situations.
boolean PatternLooksV2(byte n)
{
  byte head[4];
  EepRead(PatternAddr(n), head, (byte)4);
  return (head[0] == PTRN_MAGIC0 && head[1] == PTRN_MAGIC1 && head[2] == PTRN_FORMAT);
}

// 0 = not a v2 bank, 1 = good v2 bank, 2 = interrupted migration (index out)
byte BankCheck(unsigned int *resumeIndex)
{
  byte page[MAX_PAGE_SIZE];
  EepRead(BANK_OFFSET, page, (byte)MAX_PAGE_SIZE);

  if (page[0] != BANK_MAGIC0 || page[1] != BANK_MAGIC1 ||
      page[2] != BANK_MAGIC2 || page[3] != BANK_MAGIC3 ||
      page[4] != PTRN_FORMAT) return 0;

  if (page[7] == BANK_STATE_MIGRATE){
    if (resumeIndex) *resumeIndex = ((unsigned int)page[8] << 8) | page[9];
    return 2;
  }
  if (page[7] == BANK_STATE_UTIL) return 3;   // [F25] interrupted range op
  return 1;
}

//==================================================================================================
// v1 -> v2 migration, on the unit
//==================================================================================================
//
// The PC librarian can do this too, and for anyone with a MIDI interface that
// is the safer route because it never writes until a verified bank is pushed
// back. This exists because the conversion is possible in place, and needing a
// MIDI interface to keep your own patterns is a poor trade when you are
// nowhere near one.
//
// In place is safe here for a specific reason. A v2 record is exactly twice a
// v1 record, so writing pattern n at n*896 cannot reach v1 pattern n at n*448
// for any n >= 1 - and pattern 0 is read into RAM before its own record is
// overwritten. Converting downward from 127 therefore never destroys a source
// it has not already consumed. It is the memmove argument, and it is the same
// one that will make F25's range moves safe.
//
// Tracks go first for the same reason in reverse: v1 tracks live at 57344,
// which is inside the region v2 patterns will eventually occupy, while v2
// tracks live at 114688, above everything v1 ever used.
//
// The header is written last, with an in-progress marker and the next index,
// so an interrupted conversion is detectable and resumable rather than a bank
// half in each format with no way to tell which half.

static void MigrateOnePattern(byte n)
{
  byte page[MAX_PAGE_SIZE];
  const unsigned long src = (unsigned long)n * V1_PTRN_SIZE;
  Pattern *p = &pattern[!ptrnBuffer];

  // ---- v1 page 0: 32 B trig, 8 B setup, 24 B unused
  EepRead(src, page, (byte)MAX_PAGE_SIZE);

  unsigned int inst[NBR_INST];
  for (byte i = 0; i < NBR_INST; i++)
    inst[i] = (unsigned int)page[i * 2] | ((unsigned int)page[i * 2 + 1] << 8);

  const byte v1Length      = page[32];
  const byte v1Scale       = page[33];
  const byte v1Shuffle     = page[34];
  const byte v1Flam        = page[35];
  const byte v1ExtLength   = page[36];
  const byte v1GroupPos    = page[37];
  const byte v1GroupLength = page[38];
  const byte v1TotalAcc    = page[39];

  // ---- wipe, then fill. Everything v2 added starts at zero by construction.
  for (unsigned int i = 0; i < PTRN_SIZE; i++) ((byte *)p)[i] = 0;

  p->magic0 = PTRN_MAGIC0;
  p->magic1 = PTRN_MAGIC1;
  p->formatVer = PTRN_FORMAT;
  p->flags = 0;

  for (byte i = 0; i < 12; i++) p->name[i] = ' ';
  p->name[0] = 'P';                                   // seed P001..P128, F26
  p->name[1] = (char)('0' + ((n + 1) / 100) % 10);
  p->name[2] = (char)('0' + ((n + 1) / 10) % 10);
  p->name[3] = (char)('0' + (n + 1) % 10);

  for (byte i = 0; i < NBR_INST; i++) p->inst[i] = inst[i];
  p->length      = v1Length;
  p->scale       = v1Scale;
  p->shuffle     = v1Shuffle;
  p->flam        = v1Flam;
  p->extLength   = v1ExtLength;
  p->groupPos    = v1GroupPos;
  p->groupLength = v1GroupLength;
  p->totalAcc    = v1TotalAcc;
  p->dir         = FORWARD;

  // ---- v1 ext notes, 2 pages at +64
  for (byte q = 0; q < 2; q++){
    EepRead(src + V1_EXT_OFFSET + (unsigned long)q * MAX_PAGE_SIZE, page, (byte)MAX_PAGE_SIZE);
    for (byte i = 0; i < MAX_PAGE_SIZE; i++) p->extNote[q * MAX_PAGE_SIZE + i] = page[i];
  }

  // ---- v1 velocity, 4 pages at +192, four instruments per page.
  // This is where bit 7 stops being an overload: v1 packed the flam flag into
  // the velocity byte, so "velocity" meant a 0..127 level and a boolean at the
  // same time. Split here, once, and the two never share a byte again.
  for (byte q = 0; q < 4; q++){
    EepRead(src + V1_VEL_OFFSET + (unsigned long)q * MAX_PAGE_SIZE, page, (byte)MAX_PAGE_SIZE);
    for (byte i = 0; i < 4; i++){
      const byte instIdx = i + 4 * q;
      for (byte j = 0; j < NBR_STEP; j++){
        const byte v = page[i * NBR_STEP + j];
        p->velocity[instIdx][j] = v & 127;
        if ((v & 128) && InstHasFlam(instIdx)) bitSet(p->flamMask[instIdx], j);
      }
    }
  }

  // ---- write the v2 record. Safe because n*896 >= (n+1)*448 for n >= 1, and
  // pattern 0's source is already in RAM by the time we get here.
  const byte *out = (const byte *)p;
  const unsigned long dst = PatternAddr(n);
  for (byte q = 0; q < PTRN_PAGES; q++){
    EepWrite(dst + (unsigned long)q * MAX_PAGE_SIZE,
             out + ((unsigned int)q * MAX_PAGE_SIZE), (byte)MAX_PAGE_SIZE);
  }
}

static void MigrateOneTrack(byte t)
{
  byte page[MAX_PAGE_SIZE];
  const unsigned long src = V1_TRACK_OFFSET + (unsigned long)t * V1_TRACK_SIZE;
  Track *tr = &track[trkBuffer];

  for (unsigned int i = 0; i < TRACK_SIZE; i++) ((byte *)tr)[i] = 0;
  for (byte i = 0; i < 12; i++) tr->name[i] = ' ';
  tr->formatVer = PTRN_FORMAT;
  tr->flags = 0;

  // v1 kept the length in the last two bytes of the last page.
  EepRead(src + V1_TRACK_SIZE - MAX_PAGE_SIZE, page, (byte)MAX_PAGE_SIZE);
  unsigned int len = (unsigned int)page[62] | ((unsigned int)page[63] << 8);
  if (len >= MAX_PTRN_TRACK) len = MAX_PTRN_TRACK - 1;   // v1 held up to 1022; v2 holds 496
  tr->length = len;

  for (unsigned int i = 0; i <= len; i += MAX_PAGE_SIZE){
    EepRead(src + i, page, (byte)MAX_PAGE_SIZE);
    for (byte k = 0; k < MAX_PAGE_SIZE && (i + k) < MAX_PTRN_TRACK; k++){
      byte v = page[k];
      tr->patternNbr[i + k] = (v < MAX_PTRN) ? v : 0;
    }
  }

  const byte *out = (const byte *)tr;
  const unsigned long dst = TrackAddr(t);
  for (byte q = 0; q < TRACK_PAGES; q++){
    EepWrite(dst + (unsigned long)q * MAX_PAGE_SIZE,
             out + ((unsigned int)q * MAX_PAGE_SIZE), (byte)MAX_PAGE_SIZE);
  }
}

void MigrateV1toV2(unsigned int resumeFrom)
{
  // Setup first: tiny, and it lives above everything either format touches.
  if (resumeFrom > MAX_PTRN){
    byte page[MAX_PAGE_SIZE];
    EepRead(V1_SETUP_OFFSET, page, (byte)MAX_PAGE_SIZE);
    seq.sync           = constrain(page[0], 0, 2);
    seq.defaultBpm     = constrain((unsigned int)page[1], (unsigned int)MIN_BPM, (unsigned int)MAX_BPM);
    seq.TXchannel      = constrain(page[2], 1, MIDI_CHAN_OFF);            // [5I]
    seq.RXchannel      = constrain(page[3], 1, MIDI_CHAN_OFF);
    seq.ptrnChangeSync = constrain(page[4], 0, 1);
    seq.muteModeHH     = constrain(page[5], 0, 1);
    seq.hhPriority     = HH_PRIO_OH;                 // v1 had no such setting
    seq.pasteMask      = PASTE_DEFAULT;
    seq.singleTrig     = FALSE;
    seq.bpmFine        = 0;                         // [F30] v1 had no tenths
    SaveSeqSetup();

    // Tracks next: v1 tracks sit at 57344, inside the region v2 patterns will
    // occupy, while v2 tracks sit at 114688, above anything v1 ever used.
    for (byte t = 0; t < MAX_TRACK; t++){
      MigrateOneTrack(t);
      SetDoutLed((unsigned int)1 << t, 0, 0);
    }
    resumeFrom = MAX_PTRN - 1;
  }

  // Patterns, downward. See the note above for why this cannot eat its own
  // source. The marker is refreshed as we go so an interrupted run resumes.
  for (int n = (int)resumeFrom; n >= 0; n--){
    BankWriteHeader(BANK_STATE_MIGRATE, (unsigned int)n);
    MigrateOnePattern((byte)n);
    SetDoutLed((unsigned int)1 << (n / 8), 0, 0);
  }

  BankWriteHeader(BANK_STATE_OK, 0);
  SetDoutLed(0, 0, 0);
}

//==================================================================================================
//==================================================================================================
//init pattern in the EEprom
void InitEEprom()
{
  // Build one default record in RAM, then stamp it out. v1 hand-wrote every
  // page here, which is how [B-19] happened - the 24 spare bytes of each
  // pattern's first page had their write commented out and were left holding
  // whatever the chip did, which is exactly where new fields would have gone.
  InitPatternRecord(!ptrnBuffer, 0);
  const byte *src = (const byte *)&pattern[!ptrnBuffer];

  for (byte n = 0; n < MAX_PTRN; n++){
    pattern[!ptrnBuffer].name[1] = (char)('0' + ((n + 1) / 100) % 10);
    pattern[!ptrnBuffer].name[2] = (char)('0' + ((n + 1) / 10) % 10);
    pattern[!ptrnBuffer].name[3] = (char)('0' + (n + 1) % 10);

    const unsigned long base = PatternAddr(n);
    for (byte p = 0; p < PTRN_PAGES; p++){
      EepWrite(base + (unsigned long)p * MAX_PAGE_SIZE,
               src + ((unsigned int)p * MAX_PAGE_SIZE), (byte)MAX_PAGE_SIZE);
    }
    SetDoutLed((unsigned int)1 << (n / 8), 0, 0);
  }

  //Track Init
  for (unsigned int i = 0; i < TRACK_SIZE; i++) ((byte *)&track[trkBuffer])[i] = 0;
  for (byte i = 0; i < 12; i++) track[trkBuffer].name[i] = ' ';
  track[trkBuffer].formatVer = PTRN_FORMAT;
  track[trkBuffer].length = 0;
  const byte *trkSrc = (const byte *)&track[trkBuffer];

  for (byte t = 0; t < MAX_TRACK; t++){
    const unsigned long base = TrackAddr(t);
    for (byte p = 0; p < TRACK_PAGES; p++){
      EepWrite(base + (unsigned long)p * MAX_PAGE_SIZE,
               trkSrc + ((unsigned int)p * MAX_PAGE_SIZE), (byte)MAX_PAGE_SIZE);
    }
    SetDoutLed((unsigned int)1 << t, 0, 0);
  }

  //Setup init
  seq.sync = MASTER;
  seq.defaultBpm = DEFAULT_BPM;
  seq.TXchannel = 1;
  seq.RXchannel = 1;
  seq.ptrnChangeSync = FREE;
  seq.muteModeHH = 0;
  seq.hhPriority = HH_PRIO_OH;
  seq.pasteMask = PASTE_DEFAULT;
  seq.singleTrig = FALSE;
  SaveSeqSetup();

  BankWriteHeader(BANK_STATE_OK, 0);
  SetDoutLed(0, 0, 0);
}

//wire begin
void WireBeginTX(unsigned long address)
{
  EepAddr(address);
}

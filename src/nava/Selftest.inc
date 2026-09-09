//-------------------------------------------------
//                  NAVA v2.x
//              EEprom self-test  [3B]
//
// Verifies the I2C bus and the 24LC1025 with no PC, no MIDI interface and no
// scope. Held STOP + ENTER at power-on.
//
// This exists because 3B raises the bus to 400 kHz. The arithmetic says there
// is ~3x margin (4.7k pull-ups, ~24 pF of load, 300 ns fast-mode limit), but
// marginal I2C timing does not fail loudly - it corrupts an occasional byte,
// which surfaces months later as a pattern that quietly plays wrong. A test
// that fails deterministically is worth more than one that fails rarely.
//
// Two passes, chosen so neither can lose work:
//
//   1. Write/verify, confined to the 8 KB scratch region at the top of the
//      chip that is free under both the v1 and v2 maps. Four patterns per
//      page - 0x00, 0xFF, walking-one, and address-seeded pseudo-random -
//      because a marginal bus fails on data-dependent edges and on long runs
//      of one level, not on any single value.
//   2. Read consistency across all 2048 pages of both devices. Each page is
//      read twice and the two reads compared. This reaches the whole address
//      space, including the upper device and every live pattern, without
//      writing a single byte.
//
// Pass 1 proves writes. Pass 2 proves the address space. Marginal timing is
// noise-driven, so it shows as a disagreement between two reads of the same
// page long before it shows as a pattern that sounds wrong.
//
// It is also a plain hardware diagnostic - a tired socket or a cracked joint
// on IC115 looks exactly the same - so it earns its flash regardless of what
// the bus is clocked at.
//-------------------------------------------------

// Free under the v1 map (which ends at 73792) and under the v2 map (which ends
// at 123008), so this region is safe to hammer either side of the format
// change. The move journal lives in the bank header page at 122880, below this.
#define EEP_SCRATCH_ADDR   (unsigned long)(123008)
#define EEP_SCRATCH_PAGES  126
#define EEP_TOTAL_PAGES    2048

////////////////////////Function//////////////////////

static byte SelfTestByte(byte which, unsigned long addr, byte i)
{
  switch (which){
  case 0:  return 0x00;                    // long low run
  case 1:  return 0xFF;                    // long high run, worst case for rise time
  case 2:  return (byte)(1 << (i & 7));    // walking one
  default: {                               // decorrelated, and different on every page
      unsigned int s = (unsigned int)(addr >> 6) ^ (unsigned int)(i * 2654U);
      s ^= s << 7; s ^= s >> 9; s ^= s << 8;
      return (byte)s;
    }
  }
}

static void SelfTestPrintAddr(unsigned long v)
{
  for (char sh = 16; sh >= 0; sh -= 4){
    const byte n = (byte)((v >> sh) & 0x0F);
    lcd.print((char)(n < 10 ? ('0' + n) : ('A' + n - 10)));
  }
}

void EepromSelfTest()
{
  byte wr[MAX_PAGE_SIZE];
  byte rd[MAX_PAGE_SIZE];
  unsigned long failAddr = 0;
  boolean ok = TRUE;

  lcd.clear();
  lcd.setCursor(0, 0); lcd.print(F("EEprom test 1/2 "));
  lcd.setCursor(0, 1); lcd.print(F("do not power off"));
  LcdFlush();

  //Pass 1 - write and verify, scratch region only-----------------
  for (unsigned int p = 0; p < EEP_SCRATCH_PAGES && ok; p++){
    const unsigned long addr = EEP_SCRATCH_ADDR + (unsigned long)p * MAX_PAGE_SIZE;

    for (byte k = 0; k < 4 && ok; k++){
      for (byte i = 0; i < MAX_PAGE_SIZE; i++) wr[i] = SelfTestByte(k, addr, i);
      EepWrite(addr, wr, (byte)MAX_PAGE_SIZE);
      EepRead (addr, rd, (byte)MAX_PAGE_SIZE);

      for (byte i = 0; i < MAX_PAGE_SIZE; i++){
        if (rd[i] != wr[i]){ ok = FALSE; failAddr = addr + i; break; }
      }
    }
    SetDoutLed((unsigned int)1 << ((p * 16) / EEP_SCRATCH_PAGES), 0, 0);
  }

  //Pass 2 - read consistency, whole chip, no writes---------------
  if (ok){
    lcd.setCursor(0, 0); lcd.print(F("EEprom test 2/2 "));
    LcdFlush();

    for (unsigned int p = 0; p < EEP_TOTAL_PAGES && ok; p++){
      const unsigned long addr = (unsigned long)p * MAX_PAGE_SIZE;
      EepRead(addr, wr, (byte)MAX_PAGE_SIZE);
      EepRead(addr, rd, (byte)MAX_PAGE_SIZE);

      for (byte i = 0; i < MAX_PAGE_SIZE; i++){
        if (rd[i] != wr[i]){ ok = FALSE; failAddr = addr + i; break; }
      }
      if ((p & 127) == 0) SetDoutLed((unsigned int)1 << (p / 128), 0, 0);
    }
  }

  //Report---------------------------------------------------------
  SetDoutLed(0, 0, 0);
  lcd.clear();

  if (ok){
    lcd.setCursor(0, 0); lcd.print(F("EEprom test PASS"));
    lcd.setCursor(0, 1); lcd.print(F(" I2C 400kHz OK  "));
    LcdFlush();
    delay(2500);                 // seen, then carry on booting normally
  }
  else {
    lcd.setCursor(0, 0); lcd.print(F("EEprom FAIL at  "));
    lcd.setCursor(0, 1); lcd.print(F("0x"));
    SelfTestPrintAddr(failAddr);
    lcd.print(F("  400kHz"));
    LcdFlush();
    // Halt. A failure here means either the bus speed or the chip, and
    // booting on into normal operation would write patterns over a memory
    // that has just been shown not to hold them. Revert TWI_FREQ to 100000L
    // in lib/WireN/twiN.h and run this again to tell the two apart.
    while (1){ }
  }
}

//==================================================================================================
// [3B] Bank format check at boot
//
// The bank header makes the chip self-describing, so a v1 bank or a chip that
// lost a page is detected rather than played. Nothing is written without an
// explicit key press: converting and initialising are both destructive, and
// which one is wanted is not something the firmware can infer.
//==================================================================================================
void BankCheckBoot()
{
  unsigned int resume = 0;
  const byte state = BankCheck(&resume);

  if (state == 1) return;                       // good v2 bank, carry on

  // [F25] A range clear or move did not finish. Unlike the migration, this is
  // not resumable: a rotation is followed cycle by cycle and there is no way to
  // tell from outside how far it got. Say so plainly rather than carrying on as
  // though the bank were sound - a bank that looks fine and is not is the worst
  // outcome available here.
  if (state == 3){
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print(F("Ptn op cut off  "));
    lcd.setCursor(0, 1); lcd.print(F("check patterns  "));
    LcdFlush();
    for (;;){
      ButtonGet();
      if (enterBtn.justPressed) break;
    }
    BankWriteHeader(BANK_STATE_OK, 0);
    return;
  }

  // [3B] Header missing but the patterns themselves are v2. That is what a
  // SysEx restore leaves behind: the protocol carries patterns and the global
  // setup, not the bank header. Rebuild it rather than offering to erase a
  // bank that was just successfully restored - which would be the single worst
  // thing to offer at that moment.
  if (state == 0 && PatternLooksV2(0)){
    BankWriteHeader(BANK_STATE_OK, 0);
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print(F("Bank header     "));
    lcd.setCursor(0, 1); lcd.print(F("rebuilt         "));
    LcdFlush();
    delay(1200);
    return;
  }

  lcd.clear();
  if (state == 2){
    // An interrupted conversion. The v1 source for everything not yet done is
    // still intact - that is what converting downward buys - so finish it
    // without asking. Stopping here would leave a bank half in each format.
    lcd.setCursor(0, 0); lcd.print(F("Resuming conv.  "));
    lcd.setCursor(0, 1); lcd.print(F("do not power off"));
    LcdFlush();
    MigrateV1toV2(resume);
  }
  else {
    lcd.setCursor(0, 0); lcd.print(F("Bank is not v2  "));
    lcd.setCursor(0, 1); lcd.print(F("PLAY=conv ENT=nw"));
    LcdFlush();

    for (;;){
      ButtonGet();
      if (playBtn.pressed){
        lcd.clear();
        lcd.setCursor(0, 0); lcd.print(F("Converting v1-v2"));
        lcd.setCursor(0, 1); lcd.print(F("do not power off"));
        LcdFlush();
        MigrateV1toV2(MIGRATE_FROM_START);
        break;
      }
      if (enterBtn.pressed){
        lcd.clear();
        lcd.setCursor(0, 0); lcd.print(F("Erasing bank... "));
        lcd.setCursor(0, 1); lcd.print(F("do not power off"));
        LcdFlush();
        InitEEprom();
        break;
      }
    }
  }

  lcd.clear();
  lcd.setCursor(0, 0); lcd.print(F("Bank ready      "));
  lcd.setCursor(0, 1); lcd.print(F("format v2       "));
  LcdFlush();
  delay(1500);
}

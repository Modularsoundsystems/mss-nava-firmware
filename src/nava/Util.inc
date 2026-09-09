//-------------------------------------------------
//                  NAVA v2.x
//            Pattern utilities  [F25]
//
// Clear a range of patterns, or move one so the rest slide to fill the gap.
// Held CLEAR + PTRN to enter, CLEAR to leave.
//
// The copying is the easy part. Three things make this real work:
//
//   1. Tracks reference patterns by number, so moving one silently rewrites
//      what every song containing it plays. Tracks are remapped afterwards.
//   2. groupPos / groupLength describe a pattern's place in a contiguous
//      block, which a move breaks by definition. Group membership is cleared
//      for everything in the affected span, and that is stated on screen
//      rather than done quietly.
//   3. It is slow and destructive. Only runs stopped, only on a one-second
//      ENTER hold, and it journals to the bank header so an interrupted run
//      is detectable at the next boot instead of leaving a bank that looks
//      fine and is not.
//
// The move is a rotation of the affected span, done by cycle-following so each
// record is written exactly once. The three-reversal method is the textbook
// answer and would move every record three times - at fourteen pages of EEprom
// per move, that is three times the wear and three times the wait.
//-------------------------------------------------

////////////////////////Function//////////////////////

// One whole record, EEprom to EEprom, a page at a time through a 64-byte local.
//
// It must NOT use pattern[!ptrnBuffer]: the rotation below parks the record
// displaced by each cycle in that buffer and writes it back at the end of the
// cycle. Copying through the same buffer overwrote the parked record, so the
// moved pattern was replaced by a duplicate of whatever had been copied last -
// it vanished, which is exactly what happened. A page at a time costs the same
// number of EEprom operations and leaves both pattern buffers alone.
static void UtilCopyRecord(byte from, byte to)
{
  byte page[MAX_PAGE_SIZE];
  const unsigned long s = PatternAddr(from);
  const unsigned long d = PatternAddr(to);

  for (byte p = 0; p < PTRN_PAGES; p++){
    EepRead (s + (unsigned long)p * MAX_PAGE_SIZE, page, (byte)MAX_PAGE_SIZE);
    EepWrite(d + (unsigned long)p * MAX_PAGE_SIZE, page, (byte)MAX_PAGE_SIZE);
  }
}

static void UtilReadRecord(byte n)
{
  byte *buf = (byte *)&pattern[!ptrnBuffer];
  const unsigned long s = PatternAddr(n);
  for (byte p = 0; p < PTRN_PAGES; p++)
    EepRead(s + (unsigned long)p * MAX_PAGE_SIZE, buf + ((unsigned int)p * MAX_PAGE_SIZE), (byte)MAX_PAGE_SIZE);
}

static void UtilWriteRecord(byte n)
{
  const byte *buf = (const byte *)&pattern[!ptrnBuffer];
  const unsigned long d = PatternAddr(n);
  for (byte p = 0; p < PTRN_PAGES; p++)
    EepWrite(d + (unsigned long)p * MAX_PAGE_SIZE, buf + ((unsigned int)p * MAX_PAGE_SIZE), (byte)MAX_PAGE_SIZE);
}

// Blank [first, first+count-1] in place. Clear-all is this with the full range.
static void UtilClearRange(byte first, byte count)
{
  for (byte i = 0; i < count && (byte)(first + i) < MAX_PTRN; i++){
    InitPatternRecord(!ptrnBuffer, (byte)(first + i));
    UtilWriteRecord((byte)(first + i));
    SetDoutLed(1U << (((unsigned int)i * 16) / count), 0, 0);
  }
}

// [4T] Overlay the copy buffer onto every pattern of a range, through the paste
// mask. "Give these thirty-two patterns the same choke rules" in one gesture,
// which by hand is thirty-two visits to a page.
//
// Each record is read, overlaid and written back, so everything the mask does
// not select survives - this is the same overlay a single paste performs, from
// the same function, and cannot drift from it. Records that fail their magic
// check are started from a blank rather than overlaid onto whatever the bytes
// happened to be: a pattern that was already unreadable is not made worse, and
// nothing is written on the strength of bytes nobody has validated.
static void UtilPasteRange(byte first, byte count)
{
  for (byte i = 0; i < count && (byte)(first + i) < MAX_PTRN; i++){
    const byte n = (byte)(first + i);
    UtilReadRecord(n);
    if (!PatternRecordValid(!ptrnBuffer)) InitPatternRecord(!ptrnBuffer, n);
    PasteMaskInto(&pattern[!ptrnBuffer], &bufferedPattern, seq.pasteMask);
    SetHHPatternBuf(!ptrnBuffer);
    UtilWriteRecord(n);
    SetDoutLed(1U << (((unsigned int)i * 16) / count), 0, 0);
  }
}

// Where a pattern in the rotated span ends up. Shared by the rotation and the
// track remap so the two cannot disagree about what just happened.
static byte UtilRotatedIndex(byte p, byte lo, byte m, byte r)
{
  const byte o = (byte)(p - lo);
  return (byte)(lo + (byte)((o + m - r) % m));
}

// Rewrite every track that referenced a moved pattern. Without this a move
// silently changes what every song plays - the patterns would be fine and the
// songs would be wrong, which is the harder of the two to notice.
static void UtilRemapTracks(byte lo, byte hi, byte m, byte r)
{
  const byte keep = trk.current;

  for (byte t = 0; t < MAX_TRACK; t++){
    LoadTrack(t);
    boolean changed = FALSE;

    unsigned int used = track[trkBuffer].length;
    if (used >= MAX_PTRN_TRACK) used = MAX_PTRN_TRACK - 1;

    for (unsigned int i = 0; i <= used; i++){
      const byte v = track[trkBuffer].patternNbr[i];
      if (v >= lo && v <= hi){
        const byte nv = UtilRotatedIndex(v, lo, m, r);
        if (nv != v){ track[trkBuffer].patternNbr[i] = nv; changed = TRUE; }
      }
    }
    if (changed) SaveTrack(t);
    SetDoutLed(1U << t, 0, 0);
  }

  LoadTrack(keep);
}

// A move breaks any group the moved patterns belonged to, because a group is
// defined by contiguous position. Clearing is the honest outcome; silently
// leaving stale groupPos values would make patterns claim membership of blocks
// that no longer exist.
static void UtilClearGroups(byte lo, byte hi)
{
  byte g[2];
  g[0] = 0;
  g[1] = 0;
  for (byte p = lo; p <= hi; p++) EepWrite(PatternAddr(p) + OFFSET_GROUP, g, (byte)GROUP_SIZE);
}

// Move [first, first+count-1] so it starts at dest, sliding what lies between
// them into the vacated space. That is a rotation of the span the two ranges
// jointly cover.
static void UtilMoveRange(byte first, byte count, byte dest)
{
  if (count == 0 || dest == first) return;

  byte lo, hi, r;
  if (dest > first){
    lo = first;
    hi = (byte)(dest + count - 1);
    r  = count;                                   // left-rotate: the range goes to the end
  }
  else {
    lo = dest;
    hi = (byte)(first + count - 1);
    r  = (byte)((hi - lo + 1) - count);           // left-rotate by m-count == right-rotate by count
  }
  if (hi >= MAX_PTRN) return;

  const byte m = (byte)(hi - lo + 1);
  if (m < 2 || r == 0 || r >= m) return;

  // Juggling rotation. Each record is read once and written once; the cycles
  // are followed rather than the span reversed three times.
  byte movedTotal = 0;
  for (byte start = 0; movedTotal < m; start++){
    UtilReadRecord((byte)(lo + start));           // hold the element this cycle displaces
    byte i = start;
    for (;;){
      const byte j = (byte)((i + r) % m);
      if (j == start) break;
      UtilCopyRecord((byte)(lo + j), (byte)(lo + i));
      movedTotal++;
      i = j;
      SetDoutLed(1U << (((unsigned int)movedTotal * 16) / m), 0, 0);
    }
    UtilWriteRecord((byte)(lo + i));
    movedTotal++;
  }

  UtilClearGroups(lo, hi);
  UtilRemapTracks(lo, hi, m, r);
}

// Run whatever the panel is currently showing. Stopped only: this rewrites
// megabytes-worth of pages over several seconds and takes the back pattern
// buffer as scratch, neither of which is survivable underneath a running
// sequencer.
void UtilExecute()
{
  lcd.clear();
  lcd.setCursor(0,0); lcd.print(F("Working...      "));
  lcd.setCursor(0,1); lcd.print(F("do not power off"));
  LcdFlush();

  BankWriteHeader(BANK_STATE_UTIL, 0);

  if      (utilOp == UTIL_OP_CLR) UtilClearRange(utilFirst, utilCount);
  else if (utilOp == UTIL_OP_PST) UtilPasteRange(utilFirst, utilCount);   // [4T]
  else                            UtilMoveRange(utilFirst, utilCount, utilDest);

  BankWriteHeader(BANK_STATE_OK, 0);
  SetDoutLed(0, 0, 0);

  // Whatever is loaded may have just been overwritten or moved out from under
  // the playhead, so pull the current slot in again through the normal path.
  nextPattern = curPattern;
  selectedPatternChanged = TRUE;
  UndoDiscard();

  lcd.clear();
  lcd.setCursor(0,0); lcd.print(F("Done            "));
  LcdFlush();
  delay(900);
  needLcdUpdate = TRUE;
}

// Panel handling while the utility page is open.
void UtilButtons()
{
  // [3C] CLEAR resets the subject of the page rather than leaving it, which is
  // the rule every other page follows now. INST+PTRN closes this one, as does
  // any transport key.
  if (clearBtn.justPressed){
    utilOp    = UTIL_OP_CLR;
    utilFirst = 0;
    utilCount = 1;
    utilDest  = 0;
    needLcdUpdate = TRUE;
    return;
  }

  if (encBtn.justPressed){
    curIndex++;
    if (curIndex >= MAX_CUR_POS) curIndex = 0;
    needLcdUpdate = TRUE;
  }

  // Step buttons jump the field under the cursor to a bank boundary, which is
  // how the panel already thinks about pattern numbers.
  if (inStepPressed && curIndex != 0){
    const byte b = FirstBitOn();
    if (curIndex == 1)      utilFirst = (byte)(b * 8);
    else if (curIndex == 3){ if (utilOp == UTIL_OP_MOV) utilDest = (byte)(b * 8); }  // [4T]
    else {
      // [3E] Slot 2 is the last pattern of the range now, so the step buttons
      // mean the same thing here as they do on the other two - pattern b*8, not
      // a count of b+1. A last that lands before the first is a range of one.
      const byte last = (byte)(b * 8);
      utilCount = (last > utilFirst) ? (byte)(last - utilFirst + 1) : 1;
    }
    needLcdUpdate = TRUE;
  }

  if (enterBtn.hold){
    if (isRunning){
      lcd.setCursor(0,0);
      lcd.print(F("STOP FIRST      "));
      LcdFlush();
    }
    // [4T] A never-copied buffer is all zeroes, and pasting it across a range
    // would blank every pattern in it. Say so rather than doing it.
    else if (utilOp == UTIL_OP_PST && !bufferHasPattern){
      lcd.setCursor(0,0);
      lcd.print(F("COPY A PTRN 1ST "));
      LcdFlush();
    }
    else {
      UtilExecute();
      utilMode = FALSE;
    }
  }
}

void UtilEncoder()
{
  switch (curIndex){
  case 0:
    utilOp = (byte)constrain(EncGet(utilOp, 1), 0, UTIL_OP_MAX);
    break;
  case 1: {
    // [3E] The page reads "frm ... to ...", so moving the start leaves the end
    // where it is and the range grows or shrinks against it. Carrying the count
    // instead would drag the end along, which is not what a pair of endpoints
    // does when you move one of them.
    const byte last = (byte)(utilFirst + utilCount - 1);
    utilFirst = (byte)constrain(EncGet(utilFirst, 1), 0, MAX_PTRN - 1);
    utilCount = (last >= utilFirst) ? (byte)(last - utilFirst + 1) : 1;
    break;
  }
  case 2: {
    // Stored as a count, edited as the last pattern. Never below the first: a
    // backwards range has no meaning here and clamping is kinder than refusing.
    byte last = (byte)(utilFirst + utilCount - 1);
    last = (byte)constrain(EncGet(last, 1), utilFirst, MAX_PTRN - 1);
    utilCount = (byte)(last - utilFirst + 1);
    break;
  }
  case 3:
    // [4T] Only the move has a destination. On a paste this field is showing
    // where the buffer came from, which is not something the knob gets to change.
    if (utilOp == UTIL_OP_MOV) utilDest = (byte)constrain(EncGet(utilDest, 1), 0, MAX_PTRN - 1);
    break;
  }

  // Keep the request describable: a range cannot run past the last pattern,
  // and a move cannot land where it would.
  if ((unsigned int)utilFirst + utilCount > MAX_PTRN) utilCount = (byte)(MAX_PTRN - utilFirst);
  if ((unsigned int)utilDest + utilCount > MAX_PTRN)  utilDest  = (byte)(MAX_PTRN - utilCount);

  static byte prevOp, prevFirst, prevCount, prevDest;
  if (utilOp != prevOp || utilFirst != prevFirst || utilCount != prevCount || utilDest != prevDest){
    prevOp = utilOp; prevFirst = utilFirst; prevCount = utilCount; prevDest = utilDest;
    needLcdUpdate = TRUE;
  }
}

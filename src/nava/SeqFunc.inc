//-------------------------------------------------
//                  NAVA v1.x
//                 SEQ Functions
//-------------------------------------------------

/////////////////////Function//////////////////////
//Initialyse the sequencer before to run
void InitSeq()
{
  LoadSeqSetup();
  ppqn = 0;
  stepCount = 0;
  seq.configPage = 0;
  seq.configMode = FALSE;
  randomSeed(analogRead(0));
  seq.dir = FORWARD;
  seq.bpm = seq.defaultBpm;
  SetSeqSync();                               // [zabox] [1.028] moved
  seq.syncChanged = FALSE;
}

//Combine OH and CH pattern to trig HH and set total accent for ride and crash
// [B-26] Buffer-parameterised so the BACK buffer can be fully prepared while
// the front buffer keeps playing. SetHHPattern() keeps the old meaning.
// [3B] Non-destructive. The hat still fires from HH, which is the OR of the two
// lanes - that has not changed. What has gone is the loop that enforced "only
// one of CH/OH per step" by clearing the other lane's bit, and which in the OH
// case also overwrote velocity[CH] with instVelHigh[HH]. Writing a hi-hat step
// therefore destroyed whatever the other lane held at that position, with no
// way back.
//
// Both lanes now keep every step they were given. The collision is real - one
// voice cannot play open and closed at once - but it is a *playback* question,
// so it is answered at fire time by seq.hhPriority, where it costs nothing and
// is reversible.
void SetHHPatternBuf(boolean buf)
{
  pattern[buf].inst[HH] = pattern[buf].inst[CH] | pattern[buf].inst[OH];
}

void SetHHPattern()
{
  //static boolean rideTotalAcc;          [zabox] [1.028] unused
  //static boolean crashTotalAcc;
  //static boolean ohTotalAcc;

  // [3B] Destructive CH/OH exclusivity removed - see SetHHPatternBuf() above.
  pattern[ptrnBuffer].inst[HH] = pattern[ptrnBuffer].inst[CH] | pattern[ptrnBuffer].inst[OH];
  if (curSeqMode != PTRN_TAP){
    for (int a = 0; a < NBR_STEP; a++){

      //Set total accent velocity for Ride and Crash
      //Ride
      
 /*     if (bitRead(pattern[ptrnBuffer].inst[RIDE],a)){
        if (bitRead(pattern[ptrnBuffer].inst[TOTAL_ACC],a))rideTotalAcc = TRUE;                                                           // [zabox] [1.028] looks like old code that is redundant?
        else rideTotalAcc = FALSE;                                                                                                        //                 with the code, total acc is applied twice
      }
      if (rideTotalAcc) pattern[ptrnBuffer].velocity[RIDE][a] = instVelHigh[RIDE] + pattern[ptrnBuffer].totalAcc * 4;
      else pattern[ptrnBuffer].velocity[RIDE][a] = instVelHigh[RIDE];

      //Crash
      if (bitRead(pattern[ptrnBuffer].inst[CRASH],a)){
        if (bitRead(pattern[ptrnBuffer].inst[TOTAL_ACC],a))crashTotalAcc = TRUE;
        else crashTotalAcc = FALSE;
      }
      if (crashTotalAcc) pattern[ptrnBuffer].velocity[CRASH][a] = instVelHigh[CRASH] + pattern[ptrnBuffer].totalAcc * 4;
      else pattern[ptrnBuffer].velocity[CRASH][a] = instVelHigh[CRASH];
      
 */

      /*  //OH accent
       if (bitRead(pattern[ptrnBuffer].inst[OH],a)){
       if (bitRead(pattern[ptrnBuffer].inst[TOTAL_ACC],a))ohTotalAcc = TRUE;
       else ohTotalAcc = FALSE;
       }
       if (ohTotalAcc) pattern[ptrnBuffer].velocity[CH][a] = instVelHigh[HH] + pattern[ptrnBuffer].totalAcc * 4;
       else pattern[ptrnBuffer].velocity[CH][a] = instVelHigh[HH];*/
       
       
    }
  }
}

// [F5][F13] Map a set of held step buttons to the instruments they address.
// The low/high pairs both point at the same voice, and buttons 12+13 together
// mean OH rather than two presses of CH - the same chord that selects OH
// everywhere else, so the gesture means the same thing wherever it appears.
unsigned int InstMaskFromButtons(unsigned int btns)
{
  unsigned int m = 0;
  for (byte a = 0; a < NBR_STEP_BTN; a++){
    if (bitRead(btns, a)) m |= (1U << instOut[a]);
  }
  if ((btns & OH_BTN) == OH_BTN){
    m &= ~(1U << CH);
    m |=  (1U << OH);
  }
  return m;
}

// [F5] Blank the live pattern. [3E] One gesture reaches it now - CLEAR+PTRN, at
// any time. It used to have two, CLEAR alone while stopped and CLEAR+PLAY while
// running, which is one destructive act with two spellings and one of them
// needing no modifier at all.
void ClearWholePattern()
{
  UndoPush();                                                                       // [F4]

  // The trig and flam words are what the ISR reads to build a step. Clearing
  // thirty-two of them one at a time while running would let a step fire from a
  // pattern that is half gone; the loop is short enough to do at once.
  uint8_t sreg = SREG;
  cli();
  for (byte a = 0; a < NBR_INST; a++){
    pattern[ptrnBuffer].inst[a] = 0;
    pattern[ptrnBuffer].flamMask[a] = 0;
  }
  SREG = sreg;

  // [B-23] Every instrument, not the nine the hand-written version listed -
  // that one left OH, CH, HH, EXT_INST, TRIG_OUT and TOTAL_ACC holding the
  // previous pattern's values.
  for (byte a = 0; a < NBR_INST; a++){
    for (byte b = 0; b < NBR_STEP; b++) pattern[ptrnBuffer].velocity[a][b] = instVelLow[a];
  }

  // [F27] Lane lengths go with the notes. A cleared pattern that still had
  // polyrhythmic lanes would be empty and yet not behave like a fresh one.
  for (byte a = 0; a < NBR_INST; a++){
    for (byte b = 0; b < 4; b++) pattern[ptrnBuffer].lane[a][b] = 0;
  }
  LaneSyncAll();

  InitPatternBuf(ptrnBuffer);
  pattern[ptrnBuffer].shuffle = DEFAULT_SHUF;
  pattern[ptrnBuffer].flam    = DEFAULT_FLAM;                                       // [1.028] flam
  pattern[ptrnBuffer].length  = NBR_STEP - 1;
  pattern[ptrnBuffer].scale   = SCALE_16;
  keybOct = DEFAULT_OCT;

  // [3F] A place in a chain goes with the notes. This left the two group bytes
  // alone, so clearing a pattern and saving it wrote an empty record that still
  // announced itself as a link and still handed off to its neighbour - a pattern
  // you have cleared, saved, and which chains anyway, with nothing on the panel
  // to say why. The pattern utilities' CLR has always zeroed these; the clear on
  // the front panel now agrees with it.
  //
  // Only this pattern's own bytes. The others keep the block they were saved
  // with and simply stop when they reach this one, which is what taking a link
  // out of a chain looks like - as against silently rewriting four other records
  // that were not asked about.
  pattern[ptrnBuffer].groupPos    = 0;
  pattern[ptrnBuffer].groupLength = 0;
  // And stop a stored chain that is running on this pattern right now, rather
  // than at whatever pattern change happens to come next. An ad-hoc chain - two
  // pads held - is not membership and keeps running, because nothing about it
  // lives in the record this is clearing. [4Z]

  patternWasEdited = TRUE;
  needLcdUpdate = TRUE;
}

// [3D] Close every overlay page at once. One function, so a new page cannot be
// added and then forgotten by whatever wanted to leave.
void PanelClose()
{
  if (nameEditMode) NameEditExit();
  if (condMode)     CondExit();
  if (browseMode)   BrowseExit();
  utilMode    = FALSE;
  // [5G] The paste page used to be closed here because it borrowed the pads.
  // Nothing borrows them from a config page any more.
  needLcdUpdate = TRUE;
}

// [3C] Put the pattern back to the last thing written to the chip.
//
// The third of the three "undo" gestures, and the one that was missing. Undo
// walks back four edits. Revert-to-snapshot goes to whatever you last copied.
// This goes to what is actually stored - which is what you want after ten
// minutes of live mangling, when the edits are past counting and the snapshot
// was taken somewhere you no longer remember.
//
// Loaded into the back buffer and flipped, exactly as a paste is, so a step can
// never fire from half of each pattern. A queued change owns the back buffer, so
// the revert stands aside rather than fighting it - the bar line is about to
// bring in a different pattern anyway.
void RevertToSaved()
{
  if (nextPatternReady) return;

  UndoPush();
  LoadPattern(curPattern);              // into pattern[!ptrnBuffer]
  SetHHPatternBuf(!ptrnBuffer);
  InitPatternBuf(!ptrnBuffer);

  uint8_t sreg = SREG;
  cli();
  ptrnBuffer = !ptrnBuffer;
  muxArmedForNext = FALSE;              // anything armed came from the mangled copy
  SREG = sreg;

  LaneSyncAll();
  patternNeedSaved = FALSE;
  patternWasEdited = FALSE;
  needLcdUpdate = TRUE;
}

void CopyPatternToBuffer(byte patternNum)
{
  bufferedPattern  = pattern[ptrnBuffer];
  bufferHasPattern = TRUE;                  // [4T]
  bufferSource     = patternNum;
}

//paste buffer to current pattern
//
// Pastes into the BACK buffer, never over the one the ISR is reading.
//
// Two reasons. A 896-byte struct copy cannot be atomic, so writing it into the
// live buffer lets a step fire from half of each pattern. And [B-26] preloads a
// queued pattern change into the back buffer at queue time - so pasting into
// the front buffer while a change was pending wrote into the pattern being left
// behind, and the swap at the bar line then threw the paste away. That is the
// reported symptom: the rest of the bar plays the pasted material, then the bar
// line brings in the empty pattern that was queued all along.
//
// With the paste in the back buffer, a pending change delivers it at the bar
// line as part of the swap it was already going to do; with no change pending,
// the flip happens here and is a single byte, which is atomic.
// [4T] The mask overlay itself, against any two records. Extracted so the group
// paste and the single paste cannot drift apart - a field added to one and
// forgotten in the other would be a paste that carries different things
// depending on how you asked for it, and nothing on the panel would say so.
//
// [F29] Starts from the destination and overlays only what the mask selects.
// Copying the whole record and putting the excluded parts back would mean naming
// every field twice - once to overwrite it and once to restore it - and a field
// added later would silently be pasted regardless of the mask because nobody
// remembered to add it to the restore list.
void PasteMaskInto(Pattern *d, const Pattern *s, byte m)
{
  if (m & PASTE_TRIG)  for (byte i = 0; i < NBR_INST; i++) d->inst[i]     = s->inst[i];
  if (m & PASTE_FLAM)  for (byte i = 0; i < NBR_INST; i++) d->flamMask[i] = s->flamMask[i];
  if (m & PASTE_VEL)   memcpy(d->velocity, s->velocity, sizeof(d->velocity));
  if (m & PASTE_NAME)  for (byte i = 0; i < 12; i++)       d->name[i]     = s->name[i];

  if (m & PASTE_SETUP){
    d->length   = s->length;
    d->scale    = s->scale;
    d->dir      = s->dir;
    d->shuffle  = s->shuffle;
    d->flam     = s->flam;
    d->totalAcc = s->totalAcc;
    // [F19] Swing travels with the setup, not with the conditions it was parked
    // beside while it was only a reserved field. It is feel - the same thing
    // shuffle and flam are - and "take a pattern's feel without its notes" is
    // the gesture this mask exists for.
    memcpy(d->instSwing, s->instSwing, sizeof(d->instSwing));
  }
  if (m & PASTE_EXT){
    memcpy(d->extNote, s->extNote, sizeof(d->extNote));
    d->extLength = s->extLength;
  }
  if (m & PASTE_EUCL) memcpy(d->lane, s->lane, sizeof(d->lane));
  if (m & PASTE_COND){
    memcpy(d->stepCond,     s->stepCond,     sizeof(d->stepCond));
    memcpy(d->instCondProb, s->instCondProb, sizeof(d->instCondProb));
    memcpy(d->instLink,     s->instLink,     sizeof(d->instLink));
    memcpy(d->choke,        s->choke,        sizeof(d->choke));
  }
}

void PasteBufferToPattern(byte patternNum)
{
  (void)patternNum;

  // When a change is queued the destination is the queued pattern, already in
  // the back buffer courtesy of [B-26]; otherwise it is the one playing now.
  // The mask overlay itself is PasteMaskInto(), shared with the group paste.
  if (!nextPatternReady) pattern[!ptrnBuffer] = pattern[ptrnBuffer];

  PasteMaskInto(&pattern[!ptrnBuffer], &bufferedPattern, seq.pasteMask);
  SetHHPatternBuf(!ptrnBuffer);
  InitPatternBuf(!ptrnBuffer);

  if (!nextPatternReady){
    uint8_t sreg = SREG;
    cli();
    ptrnBuffer = !ptrnBuffer;
    muxArmedForNext = FALSE;      // anything armed came from the pattern just replaced
    SREG = sreg;
    LaneSyncAll();                // [F27] the pasted record has its own lane lengths
  }
  needLcdUpdate = TRUE;
}

// [3B] One place that knows what a blank v2 record is: used by InitEEprom(),
// by LoadPattern() when a record fails its magic check, and by the migration.
void InitPatternRecord(boolean buf, byte n)
{
  Pattern *p = &pattern[buf];
  for (unsigned int i = 0; i < sizeof(Pattern); i++) ((byte *)p)[i] = 0;

  p->magic0    = PTRN_MAGIC0;
  p->magic1    = PTRN_MAGIC1;
  p->formatVer = PTRN_FORMAT;

  for (byte i = 0; i < 12; i++) p->name[i] = ' ';
  p->name[0] = 'P';
  p->name[1] = (char)('0' + ((n + 1) / 100) % 10);
  p->name[2] = (char)('0' + ((n + 1) / 10) % 10);
  p->name[3] = (char)('0' + (n + 1) % 10);

  p->length  = DEFAULT_LEN - 1;
  p->scale   = DEFAULT_SCALE;
  p->shuffle = DEFAULT_SHUF;
  p->flam    = DEFAULT_FLAM;
  p->dir     = FORWARD;
}

//init pattern
// [B-26] The velocity/scale fix-ups are buffer-parameterised so they can run on
// the BACK buffer at load time. The group globals are NOT done here: they must
// still be refreshed at the moment the swap actually takes effect, which is
// what [4Z] retired: nothing derives chain state from a record any more.
// bookkeeping on exactly the timing it had before.
void InitPatternBuf(boolean buf)
{
  //Init Ride, Crash velocity to HIGH_VEL
  //
  // [6Z] ...unless the cymbal accent is on, in which case these two carry a level
  // like every other voice and this would flatten it on every pattern load.
  //
  // Kept for the OFF case rather than simply skipped: it is also the repair. Turn
  // the switch off and any weak cymbal step written while it was on goes back to
  // strong the next time the pattern is loaded, so the setting means the same
  // thing for a pattern you wrote yesterday as for one you write now.
  for (int stp = 0; stp < NBR_STEP; stp++){
    if (pattern[buf].velocity[CH][stp] == 0)  pattern[buf].velocity[CH][stp] = instVelHigh[HH];//HH
    if (!seq.cymAccent){
      pattern[buf].velocity[CRASH][stp] = instVelHigh[CRASH];//CRASH
      pattern[buf].velocity[RIDE][stp] = instVelHigh[RIDE];//RIDE
    }
    pattern[buf].velocity[TOTAL_ACC][stp] = HIGH_VEL;//TOTAL_ACC
    pattern[buf].velocity[TRIG_OUT][stp] = HIGH_VEL;//TRIG_OUT
    pattern[buf].velocity[EXT_INST][stp] = HIGH_VEL;//EXT_INST
  }
  // [6I] The scale LED is derived from scaleBtn.counter and the clock is driven
  // by pattern.scale, so these two ARE the same fact and this switch is the only
  // thing keeping them so. It had no default, which meant a scale byte outside
  // the four simply left the counter holding the PREVIOUS pattern's value: the
  // LED went on saying 1/16 while the sequencer ran at whatever the byte said.
  //
  // Reported as exactly that - "scale led showed 1/16, pattern was advancing
  // about 4 times slower" - and cured by cycling the button all the way round,
  // which is the one gesture that writes the pattern from the counter instead of
  // the counter from the pattern.
  //
  // A missing default in a switch over a value that came off an EEprom is a
  // silent "keep whatever was there", and what was there belongs to a different
  // pattern. Repair rather than pick a counter to match: a scale that is not one
  // of the four is not playable, so put the pattern back on 1/16 and say so on
  // the LED. Anything else leaves the machine running on a number nothing on the
  // panel can express or undo.
  switch (pattern[buf].scale){
  case  SCALE_16:
    scaleBtn.counter = 0;
    break;
  case  SCALE_32:
    scaleBtn.counter = 1;
    break;
  case  SCALE_8t:
    scaleBtn.counter = 2;
    break;
  case  SCALE_16t:
    scaleBtn.counter = 3;
    break;
  default:
    pattern[buf].scale = SCALE_16;
    scaleBtn.counter = 0;
    break;
  }
}

// [4Z] Chains are no longer stored, so nothing reads groupPos/groupLength off a
// record and UpdateGroupFromPattern() is gone with them.
//
// It re-seated group.firstPattern and group.pos from the record on every buffer
// swap, which is a chain re-deriving where it began from two bytes per pattern,
// once a bar, forever. [3F] had already had to add a validity check because
// stray bytes in an untouched record chained patterns to their neighbours; the
// remaining fault was subtler and the same shape - if the members disagreed
// about the span, and only some of them had ever been written, the chain re-
// seated differently each time round and played its first pattern two or three
// times before moving on. Clearing the patterns fixed it, which is the tell:
// the fault was in the stored bytes, not in the chain.
//
// A chain is now what it looks like on the panel - two pads, held in mind by the
// machine until you pick something else. Nothing to save, nothing to go stale,
// and no second source of truth for where the chain starts.

void InitPattern()
{
  InitPatternBuf(ptrnBuffer);
}

// [B-21] InstToStepWord() deleted - wrote Pattern.step[16], which no code
//        ever read. Removed the call from the Timer1 ISR and the 96 bytes
//        of RAM the array occupied.

// [B-33] The flam mask is a second bitmask over the same steps, so a shift that
// moves trigs has to move it too - otherwise the flams stay where they were and
// land on whichever trigs slid underneath them.
static inline void RotateFlamLeft(byte inst)
{
  unsigned int f = pattern[ptrnBuffer].flamMask[inst];
  pattern[ptrnBuffer].flamMask[inst] = (unsigned int)(f >> 1) | (unsigned int)((f & 1) << 15);
}

static inline void RotateFlamRight(byte inst)
{
  unsigned int f = pattern[ptrnBuffer].flamMask[inst];
  pattern[ptrnBuffer].flamMask[inst] = (unsigned int)(f << 1) | (unsigned int)((f >> 15) & 1);
}

//shift left pattern
void ShiftLeftPattern()
{
  patternWasEdited = TRUE;
  if (instBtn){//only shift selected instruments
    if (bitRead(pattern[ptrnBuffer].inst[curInst],0)){
      pattern[ptrnBuffer].inst[curInst] = pattern[ptrnBuffer].inst[curInst]>>1 | (1<<15);
      if (pattern[ptrnBuffer].velocity[curInst][0] > instVelLow[curInst]) pattern[ptrnBuffer].velocity[curInst][15] = instVelHigh[curInst];
      else pattern[ptrnBuffer].velocity[curInst][15] = instVelLow[curInst];
    }
    else pattern[ptrnBuffer].inst[curInst] = pattern[ptrnBuffer].inst[curInst]>>1;
    RotateFlamLeft(curInst);                                                          // [B-33] flam travels with its trig
    //update instrument velocity
    for (int stp = 0; stp < NBR_STEP - 1; stp++){
      pattern[ptrnBuffer].velocity[curInst][stp] = pattern[ptrnBuffer].velocity[curInst][stp+1];
    }
  }
  else{// shift entire pattern
    for (int nbrInst = 0; nbrInst < NBR_INST; nbrInst++){
      if (bitRead(pattern[ptrnBuffer].inst[nbrInst],0)){
        pattern[ptrnBuffer].inst[nbrInst] = pattern[ptrnBuffer].inst[nbrInst]>>1 | (1<<15);
        if (pattern[ptrnBuffer].velocity[nbrInst][0] > instVelLow[nbrInst]) pattern[ptrnBuffer].velocity[nbrInst][15] = instVelHigh[nbrInst];
        else pattern[ptrnBuffer].velocity[nbrInst][15] = instVelLow[nbrInst];
      }
      else pattern[ptrnBuffer].inst[nbrInst] = pattern[ptrnBuffer].inst[nbrInst]>>1;
      RotateFlamLeft(nbrInst);                                                        // [B-33]
      //update instrument velocity
      for (int stp = 0; stp < NBR_STEP - 1; stp++){
        pattern[ptrnBuffer].velocity[nbrInst][stp] = pattern[ptrnBuffer].velocity[nbrInst][stp+1];
      }
    }
  }
}

//Shift Right pattern
void ShiftRightPattern()
{
  patternWasEdited = TRUE; 
  if (instBtn){//shift only selected instrument
    if (bitRead(pattern[ptrnBuffer].inst[curInst],15)){
      pattern[ptrnBuffer].inst[curInst] = pattern[ptrnBuffer].inst[curInst]<<1 | 1;
      if (pattern[ptrnBuffer].velocity[curInst][15] > instVelLow[curInst]) pattern[ptrnBuffer].velocity[curInst][0] = instVelHigh[curInst];
      else pattern[ptrnBuffer].velocity[curInst][0] = instVelLow[curInst];
    }
    else pattern[ptrnBuffer].inst[curInst] = pattern[ptrnBuffer].inst[curInst]<<1;
    RotateFlamRight(curInst);                                                         // [B-33] flam travels with its trig
    //update instrument velocity
    for (int stp = NBR_STEP - 1; stp > 0; stp--){
      pattern[ptrnBuffer].velocity[curInst][stp] = pattern[ptrnBuffer].velocity[curInst][stp-1];
    }
  }
  else{//shift entire pattern
    for (int nbrInst = 0; nbrInst < NBR_INST; nbrInst++){
      if (bitRead(pattern[ptrnBuffer].inst[nbrInst],15)){
        pattern[ptrnBuffer].inst[nbrInst] = pattern[ptrnBuffer].inst[nbrInst]<<1 | 1;
        if (pattern[ptrnBuffer].velocity[nbrInst][15] > instVelLow[nbrInst]) pattern[ptrnBuffer].velocity[nbrInst][0] = instVelHigh[nbrInst];
        else pattern[ptrnBuffer].velocity[nbrInst][0] = instVelLow[nbrInst];
      }
      else pattern[ptrnBuffer].inst[nbrInst] = pattern[ptrnBuffer].inst[nbrInst]<<1;
      RotateFlamRight(nbrInst);                                                       // [B-33]
      //update instrument velocity
      for (int stp = NBR_STEP - 1; stp > 0; stp--){
        pattern[ptrnBuffer].velocity[nbrInst][stp] = pattern[ptrnBuffer].velocity[nbrInst][stp-1];
      }
    }
  }

}


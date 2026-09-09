//-------------------------------------------------
//                  NAVA v2.x
//        Trig conditions and probability  [F18]
//
// SHIFT+SCALE opens the page.
//
// Every step of every instrument carries one byte: the high nibble is a cycle
// condition, the low nibble a probability. Both were reserved in the v2 record
// back in 3B, which is why this costs no format change.
//
// A cycle condition is "X:Y" - play on the Xth of every Y passes of the
// pattern. 1:2 gives a note every other bar, 3:4 the third of four, and so on.
// Probability is independent of it and applies afterwards, so 1:4 at 50 % is a
// note that happens on one bar in four, and only half of those.
//
// Step 0 of the page is ALL: the same pair applied to the whole instrument,
// which is the per-instrument half of the request. Instrument and step
// conditions both have to pass, so a lane can be thinned as a whole without
// touching the steps inside it.
//
// Evaluated in PrepareNextStep(), not at fire time. random() is not cheap and
// the trigger edge is where jitter comes from; the result is latched into a
// mask that FireStep() applies. [B-03] still holds - the trigger word is
// rebuilt at fire time - but the *decision* is made half a step earlier, which
// also means the S/H caps are charged only for notes that will actually sound.
//-------------------------------------------------

#define COND_COUNT 12

// X of every Y. Index 0 is "always", and its pair is never read.
const byte condXY[COND_COUNT][2] PROGMEM = {
  {0,0}, {1,2}, {2,2}, {1,3}, {2,3}, {3,3},
  {1,4}, {2,4}, {3,4}, {4,4}, {1,8}, {5,8}
};

////////////////////////Function//////////////////////

static boolean CondCyclePasses(byte c)
{
  if (c == 0 || c >= COND_COUNT) return TRUE;
  const byte x = pgm_read_byte(&condXY[c][0]);
  const byte y = pgm_read_byte(&condXY[c][1]);
  return ((byte)(barCount % y) == (byte)(x - 1));
}

// 0 is always. 1..15 is that many sixteenths of a chance, so 15 is almost
// always and 1 is rare - a scale that reads the same way round as density.
static boolean CondProbPasses(byte p)
{
  if (p == 0) return TRUE;
  return ((byte)random(0, 16) < p);
}

// Instrument first, then the step. Both must pass: the instrument pair thins
// the whole lane, the step pair decides individual notes within it.
static boolean CondAllows(byte z, byte idx)
{
  const byte ic = pattern[ptrnBuffer].instCondProb[z];
  if (!CondCyclePasses((byte)(ic >> 4))) return FALSE;
  if (!CondProbPasses((byte)(ic & 0x0F))) return FALSE;

  const byte sc = pattern[ptrnBuffer].stepCond[z][idx];
  if (!CondCyclePasses((byte)(sc >> 4))) return FALSE;
  if (!CondProbPasses((byte)(sc & 0x0F))) return FALSE;
  return TRUE;
}

// Which instruments are allowed to sound on the step about to play. Evaluated
// for every instrument, not only those with a trig, so a note tapped between
// this and the trigger still gets the answer its step was given.
unsigned int BuildCondMask(byte s)
{
  unsigned int m = 0;
  for (byte z = 0; z < NBR_INST; z++){
    if (CondAllows(z, LaneIndex(z, s))) bitSet(m, z);
  }
  // [F21] Links, applied after the base mask rather than woven into it. A rule
  // reads whether its target was allowed to sound, not whether the target's own
  // link then let it - so two instruments pointing at each other resolve against
  // the pre-link answer instead of chasing one another round. Cycle-free by
  // construction rather than by asking the user not to make one.
  unsigned int linked = m;
  for (byte z = 0; z < NBR_INST; z++){
    if (!bitRead(m, z)) continue;
    const byte lk   = pattern[ptrnBuffer].instLink[z];
    const byte rule = (byte)(lk & 0x0F);
    if (rule == LINK_NONE) continue;

    const byte ti = (byte)(lk >> 4);
    if (ti >= NBR_VOICE) continue;                       // no target set
    const byte t  = voiceOrder[ti];

    const boolean nowT = bitRead(m, t) &&
                         !LaneSilent(t) &&
                         bitRead(pattern[ptrnBuffer].inst[t], LaneIndex(t, s));
    const boolean preT = bitRead(condPrevSounded, t);

    boolean pass = TRUE;
    switch (rule){
    case LINK_AND: pass =  nowT; break;
    case LINK_NOT: pass = !nowT; break;                  // this is selective choke
    case LINK_PRE: pass =  preT; break;
    case LINK_NPR: pass = !preT; break;
    }
    if (!pass) bitClear(linked, z);
  }
  m = linked;

  // The hat is fired by HH, which is not a lane anyone writes conditions on.
  // It sounds if either of the lanes feeding it was allowed to.
  if (bitRead(m, CH) || bitRead(m, OH)) bitSet(m, HH);
  else                                  bitClear(m, HH);

  // What actually sounds this step, for the next step's PRE rules. Allowed and
  // trigged, which is what "did it play" has to mean.
  unsigned int sounding = 0;
  for (byte z = 0; z < NBR_INST; z++){
    if (bitRead(m, z) && !LaneSilent(z) &&
        bitRead(pattern[ptrnBuffer].inst[z], LaneIndex(z, s))) bitSet(sounding, z);
  }
  condPrevSounded = sounding;

  return m;
}

//---------------------------------------------------------------- page

// Step 0 is the instrument as a whole; 1..16 are its steps.
static byte *CondByteAt()
{
  if (condStep == 0) return &pattern[ptrnBuffer].instCondProb[curInst];
  return &pattern[ptrnBuffer].stepCond[curInst][condStep - 1];
}

void CondEnter()
{
  condMode = TRUE;
  condPage = 0;
  condStep = 0;
  curIndex = 0;
  needLcdUpdate = TRUE;
}

// SHIFT+SCALE again moves to the links page, and once more leaves. Conditions
// and links are the same idea one level apart - "when may this play" - so they
// share a key rather than spending two.
void CondNextPage()
{
  if (condPage == 0){ condPage = 1; curIndex = 0; needLcdUpdate = TRUE; }
  else CondExit();
}

void CondExit()
{
  condMode = FALSE;
  needLcdUpdate = TRUE;
}

// [F21] Which voice this rule points at, as a voiceOrder index, or
// LINK_NO_TARGET. Kept as an index rather than an instrument number so the
// encoder walks the panel order like everywhere else.
static void LinkSetTarget(byte ti)
{
  byte *lk = &pattern[ptrnBuffer].instLink[curInst];
  *lk = (byte)((ti << 4) | (*lk & 0x0F));
  patternNeedSaved = TRUE;
}

static void LinkSetRule(byte rule)
{
  byte *lk = &pattern[ptrnBuffer].instLink[curInst];
  *lk = (byte)((*lk & 0xF0) | rule);
  patternNeedSaved = TRUE;
}

void LinkButtons()
{
  // Step buttons pick the source instrument, as on every other page.
  //
  // [6R] With or without INST. INST+pad means "name a voice" on every page in
  // this firmware and on this one it meant nothing at all, so walking here from
  // the conditions page - same key, one press apart - found the gesture missing.
  // Both spellings land on the same call.
  if (inStepPressed){
    // [4V] Naming a voice, so one pad per voice - hi-hats included.
    const byte i = InstSelectFromButtons(inStepPressed, inStepNow);
    if (i != 0xFF && i != curInst){ curInst = i; needLcdUpdate = TRUE; }
  }

  if (encBtn.justPressed){
    curIndex++;
    if (curIndex >= 3) curIndex = 0;        // source, target, rule
    needLcdUpdate = TRUE;
  }

  if (clearBtn.justPressed){
    UndoPush();
    pattern[ptrnBuffer].instLink[curInst] = (byte)(LINK_NO_TARGET << 4);
    patternNeedSaved = TRUE;
    needLcdUpdate = TRUE;
  }
}

void LinkEncoder()
{
  const byte before = pattern[ptrnBuffer].instLink[curInst];
  const byte beforeInst = curInst;

  // [6W] tgt rule src. The page opens on the target because that is the field the
  // pads cannot reach - a pad selects the source, so putting the source under the
  // opening cursor gave the knob the one job the panel already had covered.
  switch (curIndex){
  case 0: {
      byte ti = (byte)(before >> 4);
      if (ti > NBR_VOICE) ti = NBR_VOICE;            // NBR_VOICE reads as "none"
      ti = (byte)constrain(EncGet(ti, 1), 0, NBR_VOICE);
      LinkSetTarget(ti == NBR_VOICE ? LINK_NO_TARGET : ti);
    }
    break;
  // [6R] The rule is the decision - "only when", "only when not".
  case 1:
    LinkSetRule((byte)constrain(EncGet((byte)(before & 0x0F), 1), 0, LINK_MAX));
    break;
  case 2: {
      byte at = 0;
      for (byte i = 0; i < NBR_VOICE; i++) if (voiceOrder[i] == curInst){ at = i; break; }
      at = (byte)constrain(EncGet(at, 1), 0, NBR_VOICE - 1);
      curInst = voiceOrder[at];
    }
    break;
  }

  if (pattern[ptrnBuffer].instLink[curInst] != before || curInst != beforeInst) needLcdUpdate = TRUE;
}

void CondButtons()
{
  if (condPage == 1){ LinkButtons(); return; }

  // INST + step picks the instrument, step alone picks the step. INST + step
  // means "choose an instrument" everywhere else on the panel, so it means that
  // here too rather than this page having its own idea.
  if (inStepPressed){
    if (instBtn){
      const byte i = InstSelectFromButtons(inStepPressed, inStepNow);            // [4V]
      if (i != 0xFF && i != curInst){ curInst = i; needLcdUpdate = TRUE; }
    }
    else {
      for (byte a = 0; a < NBR_STEP; a++){
        if (bitRead(inStepPressed, a)){ condStep = (byte)(a + 1); needLcdUpdate = TRUE; break; }
      }
    }
  }

  if (encBtn.justPressed){
    curIndex++;
    if (curIndex >= MAX_CUR_POS) curIndex = 0;
    needLcdUpdate = TRUE;
  }

  // CLEAR takes the condition off whatever is selected, which is quicker than
  // winding two fields back to zero.
  if (clearBtn.justPressed){
    UndoPush();
    *CondByteAt() = 0;
    patternNeedSaved = TRUE;
    needLcdUpdate = TRUE;
  }
}

void CondEncoder()
{
  if (condPage == 1){ LinkEncoder(); return; }

  byte *b = CondByteAt();
  const byte before = *b;
  const byte beforeStep = condStep;
  const byte beforeInst = curInst;

  // [6R] prb cnd stp ins. The two values lead and the two addresses follow,
  // because both addresses are also on the pads - a bare pad is the step and
  // INST+pad the instrument - so the cursor is the only way to reach a value and
  // one of two ways to reach an address.
  switch (curIndex){
  case 0: {
      const byte p = (byte)constrain(EncGet((byte)(*b & 0x0F), 1), 0, 15);
      *b = (byte)((*b & 0xF0) | p);
    }
    break;
  case 1: {
      const byte c = (byte)constrain(EncGet((byte)(*b >> 4), 1), 0, COND_COUNT - 1);
      *b = (byte)((c << 4) | (*b & 0x0F));
    }
    break;
  case 2:
    condStep = (byte)constrain(EncGet(condStep, 1), 0, NBR_STEP);
    break;
  case 3: {
      // Walk the voice list by position, not by instrument number. Instrument
      // numbers are shift-register bit positions, so stepping through them
      // numerically both scrambles the panel order and has to hop over four
      // things that are not instruments - and a hop that only searches upward
      // wedges between CH and OH, which are the top two numbers.
      byte at = 0;
      for (byte i = 0; i < NBR_VOICE; i++) if (voiceOrder[i] == curInst){ at = i; break; }
      at = (byte)constrain(EncGet(at, 1), 0, NBR_VOICE - 1);
      curInst = voiceOrder[at];
    }
    break;
  }

  if (*CondByteAt() != before || condStep != beforeStep || curInst != beforeInst){
    if (curIndex == 0 || curIndex == 1) patternNeedSaved = TRUE;   // [6R] the two value fields
    needLcdUpdate = TRUE;
  }
}

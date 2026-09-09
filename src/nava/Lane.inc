
void LaneSyncPhase(byte z)
{
  const byte loop = LaneLoop(z);
  uint8_t sreg = SREG;
  cli();
  // Anchor to the step that will actually fire next, not the one sounding now.
  // The half-step precompute may already have armed the next step, and an armed
  // step fires without advancing the phase again - so anchoring to curStep left
  // the lane one step behind for as long as the length stayed set. That is the
  // shift on entering the page.
  const byte anchor = muxArmedForNext ? nextStepIndex : curStep;
  lanePhase[z] = loop ? (byte)(anchor % loop) : 0;
  SREG = sreg;
}

// Re-anchor every lane at once, for when the pattern underneath them changes.
// The phases running belong to the pattern being left, and the incoming one has
// its own lane lengths - so a polyrhythmic pattern handed its phases to the next
// one and everything sat wherever the old lane happened to have reached.
// [3X] Fold each phase into its span; do not re-anchor it to the master step.
//
// This took `curStep % loop`, which puts every lane back into step with the bar.
// On a free pattern change that runs on every change - and inside a chain, that
// is every bar - so a seven against sixteen was restarted on the downbeat each
// time round. A lane's phase being its own is the whole of what makes it a
// polyrhythm; anchoring it to the bar is the reset you hear.
//
// Folding keeps the lane running and only brings it inside a span that may have
// changed underneath it, which is all this was ever needed for. The bar-line
// swap still restarts the lanes outright, in Clock.inc, where restarting is
// correct: the bar itself has restarted there.
void LaneSyncAll()
{
  uint8_t sreg = SREG;
  cli();
  for (byte z = 0; z < NBR_INST; z++){
    const byte loop = LaneLoop(z);
    lanePhase[z] = loop ? (byte)(lanePhase[z] % loop) : 0;
  }
  SREG = sreg;
}

void LaneSetParam(byte inst, byte which, byte v)
{
  pattern[ptrnBuffer].lane[inst][which] = v;

  if ((which == LANE_SPAN || which == LANE_PAD) &&
      (inst == CH || inst == OH || inst == HH)){
    pattern[ptrnBuffer].lane[CH][which] = v;
    pattern[ptrnBuffer].lane[OH][which] = v;
    pattern[ptrnBuffer].lane[HH][which] = v;
  }

  // Length and padding both change the loop, so both need the phase re-anchored.
  if (which == LANE_SPAN || which == LANE_PAD){
    LaneSyncPhase(inst);
    if (inst == CH || inst == OH || inst == HH){
      LaneSyncPhase(CH);
      LaneSyncPhase(OH);
      LaneSyncPhase(HH);
    }
  }
  patternNeedSaved = TRUE;
  needLcdUpdate = TRUE;
}

// [3Z] The page reopens on the field you left it on. curIndex is shared by every
// page that has a cursor, so it has to be saved and restored rather than simply
// left alone - anything else visited in between would have moved it.
//
// Length was always the opening field before, so adjusting fill meant two encoder
// presses every time you came back to it - and coming back repeatedly is what
// this page is for.


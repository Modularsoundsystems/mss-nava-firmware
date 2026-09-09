//-------------------------------------------------
//                  NAVA v2.x
//            Unified input layer  [3A]
//
// One scan, one edge pass, many readers.
//
// v1 had four independent consumers of the same 16 step bits - InstValueGet(),
// the tap-record loop in Seq.inc, the keyboard loop in key.inc, and
// StepButtonGet(TOGGLE) - each keeping its own view of stepBtn[].prevState,
// .pressed and .counter. Whichever one ran first in a loop pass consumed the
// edge and the rest saw nothing. That is why chords were unreliable: the
// answer to "was this button just pressed?" depended on who asked first.
//
// The sharpest case was .pressed. It was *written* in exactly one place
// (key.inc) and *read* in two others. Press a step button in keyboard mode and
// stepBtn[].pressed latched HIGH - nothing outside keyboard mode ever cleared
// it, so that step silently stopped responding in step-write mode until
// keyboard mode was re-entered and the same button cycled. StepButtonGet's
// TOGGLE branch was worse still: a single file-scope `static unsigned int
// value` shared as the toggle accumulator by every caller. It happened to be
// harmless only because nothing ever called it.
//
// Here the scan happens once per pass and produces read-only results.
// Consumers ask questions - StepPressed(a), StepDown(a), StepHeld(a) - and
// never mutate. Adding a fifth consumer is now free, which is exactly what
// F2, F5, F6 and F13 need.
//-------------------------------------------------

////////////////////////Function//////////////////////

// Call once per scan, immediately after ScanDin(). Both event loops do this:
// the main loop via ButtonGet(), expander mode via ButtonGetExpander().
void InputScan()
{
  // Retire the previous chord one scan late, so the pass that observed
  // inChordDone got to read inChordMask and inChordPeak first.
  if (inChordDone){
    inChordDone = FALSE;
    inChordMask = 0;
    inChordPeak = 0;
  }

  inStepPrev = inStepNow;
  inStepNow  = ((unsigned int)dinSr[1] << 8) | dinSr[0];

  const unsigned int changed = inStepNow ^ inStepPrev;
  inStepPressed  = changed &  inStepNow;
  inStepReleased = changed & ~inStepNow;

  const unsigned int now = (unsigned int)millis();
  byte down = 0;

  for (byte a = 0; a < NBR_STEP_BTN; a++){
    if (bitRead(inStepNow, a)){
      down++;
      if (bitRead(inStepPressed, a)) inStepTime[a] = now;
      else if ((unsigned int)(now - inStepTime[a]) > HOLD_TIME) bitSet(inStepHeld, a);
    }
    else bitClear(inStepHeld, a);
  }
  inStepCount = down;

  if (inStepNow){
    inChordMask |= inStepNow;
    if (down > inChordPeak) inChordPeak = down;
  }
  else if (inChordMask) inChordDone = TRUE;
}

//return value of first pressed step button
// [3A] Sourced from the scan instead of re-reading dinSr[].
byte FirstBitOn()
{
  for (byte a = 0; a < NBR_BTN_STEP; a++){
    if (bitRead(inStepNow, a)) return a;
  }
  return 0;   // [B-14] was falling off the end of a non-void function. 0 matches
              // SecondBitOn()'s convention. Callers all gate on inStepNow being
              // non-zero first, so the sentinel question is still open - moving
              // to a real NO_STEP would touch fifteen call sites in Seq.inc and
              // belongs with the mode rework, not with a behaviour-preserving
              // refactor.
}

//return value of second pressed step button
byte SecondBitOn()
{
  byte counter = 0;
  for (byte a = 0; a < NBR_BTN_STEP; a++){
    if (bitRead(inStepNow, a)){
      if (++counter == 2) return a;
    }
  }
  return 0;
}

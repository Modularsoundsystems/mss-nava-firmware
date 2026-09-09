//-------------------------------------------------
//                  NAVA v1.x
//              Sequencer Button
//-------------------------------------------------

////////////////////////Function//////////////////////

void ButtonGet()
{
  ScanDin();
  InputScan();                                              // [3A] one scan, one edge pass
  byte firstByte = dinSr[2];
  byte secondByte = dinSr[3];
  byte thirdByte = dinSr[4];

  // [F28] INST and SHIFT are plain levels rather than Buttons, so they get their
  // own latch here. Folding the latch into the level itself means every existing
  // read of instBtn/shiftBtn honours it without knowing it exists - there is no
  // second spelling of "is SHIFT down" to keep in step.
  ButtonGet(&instLatchBtn,  firstByte & BTN_INST);
  ButtonGet(&shiftLatchBtn, firstByte & BTN_SHIFT);
  instBtn  = (firstByte & BTN_INST)  ? 1 : (instLatchBtn.latched  ? 1 : 0);

  shiftBtn = (firstByte & BTN_SHIFT) ? 1 : (shiftLatchBtn.latched ? 1 : 0);

  ButtonGet (&clearBtn, firstByte & BTN_CLEAR);
  ButtonGet (&trkBtn, secondByte & BTN_TRK);
  ButtonGet (&ptrnBtn, secondByte & BTN_PTRN);
  ButtonGet (&tapBtn, secondByte & BTN_TAP);
  ButtonGet (&dirBtn, secondByte & BTN_DIR);
  ButtonGet (&playBtn, firstByte & BTN_PLAY);
  ButtonGet (&stopBtn, firstByte & BTN_STOP);
  ButtonGet (&guideBtn, secondByte & BTN_GUIDE);
  ButtonGet(&scaleBtn, firstByte & BTN_SCALE);
  // [5Q] LASTSTEP is the octave-down key in keyboard mode, and two quick presses
  // down two octaves are indistinguishable from a double tap - which would latch
  // it, and a latched LASTSTEP turns every pad into a pattern-length control while
  // you are trying to type notes. Before the scan, so the latch is refused on the
  // press that would have set it rather than a pass later.
  // [6M] LASTSTEP does not latch at all now - see InitButtonCounter().
  ButtonGet(&lastStepBtn, firstByte & BTN_LASTSTEP);
  // [6U] The LASTSTEP+encoder gesture is over when the key comes up. Here rather
  // than in EncGet(), which returns early down several branches - a release that
  // happened to fall on one of those would have left the flag set and swallowed
  // the undo level for the next hold.
  if (!lastStepBtn.pressed) lastStepEncDirty = FALSE;
  ButtonGet(&shufBtn, firstByte & BTN_SHUF);
  ButtonGet(&backBtn, secondByte & BTN_BACK);
  ButtonGet(&fwdBtn, secondByte & BTN_FWD);
  ButtonGet(&tempoBtn, thirdByte & BTN_TEMPO);
  ButtonGet(&numBtn, secondByte & BTN_NUM);
  ButtonGet(&muteBtn, thirdByte & BTN_MUTE);
  ButtonGet(&bankBtn, thirdByte & BTN_BANK);

  // [5G] Copy and paste open on SHIFT plus their key and are then held by that
  // key alone. Below the muteBtn and bankBtn scans for the same reason fillHold
  // sits below tempoBtn: above them, this would read last pass's buttons.
  if (shiftBtn && bankBtn.pressed) copyHold = TRUE;
  else if (!bankBtn.pressed)       copyHold = FALSE;
  if (shiftBtn && muteBtn.pressed) pasteHold = TRUE;
  else if (!muteBtn.pressed)       pasteHold = FALSE;
  ButtonGet(&encBtn, encSwState);
  ButtonGet(&stepsBtn, inStepNow);
  ButtonGet(&enterBtn, thirdByte & BTN_ENTER);              // [3A] hold is generic now
}

//==========================================================================================
//==========================================================================================

//Init buttons state
void ButtonGet( Button *button, unsigned int pin)
{
  button->justRelease = 0;
  button->justPressed = 0;
  button->curState = ((pin) ? 1:0);
  if (button->curState != button->prevState){
    if (button->pressed == LOW && button->curState == HIGH){
      button->justPressed = HIGH;
      button->curTime = millis();                           // [3A]
      // [F28] Double-tap toggles the latch. A modifier that must be held
      // occupies a hand; latching it frees the other one, which is the whole
      // point of shortening a pattern live while still reaching the knobs.
      //
      // Opt-in, and deliberately so. A latch is a mode you can enter without
      // meaning to, and a double press inside 400 ms is an ordinary accident -
      // cycling SCALE, or correcting a mis-hit. Latching LASTSTEP silently
      // suppresses step edit and tap record and turns every step button into a
      // pattern-length control, so a button gets this only when something
      // actually reads its latch.
      // Double-tap to latch, single press to release. Asymmetric on purpose:
      // getting out of a mode should never need a gesture you have to time.
      if (button->canLatch){                                            // [F3]
        if (button->latched) button->latched = FALSE;
        else if ((button->curTime - button->tapTime) < DOUBLE_TAP_TIME) button->latched = TRUE;
      }
      button->tapTime = button->curTime;
    }
    if (button->pressed == HIGH && button->curState == LOW){
      button->justRelease = HIGH;
      button->hold = LOW;                                   // [3A]
    }
    button->pressed = button->curState;
  }
  button->prevState = button->curState;

  // [3A] hold used to be hand-written for enterBtn alone. Every button has it
  // now: F5, F13 and F25 are all hold-then-chord gestures, and a gesture
  // vocabulary that only one button speaks is not a vocabulary.
  if (button->pressed && (millis() - button->curTime) > HOLD_TIME) button->hold = HIGH;
}

// [3A] StepButtonGet() deleted. Its MOMENTARY branch is now inStepNow, and its
// TOGGLE branch was dead code carrying a file-scope `static unsigned int value`
// that every caller would have shared as one toggle accumulator.

// Cycle the trig/velocity state of each step button pressed on this scan.
// [3A] Reads inStepPressed instead of maintaining its own edge state, and the
// three-way state is a local: stepBtn[].counter was recomputed from `value`
// and the stored velocity on every call and never read across calls, so it was
// never actually state - just a global spelling of a temporary.
unsigned int InstValueGet(unsigned int value)
{
  if (!inStepPressed) return value;

  for (byte a = 0; a < NBR_STEP_BTN; a++){//a = step button number
    if (!bitRead(inStepPressed, a)) continue;
    patternWasEdited = TRUE;
    byte state;

    //two button state
    // [F7] Single-trigger mode makes every instrument behave like the ones that
    // were always two-state: one press on, one press off, always at full
    // velocity. Faster to enter a part live than cycling through weak.
    // [6Z] RIDE and CRASH are only two-state while the cymbal accent is off.
    // With it on they fall through to the three-state branch below and cycle
    // weak -> strong -> off like the drums, which is the whole of the feature on
    // the input side: they cannot flam, so that branch is the plain one.
    //
    // OH stays two-state whatever this says. Its low value is one count under its
    // high, the same as the cymbals had, and unlike them it shares a voice with CH
    // through the hat trigger - so a weak OH is a question about which lane wins,
    // not about how hard it was hit.
    if(seq.singleTrig || curInst == OH || curInst == TOTAL_ACC || curInst == TRIG_OUT || curInst == EXT_INST
       || ((curInst == RIDE || curInst == CRASH) && !seq.cymAccent)){
      state = bitRead(value,a) ? 2 : 1;
      switch (state){
      case 1:
        bitSet (value, a);
        pattern[ptrnBuffer].velocity[curInst][a] = instVelHigh[curInst];//HIGH_VEL;
        break;
      case 2:
        bitClear (value, a);
        break;
      }
      bitClear(pattern[ptrnBuffer].flamMask[curInst], a);                        // [3B]
    }
    //three button state flam
    // [B-32] ...and only on a voice that can actually flam. curFlam is cleared
    // by every non-flam instrument in the select switch, so this is a belt on
    // top of that brace - but bit 7 written here is indistinguishable from a
    // velocity later on, and the cost of being wrong is silent data damage.
    else if (curFlam && InstHasFlam(curInst)) {                                                              // [zabox] [1.027] flam
      if (bitRead(value,a)){
        //if vel > MID_VEL
        if (bitRead(pattern[ptrnBuffer].flamMask[curInst], a)) {                                        // [3B] own bitmask, not bit 7
          if ((pattern[ptrnBuffer].velocity[curInst][a]) > instVelLow[curInst]) state = 3;              // flam test, to be removed
          else state = 2;
        }
        else {
          state = 3;
        }
      }
      else {
        state = 1;
      }
      switch (state){
        //half velocity
      case 1:
        bitSet (value, a);
        pattern[ptrnBuffer].velocity[curInst][a] = instVelLow[curInst];
        bitSet(pattern[ptrnBuffer].flamMask[curInst], a);
        break;
        //full velocity
      case 2:
        pattern[ptrnBuffer].velocity[curInst][a] = instVelHigh[curInst];
        bitSet(pattern[ptrnBuffer].flamMask[curInst], a);
        break;
        //Off
      case 3:
        bitClear (value, a);
        bitClear(pattern[ptrnBuffer].flamMask[curInst], a);
        break;
      }
    }

    //three button state
    else {
      if (bitRead(value,a)){
        //if vel > MID_VEL
        if (!bitRead(pattern[ptrnBuffer].flamMask[curInst], a)) {                                                        // [3B] check if flam
          if ((pattern[ptrnBuffer].velocity[curInst][a]) > instVelLow[curInst]) state = 3;
          else state = 2;
        }
        else {
          state = 3;
        }
      }
      else {
        state = 1;
      }
      switch (state){
        //half velocity
      case 1:
        bitSet (value, a);
        pattern[ptrnBuffer].velocity[curInst][a] = instVelLow[curInst];
        break;
        //full velocity
      case 2:
        pattern[ptrnBuffer].velocity[curInst][a] = instVelHigh[curInst];
        break;
        //Off
      case 3:
        bitClear (value, a);
        break;
      }
      bitClear(pattern[ptrnBuffer].flamMask[curInst], a);                        // [3B] this branch is the non-flam one
    }

    // [3S] A step past the lane's span is written and kept, and the lane is left
    // exactly as long as it was.
    //
    // F27 grew the lane to reach it, on the reasoning that a step you cannot hear
    // is a step that did nothing. But a lane that lengthens itself whenever you
    // draw past the end is a lane whose length you cannot hold - and the length is
    // the whole of the polyrhythm. Writing past the span is now how you prepare a
    // longer loop before committing to it: the trigs wait there, silent, until
    // INST+LASTSTEP+pad says the lane is that long.
    //
    // Nothing is lost by waiting. The step LEDs show the trigs whether the lane
    // reaches them or not, so what is parked out there stays visible.
  }
  return value;
}


//Mute steps buttons -----------------------------------------------------------               [1.028] new version. faster, doens't need a button counter
// [3A] The muteButtons/lastMuteButtons shadow pair is gone - inStepPressed is
// the same edge, computed once. Safe to consume the edge directly because the
// two call sites can never both run in one scan: Expander() is a
// while(seq.sync == EXPANDER) loop that never returns to the main loop, so its
// MuteButtonGet() and the MUTE-mode one in SeqParameter() are on mutually
// exclusive paths, and each runs at most once per ScanDin().
void MuteButtonGet()
{
  if (!inStepPressed) return;                                                      // [1.028] only when a button is pressed (important for expander mode)

  for (byte a = 0; a < NBR_STEP_BTN; a++) {
    if (bitRead(inStepPressed, a)) {

      muteInst ^= (1 << muteOut[a]);                                             // [B-02] LEDs now derived, not shadowed

      if (seq.muteModeHH) {
        if (a == 12) {
          muteInst ^= (1 << muteOut[13]);
        }
        else if (a == 13) {
          muteInst ^= (1 << muteOut[12]);
        }
      }
    }
  }
}


//Mute steps buttons -----------------------------------------------------------               [1.028] gate
void GateButtonGet()
{
  if (!inStepPressed) return;                                                      // [1.028] only when a button is pressed (important for expander mode)

  for (byte a = 0; a < NBR_STEP_BTN; a++) {
    if (bitRead(inStepPressed, a)) {

      gateInst ^= (1 << muteOut[a]);                                             // [B-02] LEDs now derived, not shadowed

    }
  }
}


// [3A] FirstBitOn() and SecondBitOn() moved to Input.inc, next to the scan they
// read. The commented-out StepBtn() below them went with stepBtn[].

//Init buttons counter----------------------------------------------------
void InitButtonCounter()
{
  // [F28] The left-hand modifiers all latch. Each of them is a "hold this while
  // you do something else" key, which is exactly the gesture a latch frees a
  // hand from. Buttons whose press *does* something - PLAY, SCALE, the mode
  // keys - are left alone: latching those would mean repeating the action.
  // DIR is not here: it shifts the pattern on release, and a release that edits
  // the pattern cannot coexist with a double-tap latch - the first tap of the
  // pair is indistinguishable from an ordinary quick press at the moment it is
  // let go. GUIDE is not here either, for the same reason and because the
  // effects it now carries hold while it is held.
  // [6M] Not LASTSTEP. [5U] refused the latch in keyboard mode and [6K] kept it
  // off the pads everywhere else, which left a latch that did nothing but put
  // the encoder on the length and a letter in the corner - two states to reason
  // about for a convenience the key does not need. LASTSTEP+pad and
  // LASTSTEP+encoder are both held gestures now, and the key has one state.
  lastStepBtn.canLatch   = FALSE;
  clearBtn.canLatch      = TRUE;
  // [6T] Not SHUF either, and for the reason [6M] took the latch off LASTSTEP.
  // While SHUF is down pads 1-7 set the pattern shuffle and 9-16 the flam
  // interval, both absolutely and both audible on the next step - so a latch
  // nobody meant to set turns every pad press into a change of feel. Its other
  // job, the per-instrument swing on the encoder, is a held gesture like the rest
  // of the page.
  shufBtn.canLatch       = FALSE;
  guideBtn.canLatch      = FALSE;
  instLatchBtn.canLatch  = TRUE;
  shiftLatchBtn.canLatch = TRUE;
  stopBtn.counter = 0;
  guideBtn.counter = 0;
  scaleBtn.counter = 0;
  encBtn.counter = 0;
  muteBtn.counter = 0;
}

// [B-02] Removed: InitMuteBtnCounter() and the pre-[1.028] MuteButtonGet()
// were already dead — commented out, superseded, and operating on
// muteStepBtn[], which was itself commented out in define.h. Both still
// referenced muteLeds, which no longer exists now that LEDs are derived.


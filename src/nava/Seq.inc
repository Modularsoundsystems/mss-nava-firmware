//-------------------------------------------------
//                  NAVA v1.x
//                 SEQ Parameter
//-------------------------------------------------

/////////////////////Function//////////////////////
void SeqParameter()
{  
  //can not access config when isRunning
  // [3T][4E] Only the pages that must not be open while playing. Everything that
  // is a decision you make *while* playing - what a paste carries, when a pattern
  // change lands, how far a detent goes, what a tapped pad ignores - stays
  // reachable, because having to stop to reach them is why they get set once and
  // left alone. Sync, channels and the dump page still close on PLAY: one
  // reconfigures the clock under a running sequencer, the other blocks the panel
  // for the best part of a minute.
  if (isRunning && seq.configMode && !ConfPageLive(seq.configPage)){
    seq.configMode = FALSE;
    needLcdUpdate = TRUE;
  }

  // [5N] INST+ENTER names total accent and INST+STOP names the trigger output,
  // from PLAY as well as from step and tap. Neither key does its own job under
  // INST - the transport gate already excludes instBtn - so there is nothing to
  // collide with, and the step screen shows the name the moment you get there.
  if (curSeqMode == PTRN_PLAY && !PanelBusy()){
    if (instBtn && enterBtn.justPressed){ curInst = TOTAL_ACC; needLcdUpdate = TRUE; }
    if (instBtn && stopBtn.justPressed) { curInst = TRIG_OUT;  needLcdUpdate = TRUE; }
  }

  if (curSeqMode != MUTE) muteBtn.counter = 0;
  if (!seq.configMode) seq.configPage = 0;

  // [3F] Tap MUTE and it latches; keep it pressed and it does not. Letting go of
  // a held MUTE puts you back in the mode you came from, so punching a voice out
  // for a bar is one gesture instead of three - and mute mode stops being
  // somewhere you can be left standing without noticing.
  //
  // The flag is needed because ButtonGet() clears hold on the same scan it raises
  // justRelease, so by the time the release is visible the hold is not.
  //
  // Out here rather than beside the press that enters mute mode: that lives in
  // the SHIFT-released branch, and a release that happened to fall while SHIFT
  // was down would have been missed, leaving the flag set to fire on some later
  // tap instead.
  {
    static boolean muteWasHeld = FALSE;
    // [3J] 300 ms, not HOLD_TIME's full second. Punching a voice out for a bar
    // is the gesture this exists for, and a second of lead-in is longer than
    // most of the bars it would be used in.
    // [5G] Not while MUTE is holding a paste open. In mute mode the same key is
    // both the momentary-mute hold and the second half of SHIFT+PASTE, and
    // without this the paste would drop you out of mute mode as it landed.
    if (BtnHeldFor(&muteBtn, MOMENTARY_TIME) && curSeqMode == MUTE && !pasteHold)
      muteWasHeld = TRUE;
    if (muteBtn.justRelease){
      if (muteWasHeld && curSeqMode == MUTE){
        curSeqMode = prevSeqMode;
        muteBtn.counter = 0;
        needLcdUpdate = TRUE;
      }
      muteWasHeld = FALSE;
    }
  }

  const boolean panelWasBusy = PanelBusy();

  // [B-05] Finalise a buffer swap - main-loop work that used to sit inside the
  // Timer1 ISR.
  //
  // [3F] First thing in the pass, so everything below reads one truth about
  // which pattern is playing. It used to run last, after the bar-end commits,
  // and the ISR raises patternSwapped and trackPosNeedIncremante on the same bar
  // line - so a chain's commit advanced nextPattern to the pattern due next, and
  // then `curPattern = nextPattern` down here recorded *that* as the one already
  // playing. curPattern ran one ahead of the music from the first bar line of
  // every chain.
  //
  // Everything about a chain was computed from it, including the commit's own
  // `curPattern != nextPattern` test - so the chain's idea of where it began
  // slid by one each time round, which is a chain that sticks on one pattern or
  // plays one of them two or three times before moving on. The track modes had
  // the same reading a bar behind. [4Z] removed the other half of that, which
  // re-derived the chain's start from the record on every swap.
  if (patternSwapped)
  {
    patternSwapped = FALSE;
    // [5A] The pattern that was armed, not whatever nextPattern says now. In a
    // chain nextPattern moves at the same bar line this swap happened on, and
    // which of the two the loop sees first depends on where the interrupt fell -
    // so reading it here made the chain skip a pattern, repeat one, or open on
    // its second, depending on the interleave. Reported as all three.
    curPattern = swappedPattern;
    keybOct = DEFAULT_OCT;
    noteIndex = 0;
    InitMidiNoteOff();
    needLcdUpdate = TRUE;
  }

  // [3U] Commit the armed paste when the chord breaks - whichever of the two
  // keys comes up first. Outside the SHIFT block on purpose: releasing SHIFT
  // stops that block running, and a paste that only completed if you happened to
  // let go in the right order would be worse than one that fired too early.
  //
  // SeqConfiguration() has already run this pass, so the mask read here is the
  // one the last pad press left.
  // [5G] On MUTE alone now: releasing SHIFT is the middle of the gesture, not
  // the end of it. The mask you set after letting go of SHIFT is the mask the
  // paste carries, which is the whole point of holding it through.
  if (pasteArmed && !pasteHold){
    pasteArmed = FALSE;
    // [F4] Undo snapshots the live buffer. When a pattern change is queued the
    // paste lands in the back buffer instead, which undo cannot reach - so drop
    // the stack rather than leave it claiming to cover something it does not.
    if (!nextPatternReady) UndoPush();
    else                   UndoDiscard();
    PasteBufferToPattern(curPattern);
    MsgShow(MSG_PASTE, 0);              // again, so the confirmation is the mask used
    patternWasEdited = TRUE;
  }

  //-------------------Encoder button---------------------------
  if(encBtn.justPressed && !PanelBusy()){                                         // [F26][F25]
    curIndex++;
    if (curIndex >= MAX_CUR_POS)  curIndex = 0;
    needLcdUpdate = TRUE;
  }
  // [3D] Any transport or mode button leaves whatever page is open. A page you
  // can only leave by remembering which key opened it is a trap - and there are
  // six of them now. The press still does its normal job as well, so PLAY both
  // closes the page and starts the sequencer.
  //
  // Plain TAP and DIR are excluded: those shift the pattern, and closing a page
  // must not edit one. SHIFT+TAP is a mode change and is included.
  if (PanelBusy() &&
      (playBtn.justPressed || stopBtn.justPressed || ptrnBtn.justPressed ||
       trkBtn.justPressed  || muteBtn.justPressed || (tapBtn.justPressed && shiftBtn))){
    PanelClose();
  }

  // [F5] Instant instrument clear, in every mode. CLEAR+INST+pads wipes those
  // instruments outright - the cursor clear takes a note out, this takes the
  // part out, and waiting a bar for that is not "instant". StepModifierHeld()
  // keeps every mode's own step handling off these presses.
  // [3K] Applied when the pads come up, not as they go down.
  //
  // OH is the chord 13+14, and two pads never arrive in the same scan. Acting on
  // each press meant the first one named CH - wiping the lane you were reaching
  // past - and the second then named OH, so "delete the open hat" deleted the
  // closed hat as well and charged two undo levels for it. Sometimes: if the two
  // presses landed in one scan it behaved, which is what made it intermittent.
  //
  // The raw button bits are collected while the chord is held and the
  // instruments worked out once, from the whole of it, on release.
  static unsigned int instClearBtns = 0;
  const boolean instClearHeld = (BtnHeld(&clearBtn) && instBtn && !shiftBtn && !PanelBusy());

  if (instClearHeld){                                                  // [F3]
    instClearBtns |= inStepNow;
    tempMuteInst = InstSelectMask(inStepNow, inStepNow);   // [5H] silent while you choose
  }

  if (instClearBtns && (!instClearHeld || !inStepNow)){
    // [5H] Through the `oh ` setting, like every page that names a voice. This is
    // the gesture where naming the wrong hi-hat DESTROYS a lane, and it is where
    // [3K], [3L], [3M] and [4L] all came from: two pads never land in the same
    // scan, so under the chord the leading one names CH and the wipe lands on it
    // before the chord arrives. Under `2nd` there is no leading pad, so the whole
    // family cannot happen. Under `BTH` this returns exactly what it always did,
    // and the mitigations below - collect while held, apply on release - are
    // still load-bearing.
    unsigned int w = InstSelectMask(instClearBtns, instClearBtns);

    // [3K] The hi-hat lanes travel together here when `mte` says they do, the
    // same as a single mute and as solo. Deleting a lane and silencing one are
    // the same question asked of the same pair of pads, and answering them
    // differently is how you end up deleting a lane you meant to keep.
    if (seq.muteModeHH && (w & ((1U << CH) | (1U << OH)))){
      w |= (1U << CH) | (1U << OH);
    }

    UndoPush();                                      // [F4] one level, one gesture
    for (byte i = 0; i < NBR_INST; i++){
      if (!bitRead(w, i)) continue;
      InstClear(i);
      for (byte s = 0; s < NBR_STEP; s++) pattern[ptrnBuffer].velocity[i][s] = instVelLow[i];
    }
    SetHHPattern();
    patternWasEdited = TRUE;
    needLcdUpdate = TRUE;
    instClearBtns = 0;
  }

  // [6J] A transient mute exists only while CLEAR is down. Asserted here, in
  // every mode.
  //
  // [B-02] defines tempMuteInst as "transient mutes held only while CLEAR is
  // down", and three places set it: the block above, the cursor erase, and the
  // tap-mode step erase. The two that clear it again both live INSIDE the
  // `curSeqMode == PTRN_STEP || PTRN_TAP` block - but the block above is
  // deliberately mode-independent, because [F5] says instant instrument clear
  // works in every mode.
  //
  // So CLEAR+INST+pads in mute mode, in play mode or in either track mode set a
  // mute that nothing would ever take off. The instruments named simply stopped
  // playing, permanently, and invisibly: only the mute page draws ActiveMute(),
  // so anywhere else there is no indication at all - not on the pads, not on the
  // screen, and not in the pattern, which is intact. Reloading the pattern would
  // not fix it and neither would saving it.
  //
  // One line, and it is an invariant rather than another reset site: whatever set
  // this, it has no business outliving the key it belongs to. Placed above the
  // two erases, which set it later in the same pass while CLEAR is still held.
  if (!BtnHeld(&clearBtn)) tempMuteInst = 0;

  if ((playBtn.justPressed && !shiftBtn && !instBtn) || midiStart){                  // [F9][4U][6V][6X]
    isRunning = TRUE;
    isStop = FALSE;
    ppqn = 0;
    stepCount = 0;
    tapStepCount = 0;
    rollPhase = 0;                   // [3X] the roll grid starts with the transport
    changeDir = 1;//restart Forward
    stopBtn.counter = 0;
    shufPolarity = 0;//Init shuffle polarity
    swingPos = SWING_SLOTS;                                                        // [F19] drop any group still pending
    for (byte z = 0; z < NBR_INST; z++) lanePhase[z] = 0;                          // [F27]
    lanePhaseFresh = TRUE;
    // [B-07] Anything armed belongs to the phase we just abandoned. Without
    // this the first step after PLAY fires the stale precomputed index.
    muxArmedForNext = FALSE;
    noteIndexCpt = 0;//init ext instrument note index counter
    blinkTempo = 0;                                                               // [zabox] looks more consistent
    //trackPosNeedIncremante = TRUE;
    //init Groupe pattern  position
    /*group.pos = 0;
     nextPattern = group.firstPattern + group.pos;
     if(PlayingPattern() != nextPattern) selectedPatternChanged = TRUE;
     trackPosNeedIncremante = FALSE;
     needLcdUpdate = TRUE;*/

    MIDI.sendRealTime(Start);  //;MidiSend(START_CMD);
      DIN_START_HIGH;
      dinStartState = HIGH;
//    DIN_CLK_HIGH;                                                               // [1.028] redundant
//    dinClkState = HIGH;
  }

  //-------------------stop button------------------------------
  // [4H] Not under SHIFT - that chord is the cyclic record toggle, and a key
  // that armed a recording mode and stopped the sequencer in one press would be
  // unusable for either.
  // [6X] Same rule as PLAY above: held or latched, SHIFT and INST both own this
  // key rather than sharing it with the transport.
  if ((stopBtn.justPressed && !instBtn && !shiftBtn) || midiStop || midiContinue){   // [4U][6V][6X]
    //Init Midi note off
    SendAllNoteOff();//InitMidiNoteOff();
    if (midiStop) stopBtn.counter = 0;
    else if (midiContinue) stopBtn.counter = 1;
    stopBtn.counter++;
    switch (stopBtn.counter){
    case 1:
      isStop = TRUE;
      isRunning = FALSE;
      rollMask = 0;                               // [F11] no roll outlives the transport
      swingPos = SWING_SLOTS;                     // [F19] nor a swung group still in the air
      muxArmedForNext = FALSE;                    // [B-07] discard the armed step
      MIDI.sendRealTime(Stop);//;MidiSend(STOP_CMD);
      DIN_START_LOW;
      dinStartState = LOW;
      break;
    case 2:
      isStop = FALSE;
      isRunning = TRUE;
      stopBtn.counter = 0;
      ppqn = 0;
      // [B-03] Re-establish the counters that are phase-locked to ppqn.
      // PLAY sets ppqn/stepCount/tapStepCount/shufPolarity all to 0 together;
      // CONTINUE reset only ppqn, leaving tapStepCount half a step ahead of a
      // fresh phase — every tap then landed one step late until the next PLAY.
      // Invariant at ppqn==0: tapStepCount == stepCount, shufPolarity == stepCount & 1.
      tapStepCount = stepCount;
      shufPolarity = stepCount & 1;
      // [F27] The lane phases are free-running and have no defined relation to
      // stepCount, so a resume cannot reconstruct them - restart them instead.
      for (byte z = 0; z < NBR_INST; z++) lanePhase[z] = 0;
      lanePhaseFresh = TRUE;
      // [B-07] Force a fresh precompute at the resumed ppqn==0 boundary rather
      // than firing whatever was armed when playback stopped.
      muxArmedForNext = FALSE;
      MIDI.sendRealTime(Continue);//MidiSend(CONTINU_CMD);
      DIN_START_HIGH;
      dinStartState = HIGH;
      break;
    }
  }

  if (guideBtn.justPressed){
    guideChordUsed = FALSE;
    guidePending = 0;
  }


  if (guideBtn.pressed
      && (shiftBtn || instBtn || BtnHeldFor(&guideBtn, MOMENTARY_TIME))) guideChordUsed = TRUE;

  if (guideBtn.justRelease && !guideChordUsed && !shiftBtn){        // [6X]
    guidePending = millis() | 1;                   // never zero: that means idle
  }
  if (guidePending && !guideBtn.pressed && (millis() - guidePending) > DOUBLE_TAP_TIME){
    guidePending = 0;
  }

  // [3C] INST opens pages; SHIFT means the orange word printed under the key.


  if (guideBtn.justPressed && instBtn && !shiftBtn){
    if (condMode) CondNextPage();          // conditions -> links -> out
    else { PanelClose(); CondEnter(); }
    // This GUIDE press has been spent. Without saying so its release would fall
    // through to the metronome, which waits out the double-tap window and then
    // toggles - a page opening and the metronome coming on together.
    guideChordUsed = TRUE;
    needLcdUpdate = TRUE;
  }

  //-------------------Shift button pressed------------------------------
  if (shiftBtn){
    if (trkBtn.justPressed){
      needLcdUpdate = TRUE;
      curSeqMode = TRACK_WRITE;
      keyboardMode = FALSE;
      seq.configMode  = FALSE;
    }
    if (ptrnBtn.justPressed) {
      needLcdUpdate = TRUE;
      curSeqMode = PTRN_STEP;
      seq.configMode  = FALSE;
      trackNeedSaved = FALSE;
    }
    if (tapBtn.justPressed){
      curSeqMode = PTRN_TAP;
      // [6P] This chord has spent the key. A bare TAP shifts the pattern left on
      // its RELEASE, guarded only by SHIFT not being down at that instant - so
      // letting go of SHIFT before TAP entered tap mode AND shifted the pattern.
      // SHIFT is bottom-left and TAP is across the panel, which is exactly the
      // reach where the hands do not come off in a defined order. Same mechanism
      // [F8] already uses for the insert and delete chords.
      tapChordUsed = TRUE;
      needLcdUpdate = TRUE;
      keyboardMode = FALSE;
      seq.configMode  = FALSE;
      trackNeedSaved = FALSE;
    }
    if (bankBtn.justPressed){
      CopyPatternToBuffer(curPattern);
      MsgShow(MSG_COPY, 0);             // [3D] a copy is invisible; say it, and what it carries
    } // copy current pattern to the buffer

    // [3C] SHIFT+SCALE and SHIFT+LAST STEP are +OCT and -OCT on the front panel
    // and now mean that. An octave only exists in keyboard mode, so outside it
    // they do nothing rather than pretending to.
    if (keyboardMode){
      if (scaleBtn.justPressed    && keybOct < MAX_OCT - 1){ keybOct++; needLcdUpdate = TRUE; }
      if (lastStepBtn.justPressed && keybOct > 0)          { keybOct--; needLcdUpdate = TRUE; }
    }

    // [F9] Record arm. Tapping always sounds; this decides whether it is also
    // written. Classic transport idiom, and it keeps "play the part" and
    // "commit the part" as two separate decisions.
    // [6X] Held or latched. The transport gate above now declines the press in
    // both cases, so this is the only thing the key does under SHIFT - which is
    // what a latch is for. It was ShiftPhysical() while a latched SHIFT still let
    // PLAY start the sequencer, to stop one press doing two jobs.
    if (playBtn.justPressed){
      tapRecordArm = !tapRecordArm;
      needLcdUpdate = TRUE;
    }

    // [4H] SHIFT+STOP: cyclic record on or off. One deliberate chord rather than
    // a timed gesture, and on a key whose own job - stopping - is still one
    // unmodified press away. The stop handler declines the press when SHIFT is
    // down, so the chord cannot arm the mode and halt the take in the same
    // motion.
    if (stopBtn.justPressed){                                        // [6X] held or latched
      cycRec = !cycRec;
      cycTouched  = 0;
      cycHatSaved = FALSE;
      needLcdUpdate = TRUE;
    }

    // [F26] SHIFT+ENTER names whatever is selected - the pattern in pattern
    // modes, the track in track modes.
    if (enterBtn.justPressed && !seq.configMode){
      if (nameEditMode) NameEditExit();
      else              NameEditEnter();
    }

    // [F4] Undo. SHIFT+CLEAR was free and reads exactly right; the three plain
    // CLEAR actions above now exclude shiftBtn so they cannot fire alongside it.
    // Pattern modes only. In the track modes CLEAR already means "rewind the
    // track", and undo restores pattern content, so firing both from one press
    // would be two unrelated things at once.
    // [6S] ...and INST added to it is redo. All three keys are in the one 2x3
    // block under the left hand - CLEAR above SHIFT, INST beside it - so the pair
    // is a single grip with one finger moved, which is the right shape for two
    // gestures that walk the same list in opposite directions.
    //
    // Free to take: CLEAR+INST+pads is the instant instrument clear and it
    // requires !shiftBtn, so nothing else answers to these three together.
    if (clearBtn.justPressed && curSeqMode != TRACK_PLAY && curSeqMode != TRACK_WRITE){
      if (instBtn){
        if (RedoPop()) MsgShow(MSG_REDO, redoDepth);
        else           MsgShow(MSG_NOREDO, 0);
      }
      else {
        if (UndoPop()) MsgShow(MSG_UNDO, undoDepth);
        else           MsgShow(MSG_NOUNDO, 0);
      }
    }
    // [3U] The press arms the paste and puts the mask on screen; the release
    // performs it. 3T made the mask editable while this chord is held, and firing
    // on the press applied whatever it was *before* you looked at it - so the
    // message showing you what a paste carries was showing you a decision already
    // taken. One paste per gesture, with the mask as it stands when you let go.
    if (muteBtn.justPressed){
      pasteArmed = TRUE;
      MsgShow(MSG_PASTE, 0);            // [3D] and what it will bring across
    } //paste copy buffered pattern to the current pattern number

    //sequencer configuration page
    // [3T] Open while running too, but only onto the pages that are safe there -
    // the cycler steps over the rest rather than showing a page it would have to
    // close again.
    if (tempoBtn.justPressed){
      seq.configMode  = TRUE;
      curIndex = 0;
      do {
        seq.configPage++;
        if (seq.configPage > MAX_CONF_PAGE){
          seq.configPage = 0;
          seq.configMode  = FALSE;
          break;
        }
      } while (isRunning && !ConfPageLive(seq.configPage));
      needLcdUpdate = TRUE;
    }
  }
  //-------------------Shift button released------------------------------
  else {
    if (trkBtn.justPressed){
      curSeqMode = TRACK_PLAY;
      needLcdUpdate = TRUE;
      keyboardMode = FALSE;
      seq.configMode  = FALSE;
    }
    // if (backBtn.justPressed) ;//back  track postion
    //if (fwdBtn.justPressed) ;//foward track postion
    if (numBtn.pressed) ;//select Track number
    // [F26] INST + NUM opens the browser - the list of patterns by name, or of
    // tracks in a track mode. Same shape as INST + PTRN below: INST plus the key
    // that names the thing opens the page about that thing.
    if (numBtn.justPressed && instBtn){                                 // [F3]
      if (browseMode) BrowseExit();
      else { PanelClose(); BrowseEnter(); }
    }

    // [6Y] Pattern modes only - PLAY or STEP - and not on a config page.
    //
    // It ran in every mode: in tap mode, where CLEAR is already the live erase
    // and PTRN is a mode key you press on the way somewhere; in mute mode,
    // where you are performing; and in the track modes. Wiping a pattern is an
    // editing act, so it belongs where editing happens, and PLAY is included
    // because choosing a pattern to replace is where you decide to replace it.
    //
    // Instant instrument clear stays available everywhere - that gesture names
    // exactly what it destroys, on the pads, while you hold it.
    // [3E] CLEAR+PTRN clears the whole pattern. It used to be CLEAR+PLAY, and -
    // worse - a bare CLEAR with the sequencer stopped, which is a single
    // unmodified press that erases a pattern. PTRN is the key that already means
    // "the pattern", it is not a transport, and the pair reads as what it does.
    //
    // Physical CLEAR, not BtnHeld(): [F28]'s rule is that a latch may arm a
    // destructive chord but must not sustain a destructive act, and this is the
    // company PLAY used to keep. Not while a page was open either - closing one
    // and wiping a pattern on the same press is two answers to one question.
    //
    // First in the chain, so the same press cannot also toggle the mode.
    if (ptrnBtn.justPressed && clearBtn.pressed && !shiftBtn && !instBtn && !panelWasBusy
        && !seq.configMode                                                     // [6Y]
        && (curSeqMode == PTRN_PLAY || curSeqMode == PTRN_STEP)){
      ClearWholePattern();
    }
    // [F25] Held INST + PTRN opens the pattern utilities. INST is otherwise
    // unused with PTRN, and it keeps CLEAR - which is destructive on its own -
    // out of the way of a page whose whole job is destructive.
    else if (ptrnBtn.justPressed && instBtn){                           // [F3]
      utilMode = TRUE;
      curIndex = 0;
      needLcdUpdate = TRUE;
    }
    else if (ptrnBtn.justPressed){
      if (curSeqMode == PTRN_STEP) curSeqMode = PTRN_PLAY;
      else curSeqMode = PTRN_STEP;
      needLcdUpdate = TRUE;
      keyboardMode = FALSE;
      seq.configMode  = FALSE;
      trackNeedSaved = FALSE;
    }
    // [F8] TAP and DIR carry insert and delete now, on top of the shifts they
    // already do. They are the two keys that mean "move the pattern left" and
    // "move the pattern right", so a pad on top of them saying *where* is the
    // same sentence with an object added. << and >> keep the same binding below
    // for panels that have them - nothing on this one appears to answer to them.
    //
    // Both shift on release and only when no pad joined the hold, so reaching
    // for an insert cannot shift on the way in.
    if (tapBtn.justPressed) tapChordUsed = FALSE;
    if (tapBtn.justRelease && !tapChordUsed){ UndoPush(); ShiftLeftPattern(); }        // [F4]

    if (dirBtn.justPressed) dirChordUsed = FALSE;
    if (dirBtn.justRelease && !dirChordUsed){ UndoPush(); ShiftRightPattern(); }       // [F4][F8]
    // [F8] The same shift on << and >>, which is where a hand looks for it.
    // Both honour INST for a single instrument, exactly as TAP/DIR do - they
    // call the same functions, so there is no second behaviour to keep in step.
    // [5N] Not in keyboard mode. It runs inside PTRN_STEP and takes << and >> for
    // walking the note stack, so every release of those also shifted the whole
    // pattern sideways - one gesture doing two unrelated things, one of which you
    // could not see from the page you were on. The pads are notes there too, so
    // the insert and delete below have nothing to act on either.
    if ((curSeqMode == PTRN_STEP || curSeqMode == PTRN_TAP) && !keyboardMode){
      // [F8] With a pad, these insert and delete instead of shifting. << pulls
      // the later steps left over the one you pressed; >> pushes them right and
      // leaves a blank in its place. The arrows already mean those directions
      // and the pad says where - the same hold-and-point the rest of the panel
      // uses. INST narrows it to the selected lane, exactly as it does for the
      // plain shifts.
      //
      // The whole-pattern shift moved to release for this, and fires only when
      // no pad joined the hold: reaching for an insert must not shift on the way
      // in. Same shape TAP and DIR have.
      if (backBtn.justPressed) backChordUsed = FALSE;
      if (fwdBtn.justPressed)  fwdChordUsed  = FALSE;

      const boolean cutLeft  = (backBtn.pressed || tapBtn.pressed);
      const boolean cutRight = (fwdBtn.pressed  || dirBtn.pressed);

      if (inStepPressed && (cutLeft || cutRight)){                      // [F3]
        const byte at = FirstBitOn();
        const boolean del = cutLeft;
        UndoPush();                                                                    // [F4]
        if (instBtn){
          if (del) StepDeleteLane(curInst, at); else StepInsertLane(curInst, at);
        }
        else {
          for (byte z = 0; z < NBR_INST; z++){
            if (del) StepDeleteLane(z, at); else StepInsertLane(z, at);
          }
          SetHHPattern();                       // the hat lane is derived from CH/OH
        }
        if (del){ backChordUsed = TRUE; tapChordUsed = TRUE; }
        else    { fwdChordUsed  = TRUE; dirChordUsed = TRUE; }
        patternWasEdited = TRUE;
        needLcdUpdate = TRUE;
      }

      if (backBtn.justRelease && !backChordUsed){ UndoPush(); ShiftLeftPattern(); }
      if (fwdBtn.justRelease  && !fwdChordUsed) { UndoPush(); ShiftRightPattern(); }
    }

    //if (bankBtn && inStepNow) curBank = FirstBitOn();
    // [F16] Solo. Toggling the same instrument again, or soloing another, is the
    // whole interface - there is no separate exit.
    //
    // [3C] Moved from INST+MUTE to CLEAR+MUTE, trading places with the mute
    // group. INST means "which instruments" everywhere else on the panel and a
    // group mute is a list of instruments; solo is the thing that takes
    // everything else away, which is the shape CLEAR already has. SHIFT+MUTE is
    // paste and neither of them may touch it.
    if (muteBtn.justPressed && BtnHeld(&clearBtn) && !instBtn){
      // The selected voice, from any mode. In mute mode CLEAR + a pad names the
      // voice directly; this is the version for when you are somewhere else and
      // the instrument you want is already selected. Same mask either way.
      soloInst ^= (1U << curInst);
      needLcdUpdate = TRUE;
    }
    else if (muteBtn.justPressed){
      muteBtn.counter++;
      switch (muteBtn.counter){
      case 1:
        prevSeqMode = curSeqMode;
        curSeqMode = MUTE; //paste copy buffered pattern to the current pattern number
        break;
      case 2:
        curSeqMode = prevSeqMode;
        muteBtn.counter = 0;
        break;
      }
      needLcdUpdate = TRUE;
    }

    if (tempoBtn.justPressed && seq.sync == MASTER && !seq.configMode){
      const unsigned long now = millis();
      const unsigned long gap = now - tapTempoLast;
      tapTempoLast = now;

      // [4Q] Start again on a gap that is not part of this tempo: too long to be
      // keeping time at all, or far enough from the running average to be a new
      // tempo rather than a sloppy tap. The second test is what lets you tap
      // straight from one tempo to a much faster one - averaged in instead, the
      // answer crawls between the two and never quite arrives, which is exactly
      // what "it will not go above X" feels like from the front panel.
      boolean restart = (gap > TAP_TEMPO_MAX);
      if (!restart && tapTempoCount){
        const unsigned long avg = tapTempoSum / tapTempoCount;
        const unsigned long d   = (gap > avg) ? (gap - avg) : (avg - gap);
        if (d * 8 > avg * TAP_TEMPO_JUMP) restart = TRUE;
      }
      if (restart){
        // Keep this gap rather than throwing it away: it is the first interval
        // of the new tempo, and discarding it costs a whole extra tap.
        if (gap >= TAP_TEMPO_MIN && gap <= TAP_TEMPO_MAX){
          tapTempoSum = gap;
          tapTempoCount = 1;
        }
        else {
          tapTempoSum = 0;
          tapTempoCount = 0;
        }
      }
      else if (gap >= TAP_TEMPO_MIN){
        tapTempoSum += gap;
        tapTempoCount++;
        // Three intervals - four taps - before acting. Two is enough
        // arithmetically and not enough to tell a tempo from a stumble.
        if (tapTempoCount >= 3){
          // [F30] To a tenth, because there is now somewhere to put one - four
          // taps rarely land on a whole number and rounding to one threw away
          // most of what the hand actually gave.
          // [4Q] Rounded, both times. Truncating the interval and then truncating
          // the division threw away up to a whole tenth for free, on a figure
          // the taps were only ever going to get so close to anyway.
          const unsigned long iv = (tapTempoSum + tapTempoCount / 2) / tapTempoCount;
          unsigned int b10 = (unsigned int)((600000UL + iv / 2) / iv);
          b10 = constrain(b10, (unsigned int)(MIN_BPM * 10), (unsigned int)(MAX_BPM * 10));
          seq.bpm     = b10 / 10;
          seq.bpmFine = (byte)(b10 % 10);
          TimerSetFrequency();
          needLcdUpdate = TRUE;
        }
        // Roll the window rather than averaging forever, so a drifting tempo
        // is followed instead of anchored to the first few taps.
        if (tapTempoCount >= 8){
          tapTempoSum = tapTempoSum / tapTempoCount;
          tapTempoCount = 1;
        }
      }
    }

    if (curSeqMode != MUTE) muteBtn.counter = 0;
    if (!seq.configMode) seq.configPage = 0;
    if (tempoBtn.justRelease) needLcdUpdate = TRUE;
  }

  //==============================================================================
  ////////////////////////////// NAME EDIT [F26] /////////////////////////////

  // Takes the step buttons, CLEAR and BACK/FWD for as long as it is open, so
  // the mode blocks below are gated on it rather than acting on the same
  // presses. ENTER alone leaves - the same key that opened it with SHIFT held,
  // and the press that leaves also commits, since patternNeedSaved is set by
  // the edit and ENTER is already what saves.
  //
  // Deliberately NOT an early return out of SeqParameter(): the pattern-change
  // and buffer-swap housekeeping lives at the end of this function, and a
  // queued pattern change stalling because a name editor is open would desync
  // curPattern from the buffer the ISR is already playing.
  if (nameEditMode){
    NameEditButtons();
    if (enterBtn.justPressed && !shiftBtn) NameEditExit();
  }

  //////////////////////////// PATTERN UTILITIES [F25] ///////////////////////
  if (utilMode) UtilButtons();


  ////////////////////////// TRIG CONDITIONS [F18] ///////////////////////////
  if (condMode) CondButtons();

  {
    static boolean panelWasBusy = FALSE;
    const boolean busyNow = PanelBusy();
    if (panelWasBusy && !busyNow) stepLockout = TRUE;
    panelWasBusy = busyNow;
    if (stepLockout && !inStepNow) stepLockout = FALSE;   // released: hand it back
  }

  //==============================================================================
  //////////////////////////MODE PATTERN EDIT/////////////////////////////////

  // [3C] A pattern or track scrolled on the encoder commits when the key comes
  // up. Loading per detent would be a 30-50 ms EEprom read per click; letting go
  // is also the moment the hand has finished choosing.
  if (bankBtn.justRelease && encPtrnArmed && !shiftBtn){
    encPtrnArmed = FALSE;
    if (encPtrnScroll != PlayingPattern()){
      // [5F] Naming one pattern ends the chain, exactly as a single pad press
      // does. Without this the chain was still running underneath: the scrolled-to
      // pattern played its bar and the next bar-end advance overwrote nextPattern
      // with the chain's next member, so it appeared once and the chain resumed.
      //
      // Two ways to choose one pattern and only one of them said so - the pads
      // clear the chain in their own branch, and this path was written beside the
      // scroll rather than beside them, so it inherited nothing.
      group.length = 0;
      group.pos    = 0;
      nextPattern = encPtrnScroll;
      curBank = (byte)(encPtrnScroll / NBR_PATTERN);
      selectedPatternChanged = TRUE;
      needLcdUpdate = TRUE;
    }
  }
  // [3C] The scroll overlays appear and disappear with the key, so both edges
  // need a redraw - otherwise the last one drawn stays on screen.
  if (bankBtn.justPressed || bankBtn.justRelease
   || trkBtn.justPressed  || trkBtn.justRelease) needLcdUpdate = TRUE;
  if (trkBtn.justRelease && encTrkArmed && !shiftBtn){
    encTrkArmed = FALSE;
    if (encTrkScroll != trk.current){
      trk.next = encTrkScroll;
      selectedTrackChanged = TRUE;
      trackChangeManual = TRUE;                                                    // [3C]
      needLcdUpdate = TRUE;
    }
  }

  // [F26] The browser owns the panel while it is open, so its keys are read
  // here rather than inside a mode block - it is reachable from every mode and
  // means the same thing in all of them.
  if (browseMode) BrowseButtons();

  // [F11] A roll cannot survive its own handler being skipped. The tap block
  // below recomputes rollMask from the held pads every scan, so releasing a pad
  // stops the roll - but leaving tap mode or opening a page skips the block
  // entirely, and a roll left running with nothing reading the pads would never
  // stop. One place, covering every way out.
  if (curSeqMode != PTRN_TAP || PanelBusy()) rollMask = 0;

  if ((curSeqMode == PTRN_STEP || curSeqMode == PTRN_TAP) && !PanelBusy()){           // [F26][F25]

    static boolean curInstChanged;//flag that curInstchanged to not update LCD more than one tiem

    //-------------------Select instrument------------------------------
    //Match with trig shift register out (cf schematic)
    if(inStepNow == 0) doublePush = 0; //init double push if all step button  released 

    // [F8] << and >> claim the pads while they are down, and with INST they mean
    // "this lane only" - so the pad is saying where to cut, not which instrument
    // to select. Without this the same press would try to do both.
    if (instBtn && inStepNow && !lastStepBtn.pressed && !CutKeyHeld()){          // [zabox] [1.027] added flam [F27]
      curInstChanged = TRUE;
      keyboardMode = FALSE;
      switch(FirstBitOn()){
      case BD_BTN:
      case BD_LOW_BTN:
        curInst = BD;
        if (doublePush == 0){
          curFlam = 0;
        }
        break;
      case SD_BTN:
      case SD_LOW_BTN:
        curInst = SD;
        if (doublePush == 0){
          curFlam = 0;
        }
        break;
      case LT_BTN:
      case LT_LOW_BTN:
        curInst = LT;
        if (doublePush == 0){
          curFlam = 0;
        }
        break;
      case MT_BTN:
      case MT_LOW_BTN:
        curInst = MT;
        if (doublePush == 0){
          curFlam = 0;
        }
        break;
      case HT_BTN:
      case HT_LOW_BTN:
        curInst = HT;
        if (doublePush == 0){
          curFlam = 0;
        }
        break;
      case RM_BTN:
        curInst = RM;
        curFlam = 0;
        break;
      case HC_BTN:
        curInst = HC;
        curFlam = 0;
        break;
      // [4V][4W] INST is held here, so nothing is being played - the pads are a
      // list of voices to point at. Under `oh 2nd` each hat pad is its own lane;
      // under BTH either pad is the closed hat and the chord below names the
      // open one, the way it always did.
      case CH_BTN:
        curFlam = 0;
        curInst = CH;
        break;
      case CH_LOW_BTN:
        curFlam = 0;
        curInst = (seq.ohSelect == OH_SEL_2ND) ? OH : CH;
        break;
      case RIDE_BTN:
        curInst = RIDE;
        curFlam = 0;
        break;
      case CRASH_BTN:
        curInst = CRASH;
        curFlam = 0;
        break;
      }
      //if (inStepNow == OH_BTN){
      //  curInst = OH;
      //  doublePush = 1;
      //}
      switch (inStepNow) {
        case BD_F_BTN:
          curFlam = 1;
          doublePush = 1;
        break;
        case SD_F_BTN:
          curFlam = 1;
          doublePush = 1;
        break;
        case LT_F_BTN:
          curFlam = 1;
          doublePush = 1;
        break;
        case MT_F_BTN:
          curFlam = 1;
          doublePush = 1;
        break;
        case HT_F_BTN:
          curFlam = 1;
          doublePush = 1;
        break;
        // [4W] The chord still names OH under BTH. Under 2nd it is a no-op, so
        // holding both pads lands somewhere defined - whichever was read first.
        case OH_BTN:
          if (seq.ohSelect == OH_SEL_BOTH) curInst = OH;
          doublePush = 1;
        break;
      }
        
        
    }
    if (curInstChanged && stepsBtn.justRelease){
      needLcdUpdate = TRUE;
      curInstChanged = FALSE;
    }
    // [5N] Also reachable from PLAY - see the copy of these below the mode
    // blocks. Naming a lane is not a step-mode idea, and having to change mode
    // first is a rule nothing else on the panel follows.
    if (instBtn && enterBtn.justPressed){
      curInst = TOTAL_ACC;
      needLcdUpdate = TRUE;
    }
    if (instBtn && stopBtn.justPressed){
      curInst = TRIG_OUT;
      needLcdUpdate = TRUE;
    }
    if (shiftBtn && guideBtn.justPressed){
      curInst = EXT_INST;
      needLcdUpdate = TRUE;
    }

    //-------------------Clear Button------------------------------
    // [F28] Physical hold only, and the one CLEAR gesture that stays that way.
    // Held CLEAR erases the step under the playhead every bar; latched, it would
    // quietly eat the whole instrument while nobody was touching the panel. A
    // latch may arm a destructive chord - it must not sustain a destructive act.
    // [3K] Not while INST is down: CLEAR+INST+pads is the instant instrument
    // clear, and the pad that names an instrument for that chord also *selects*
    // it, so this block was erasing the newly selected lane at the playhead on
    // top of the wipe the chord was already doing.
    //
    // [3L] ...and not on the leading edge of the press either, which is what the
    // [3K] guard missed. CLEAR goes down first in that chord and INST follows a
    // tenth of a second later, so for those hundred milliseconds `instBtn` was
    // still false, this read a bare held CLEAR, and it erased a step - with its
    // own undo level. That is the stray CH step that survived: the guard was
    // right, it was just being asked too early.
    //
    // [3N] 3L bought that discrimination with a delay - the erase waited out
    // MOMENTARY_TIME to see whether INST was coming - and a held CLEAR that takes
    // a third of a second to bite is the wrong trade for a live control. This
    // erases from the first scan again, as it always did, and repairs instead:
    // if INST turns up promptly the press was always CLEAR+INST+pads, the step
    // taken in the gap was never meant to go, and the snapshot to put it back is
    // the one this block pushed on its way in. One UndoPop and the step and its
    // level both come back.
    //
    // Promptly, and only promptly. Hold CLEAR for a second, erase in earnest and
    // then reach for INST, and those are two gestures - the erasing stands.
    static boolean clearSawInst = FALSE;
    static boolean clearErasing = FALSE;
    if (clearBtn.justPressed || clearBtn.justRelease){ clearSawInst = FALSE; clearErasing = FALSE; }

    if (instBtn && !clearSawInst){
      clearSawInst = TRUE;
      if (clearErasing && clearBtn.pressed
          && (millis() - clearBtn.curTime) < MOMENTARY_TIME){
        UndoPop();                             // the step back, and the level with it
        clearErasing = FALSE;
      }
    }

    if (clearBtn.pressed && !clearSawInst
        && !shiftBtn && !BtnHeld(&shufBtn) && !PanelBusy() && !seq.configMode
        && !keyboardMode
        && curSeqMode != PTRN_TAP && isRunning){

      if (!clearErasing){ clearErasing = TRUE; UndoPush(); }                           // [F4] one level per gesture, not per step
      tempMuteInst = (1U << curInst);        // [B-02] transient, not a snapshot

      StepClear(curInst, curStep);                                                     // [B-33] flam goes with it
      pattern[ptrnBuffer].velocity[curInst][curStep] = instVelLow[curInst];
      if (curInst == CH) pattern[ptrnBuffer].velocity[CH][curStep] = instVelHigh[HH];//update HH velocity that OH is trigged correctly
      patternWasEdited = TRUE;
    }
    
    if (clearBtn.justRelease) tempMuteInst = 0;                                                               // [B-02]

    // [3E] A bare CLEAR while stopped used to clear the whole pattern here. One
    // press, no modifier, no confirmation, and the same key that means "take this
    // step out" while running. Clearing a pattern is CLEAR+PTRN now and only
    // that; with the sequencer stopped, CLEAR on its own does nothing.


    //-------------------shuffle Button------------------------------                          [zabox] test
    if (shufBtn.justPressed || shufBtn.justRelease) needLcdUpdate = TRUE;
    
    if (BtnHeld(&shufBtn)) {

      // [F19] CLEAR puts every instrument back on the pattern's shuffle. Swing
      // is dialled in one voice at a time and there is no single value that
      // means "none of this" - without a way out you would have to walk all
      // sixteen back by hand. The clear gestures above stand aside while this
      // page is open so the same key does not also erase steps.
      if (clearBtn.justPressed){
        UndoPush();
        for (byte z = 0; z < NBR_INST; z++) pattern[ptrnBuffer].instSwing[z] = 0;
        patternWasEdited = TRUE;
        needLcdUpdate = TRUE;
      }

      // [3E] Ask for a redraw and let LcdUpdate() lay the page out. This used to
      // poke the panel directly - rub out the old marker, draw the new one - with
      // the bar's coordinates written into the sequencer, a second copy of the
      // layout that had to be kept in step with LCD.inc by hand. It never was:
      // the flam branch was still erasing along line 1, where the swing bar and
      // the instrument name had moved in, and it printed the flam digit at cell
      // 13 when the digit sat at 15. The framebuffer makes a full redraw cost
      // only the cells that actually changed, so there is nothing left to buy.
      if (inStepNow) {
        if (FirstBitOn() < 7) {
          pattern[ptrnBuffer].shuffle = FirstBitOn() + 1;
          if (pattern[ptrnBuffer].shuffle != prevShuf){
            prevShuf = pattern[ptrnBuffer].shuffle;
            patternWasEdited = TRUE;
            needLcdUpdate = TRUE;
          }
        }
        else if (FirstBitOn() > 7) {
          pattern[ptrnBuffer].flam = FirstBitOn() - 8;
          if (pattern[ptrnBuffer].flam != prevFlam){
            prevFlam = pattern[ptrnBuffer].flam;
            patternWasEdited = TRUE;
            needLcdUpdate = TRUE;
          }
        }
      }
    }
      
    
    if (scaleBtn.justPressed && !keyboardMode && !shiftBtn && !instBtn){
      // [6P] ...and it takes an undo level, which it never did - the last
      // pattern-changing gesture on the panel that did not. One unmodified press
      // in the modifier cluster, directly above CLEAR, that halves or quadruples
      // the rate of everything you are playing.
      UndoPush();
      needLcdUpdate = TRUE;
      patternWasEdited = TRUE;
      scaleBtn.counter++;
      if (scaleBtn.counter == 4) scaleBtn.counter = 0;
      switch (scaleBtn.counter){
      case 0:
        pattern[ptrnBuffer].scale =  SCALE_16;// 1/16
        break;
      case 1:
        pattern[ptrnBuffer].scale =  SCALE_32;// 1/32
        break;
      case 2:
        pattern[ptrnBuffer].scale =  SCALE_8t;// 1/8t 
        break;
      case 3:
        pattern[ptrnBuffer].scale =  SCALE_16t;// 1/16t 
        break;
      }
    } 

    //-------------------last step button------------------------------
    // [6K][6M] The physical key. LASTSTEP does not latch at all any more.
    //
    // LASTSTEP+pad is the most destructive unmodified gesture on the panel: one
    // pad sets the pattern length absolutely and returns every lane to following
    // it, so pad 1 while latched turned whatever you were playing into a one-step
    // loop with its polyrhythms gone. It latched on a double tap, which is two
    // quick presses - reachable by accident, and reported as exactly that:
    // "the pattern length became 1 step... could it be accidental button press?"
    //
    // [5U] had already refused the latch in keyboard mode. [6K] took it off the
    // pads everywhere else, which left a latch whose only remaining effect was to
    // put the encoder on the length - one more state to reason about for a
    // convenience the key does not need. [6M] removed it outright, so LASTSTEP has
    // one state: held or not. The guards below read the same physical key.
    if (lastStepBtn.pressed && inStepNow) {                                          // [F28]
      if (instBtn){
        // [6D] And it takes an undo level, which it never did - the pattern-wide
        // branch below has had one since [F4] while the per-lane gesture beside
        // it had none. On the press, not on the level: LaneSetParam() runs every
        // pass while the pad is down, and four levels of the same length would
        // be no undo at all.
        if (inStepPressed) UndoPush();
        LaneSetParam(curInst, LANE_SPAN, (byte)(FirstBitOn() + 1));              // [F3]
      }
      else {
        // [3Q] Setting the pattern's length puts every lane back on the pattern.
        //
        // A lane with its own length wraps at its own span and ignores this
        // entirely, so with lanes set the bar changed and the music did not - the
        // display said 8 and all sixteen steps kept playing. It also breaks tap
        // record, which writes at tapStepCount (wrapping at the pattern length)
        // while playback reads the lane phase (wrapping at the span): the two
        // indices diverge and most taps land where nothing will read them.
        //
        // "The pattern is this long" is the one gesture that means the whole
        // pattern, so it is also the way back out of a polyrhythm. INST+LASTSTEP
        // is still the per-lane gesture, and undo covers this one.
        //
        // [6U] On the press, not on the level. inStepNow is a level, so this whole
        // branch runs every pass the pad is held - and the unguarded UndoPush()
        // filled all four levels with the same post-change state inside a few
        // milliseconds. Undo then "worked" four times and put back what was
        // already there, which is indistinguishable from undo being broken.
        //
        // Reported as exactly that, and as the lane length undoing correctly:
        // [6D] gave the INST branch above this guard and left its neighbour
        // without one. Two branches of one `if`, one gesture apart.
        if (inStepPressed) UndoPush();                                       // [F4]
        pattern[ptrnBuffer].length = FirstBitOn();
        for (byte z = 0; z < NBR_INST; z++){
          if (pattern[ptrnBuffer].lane[z][LANE_SPAN]) LaneSetParam(z, LANE_SPAN, 0);
        }
        LaneSyncAll();
        patternWasEdited = TRUE;
      }
      needLcdUpdate = TRUE;
    }
    //-------------------Steps buttons------------------------------
    /////////////////////////////STEP EDIT////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if (curSeqMode == PTRN_STEP){
      if (stepsBtn.justRelease) doublePush = FALSE; 
      if(!lastStepBtn.pressed && !instBtn && !keyboardMode && !BtnHeld(&shufBtn) && !StepModifierHeld()){             // [zabox] test [F28][F17]
        if (isRunning)
        {
          if (inStepPressed) UndoPush();                                              // [F4] before the edit
          pattern[ptrnBuffer].inst[curInst] = InstValueGet(pattern[ptrnBuffer].inst[curInst]);//cf InstValueGet()
        }
        else if (!isRunning)
        {//Return pattern number
          if (stepsBtn.pressed){
            if (bankBtn.pressed){
              if(FirstBitOn() >= MAX_BANK) curBank = MAX_BANK;
              else curBank = FirstBitOn();
              nextPattern = curBank * NBR_PATTERN + (curPattern % NBR_PATTERN);
              if(PlayingPattern() != nextPattern) selectedPatternChanged = TRUE;
              group.length = 0;
            }
            else{//pattern group edit------------------------------------------------------
              if (SecondBitOn())
              {
                trackPosNeedIncremante = FALSE;                                                               // [zabox] fixes group bug
                group.length = SecondBitOn() - FirstBitOn();
                nextPattern = group.firstPattern = FirstBitOn() + curBank * NBR_PATTERN;
                group.pos = 0;                                                    // [3F] chains start at their first pattern
                doublePush = TRUE;
              }
              else if (!doublePush){
                // [4Z] A chain is a live thing now - see the note at its other
                // builder below. Selecting one pattern ends whatever chain was
                // running; there is nothing stored to un-store.
                nextPattern = FirstBitOn() + curBank * NBR_PATTERN;
                group.length = 0;
                group.pos    = 0;
              }
              if(PlayingPattern() != nextPattern) selectedPatternChanged = TRUE;
            }
          }
        }
      }
      /*if (trackPosNeedIncremante && group.length ){//&& stepCount > 0)
       group.pos++;
       if (group.pos > group.length) group.pos = 0;
       nextPattern = group.firstPattern + group.pos;
       if(PlayingPattern() != nextPattern) selectedPatternChanged = TRUE;
       trackPosNeedIncremante = FALSE;
       needLcdUpdate = TRUE;
       }*/
    }
    //////////////////////////////TAP EDIT///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    else if (curSeqMode == PTRN_TAP)
    {
      //static byte lastHHtrigged;// remmeber last OH or CH was trigged to prevent OH noise when trigget other instruments


      // [3M] Not while INST is in the gesture. This is tap mode's own live erase -
      // CLEAR + pads, wiping those lanes at the playhead as it goes - and it was
      // running underneath CLEAR+INST+pads as well, which is a different gesture
      // handled once at the top of this function.
      //
      // It has the same two faults 3K and 3L fixed there, for the same reasons:
      // it reads InstMaskFromButtons() per scan, so pad 13 on its way to the 13+14
      // chord names CH and erases a step of it, and it pushes an undo level per
      // press. One stray CH step and a second undo level, in tap mode only - which
      // is why it came back after switching modes when step mode had been fixed.
      //
      // clearSawInst is the same flag the step-mode erase uses: set the moment
      // INST appears during a CLEAR press and held until that press ends, so the
      // order the two keys go down in cannot matter.
      if (BtnHeld(&clearBtn) && !shiftBtn && !clearSawInst){                                           // [zabox] new function in tap mode: hold clear + inst in tap mode

        if (inStepNow) {
          {
          // [F5] Every held instrument, not just the first. The old code took
          // FirstBitOn() and switched on it, so a chord cleared one lane and
          // silently ignored the rest - and on the CH button with doublePush
          // already set, no case matched at all and clearInst was left
          // uninitialised, then shifted by.
          // [6Y] Not on a config page, for the same reason as the cursor erase in
          // step mode above.
          // [5H] The level, not the edge, twice over: this erase has to keep
          // firing as the playhead moves under a held pad. InstSelectMask() takes
          // both because its two halves want different things - the chord form
          // needs the level so a second pad can change the answer, the one-pad
          // form normally wants the edge. Here the level is right for both.
          const unsigned int m = InstSelectMask(inStepNow, inStepNow);
          if (inStepPressed) UndoPush();                                                                 // [F4]
          tempMuteInst = m;                                                                              // [B-02]

          for (byte i = 0; i < NBR_INST; i++){
            if (!bitRead(m, i)) continue;
            StepClear(i, curStep);                                                                       // [B-33] flam goes with it
            pattern[ptrnBuffer].velocity[i][curStep] = instVelLow[i];                                    // [zabox] was missing here
            if (i == CH) pattern[ptrnBuffer].velocity[CH][curStep] = HIGH_VEL;   //update HH velocity that OH is trigged correctly
          }
          patternWasEdited = TRUE;
          }
        }
        else tempMuteInst = 0;                                                                           // [B-02]
      }
      
      if (clearBtn.justRelease) tempMuteInst = 0;                                                               // [B-02]

      // [F11] Whatever is still held rolls, under exactly the modifiers that let
      // a pad play in the first place - a pad being used as a clear target or an
      // instrument selector is not being played, so it must not repeat either.
      rollMask = (!lastStepBtn.pressed && !instBtn && !BtnHeld(&clearBtn)
                  && !BtnHeld(&shufBtn) && !StepModifierHeld())
                 ? InstMaskFromButtons(inStepNow) : 0;

      if (!lastStepBtn.pressed && !instBtn && !BtnHeld(&clearBtn) && !BtnHeld(&shufBtn) && !StepModifierHeld())   // [zabox] test [F28][F17]
      {
        static boolean doublePushOH;
        static byte tempVel[16];//store temp instrument velocity
        static byte triggedInst;

        if (bitRead(inStepNow,12) && bitRead(inStepNow,13)) doublePushOH = 1;
        else doublePushOH = 0;

        for (byte a = 0; a < NBR_STEP_BTN; a++){
          if (bitRead(inStepPressed, a)){                                                    // [3A]
              // [F4] One undo level per burst of taps, not per tap. This cannot
              // key off endMeasure: that flag is set at the first bar end and
              // never cleared (see Clock.inc), so "commit at end of measure" has
              // always meant "commit on the next loop pass" - and grouping by it
              // gave one undo per tap, which is what was reported. A gap of quiet
              // is the honest boundary, and it also folds simultaneous taps into
              // one level for free.
              if ((millis() - tapUndoTime) > TAP_UNDO_GAP) UndoPush();
              tapUndoTime = millis();
              // [4D] Cyclic record, before the switch writes anything: the wipe
              // has to precede the velocity this tap is about to set, or it
              // would erase it. The hi-hat pads resolve to OH when both are
              // down - taking instOut[] at face value there would clear CH for
              // a tap the player aimed at OH.
              CycRecTouch((doublePushOH && (a == CH_BTN || a == CH_LOW_BTN)) ? OH : instOut[a]);
              //the "a" button is pressed NOW!
              switch (a){
              case BD_BTN:
                if (isRunning && tapRecordArm) pattern[ptrnBuffer].velocity[instOut[a]][TapIndex(instOut[a])] = instVelHigh[BD];
                tempVel[BD] = instVelHigh[BD];
                triggedInst = BD;
                break;
              case BD_LOW_BTN:
                if (isRunning && tapRecordArm) pattern[ptrnBuffer].velocity[instOut[a]][TapIndex(instOut[a])] = instVelLow[BD];
                tempVel[BD] = instVelLow[BD];
                triggedInst = BD;
                break;
              case SD_BTN:
                if (isRunning && tapRecordArm) pattern[ptrnBuffer].velocity[instOut[a]][TapIndex(instOut[a])] = instVelHigh[SD];
                tempVel[SD] = instVelHigh[SD];
                triggedInst = SD;
                break;
              case SD_LOW_BTN:
                if (isRunning && tapRecordArm) pattern[ptrnBuffer].velocity[instOut[a]][TapIndex(instOut[a])] = instVelLow[SD];
                tempVel[SD] = instVelLow[SD];
                triggedInst = SD;
                break;
              case LT_BTN:
                if (isRunning && tapRecordArm) pattern[ptrnBuffer].velocity[instOut[a]][TapIndex(instOut[a])] = instVelHigh[LT];
                tempVel[LT] = instVelHigh[LT];
                triggedInst = LT;
                break;
              case LT_LOW_BTN:
                if (isRunning && tapRecordArm) pattern[ptrnBuffer].velocity[instOut[a]][TapIndex(instOut[a])] = instVelLow[LT];
                tempVel[LT] = instVelLow[LT];
                triggedInst = LT;
                break;
              case MT_BTN:
                if (isRunning && tapRecordArm) pattern[ptrnBuffer].velocity[instOut[a]][TapIndex(instOut[a])] = instVelHigh[MT];
                tempVel[MT] = instVelHigh[MT];
                triggedInst = MT;
                break;
              case MT_LOW_BTN:
                if (isRunning && tapRecordArm) pattern[ptrnBuffer].velocity[instOut[a]][TapIndex(instOut[a])] = instVelLow[MT];
                tempVel[MT] = instVelLow[MT];
                triggedInst = MT;
                break;
              case HT_BTN:
                if (isRunning && tapRecordArm) pattern[ptrnBuffer].velocity[instOut[a]][TapIndex(instOut[a])] = instVelHigh[HT];
                tempVel[HT] = instVelHigh[HT];
                triggedInst = HT;
                break;
              case HT_LOW_BTN:
                if (isRunning && tapRecordArm) pattern[ptrnBuffer].velocity[instOut[a]][TapIndex(instOut[a])] = instVelLow[HT];
                tempVel[HT] = instVelLow[HT];
                triggedInst = HT;
                break;
              case CH_BTN:
                if (!doublePushOH){
                  if (isRunning && tapRecordArm) pattern[ptrnBuffer].velocity[instOut[a]][TapIndex(instOut[a])] = instVelHigh[CH];
                  tempVel[CH] = instVelHigh[CH];
                  triggedInst = CH;
                }
                break;
              case CH_LOW_BTN:
                if (!doublePushOH){
                  if (isRunning && tapRecordArm) pattern[ptrnBuffer].velocity[instOut[a]][TapIndex(instOut[a])] = instVelLow[CH];
                  tempVel[CH] = instVelLow[CH];
                  triggedInst = CH;
                }
                break;
              case RM_BTN:
                if (isRunning && tapRecordArm) pattern[ptrnBuffer].velocity[instOut[a]][TapIndex(instOut[a])] = instVelHigh[RM];
                tempVel[RM] = instVelHigh[RM];
                triggedInst = RM;
                break;
              case HC_BTN:
                if (isRunning && tapRecordArm) pattern[ptrnBuffer].velocity[instOut[a]][TapIndex(instOut[a])] = instVelHigh[HC];
                tempVel[HC] = instVelHigh[HC];
                triggedInst = HC;
                break;
              case CRASH_BTN:
                if (isRunning && tapRecordArm) pattern[ptrnBuffer].velocity[instOut[a]][TapIndex(instOut[a])] = instVelHigh[CRASH];
                tempVel[CRASH] = instVelHigh[CRASH];
                triggedInst = CRASH;
                break;
              case RIDE_BTN:
                if (isRunning && tapRecordArm) pattern[ptrnBuffer].velocity[instOut[a]][TapIndex(instOut[a])] = instVelHigh[RIDE];
                tempVel[RIDE] = instVelHigh[RIDE];
                triggedInst = RIDE;
                break;
              }
              if (doublePushOH){
            //  if (isRunning && tapRecordArm) pattern[ptrnBuffer].velocity[instOut[a]][TapIndex(instOut[a])] = instVelHigh[OH];
                if (isRunning && tapRecordArm) pattern[ptrnBuffer].velocity[OH][TapIndex(OH)] = instVelHigh[OH];                     // [zabox] fixes oh low velocity bug in tap mode (dim leds, no sound after mux fix)
                tempVel[OH] = instVelHigh[OH];
                triggedInst = OH;
              }

              //-----SET Velocity Values-----//
              SetMuxTrigMidi(triggedInst, tempVel[triggedInst]);
              rollVel[triggedInst] = tempVel[triggedInst];        // [F11] repeats at what was played

              if( a == CH_LOW_BTN || a == CH_BTN || inStepNow == OH_BTN){
                //If OH is tapped
                if (doublePushOH){
                  lastHHtrigged = 0;
                  if(isRunning && tapRecordArm){
                    TEMPINST_ATOMIC_BEGIN;                                                   // [F11]
                    bitSet(tempInst[OH], TapIndex(OH));
                    bitClear(tempInst[CH], TapIndex(CH));
                    TEMPINST_ATOMIC_END;
                  }
                }
                //If CH is tapped
                else {
                  lastHHtrigged = B10;
                  if(isRunning && tapRecordArm){
                    TEMPINST_ATOMIC_BEGIN;                                                   // [F11]
                    bitSet(tempInst[CH], TapIndex(CH));
                    bitClear(tempInst[OH], TapIndex(OH));
                    TEMPINST_ATOMIC_END;
                  }
                }
                // [B-17] tempDoutTrig is what TIMER2 restores when the 2 ms
                // pulse expires; set it before starting the pulse.
                tempDoutTrig = lastHHtrigged;
                SetDoutTrig((1 << HH) | lastHHtrigged);
              }
              else{
                //SetDoutTrig(1 << instOut[a] | lastHHtrigged);
                tempDoutTrig = lastDoutTrig & B11;                                            // [B-17]
                // [4B] A tapped pad has always ignored the mutes while a roll from the
                // same pad obeyed them - the same finger, two answers. `tap mut` makes
                // it one answer: ON, both ignore the mutes; OFF, both follow them.
                if (seq.tapOverMute || !bitRead(ActiveMute(), instOut[a])){
                  SetDoutTrig((1 << instOut[a]) | (lastDoutTrig & B11));                      // [zabox] [1.027] fixes hh cuts when tapping other instruments
                }
                if(isRunning && tapRecordArm){ TEMPINST_ATOMIC_BEGIN; bitSet(tempInst[instOut[a]], TapIndex(instOut[a])); TEMPINST_ATOMIC_END; }   // [F11]
              }
              // [B-17] Was: "while (TCCR2B) {}" to wait out the previous pulse,
              // then delayMicroseconds(2000) and a manual trigger-off - up to
              // 4 ms of dead main loop per tap, with two simultaneous taps
              // costing double. That directly capped how accurately taps could
              // be timed, and stalled MIDI.read() and the encoder along with it.
              // The 2 ms trigger-off timer already exists and the sequencer path
              // has always used it; use it here too and return immediately.
              TRIG_TIMER_ZERO;
              TRIG_TIMER_START;
          }
        }//END FOR LOOP
      }

      if (endMeasure){//Update pattern at the end of measure to not get a double trig
        // [F11] Take a copy with interrupts off: the roll driver writes tempInst
        // from the timer ISR, and reading-then-clearing it in two steps would
        // drop any hit that landed between them.
        unsigned int ti[NBR_INST];
        TEMPINST_ATOMIC_BEGIN;
        for (byte z = 0; z < NBR_INST; z++){ ti[z] = tempInst[z]; tempInst[z] = 0; }
        TEMPINST_ATOMIC_END;

        // [3B] Tapping a hi-hat used to clear the other lane at that step. It no
        // longer does: both lanes keep what they were given and seq.hhPriority
        // decides which is heard. The OH and CH special cases that used to head
        // this loop went with that - they chained through an else-if that made
        // them commit on a different pass from everything else, which was only
        // ever there to sequence an interaction that no longer exists.
        // [B-33] StepAddPlain, not |=. A tapped step is a plain step, and must
        // not inherit a flam from whatever occupied that position before - which
        // is what made a freshly tapped lane come out as a mix of flammed and
        // unflammed steps after the lane was cleared.
        for (byte inst = 0; inst < NBR_INST; inst++){
          if (!ti[inst]) continue;
          StepAddPlain(inst, ti[inst]);

          // [3R] Nothing to grow. F27 stretched the lane to reach a tap that had
          // landed outside its span - because taps were written at the master step
          // while the lane played from its own phase, so they often did. They are
          // written at the lane's own index now, so a tap is inside the span by
          // construction and a lane keeps the length you gave it however long you
          // play into it. A polyrhythm that grows back to bar length while you
          // record onto it is not a polyrhythm.
          patternWasEdited = TRUE;
        }
      }
    }//END IF PTRN_TAP MODE
  }//ENDIF MODE EDIT

  //======================================================================================================================================================================================================================================
  //////////////////////////MODE PATTERN PLAY...//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  if (curSeqMode == PTRN_PLAY && !PanelBusy()){                                       // [F26][F25]
    //-------------------------------select pattern-----------------------------------
    if (stepsBtn.justRelease) doublePush = FALSE;
    // [6N] Nothing held. StepModifierHeld() lists the keys that borrow the pads
    // in the step editor and leaves SHIFT and INST out on purpose, because there
    // they have pad meanings of their own. Here they have none, so a pad under
    // them fell straight through to a pattern change - INST+pad in play mode is
    // 'select this instrument' everywhere else on the panel and here it swapped
    // the pattern instead, with no way back to what was playing.
    if (inStepNow && !AnyModifierHeld()){                                               // [F17]
      if (bankBtn.pressed){
        if(FirstBitOn() >= MAX_BANK) curBank = MAX_BANK;
        else curBank = FirstBitOn();
        nextPattern = curBank * NBR_PATTERN + (curPattern % NBR_PATTERN);
        group.length = 0;//should be 0 to play the right next pattern
        // [3F] Queue it here, exactly as the plain pattern select below does.
        // This was `&& !isRunning`, so while playing the press only moved
        // nextPattern and nothing prepared the back buffer. The bar-end commit
        // then noticed the mismatch and queued the load *at* the bar line, so the
        // ISR could not flip until the bar after that - a bank change landed one
        // whole pattern late, every time.
        //
        // On justPressed, not on the level: the handler reloads whenever
        // curPattern still differs, and curPattern does not catch up until the
        // swap, so a held pad would re-run a 30-50 ms I2C load every pass.
        if (PlayingPattern() != nextPattern && stepsBtn.justPressed) selectedPatternChanged = TRUE;   // [5E]
      }
      else{
        //Group selected
        if (SecondBitOn())
        {
          // The first scan on which both pads are down. NOT stepsBtn.justPressed:
          // that fires when the step group goes from none-pressed to any-pressed,
          // and a chord is normally made by adding the second pad to a pad already
          // held - so on the scan the chord completes it is long since false. Used
          // as the seating edge it never fired at all, and the chain went on with
          // whatever the previous selection had left in the counter.
          //
          // doublePush is exactly the right flag: set here, cleared when the last
          // pad comes up, so !doublePush means "this chord is new".
          const boolean newChord = !doublePush;
          group.length = SecondBitOn() - FirstBitOn();
          group.firstPattern = FirstBitOn() + curBank * NBR_PATTERN;

          // [5C] group.pos is the position of the pattern that is QUEUED, not of
          // the one playing - the bar-end advance arms a bar ahead, so by the time
          // a pattern is sounding the counter has already moved past it. Every
          // seat of this counter has to respect that or the chain is a bar out.
          //
          // [3F] From cold, that means 0: nothing is queued yet, the first bar
          // line starts the first pattern, and the advance then moves the counter
          // to 1. (It was group.length, which wrapped to 0 at that first bar end
          // and queued the first pattern a second time.)
          //
          // [5C] But a chain chosen *around the pattern already playing* is not
          // starting from cold - it is being asked to carry on from here. Seating
          // it at 0 queues nothing for the coming bar line, so that pattern played
          // a second time before the chain got going: 1-2 running, select 2-3
          // while 2 plays, and 2 came round again. Take up its place instead and
          // queue the one after it, which is what the bar-end advance would have
          // done had the chain existed a bar earlier.
          //
          // Once per chord. This block runs every pass the two pads are held, and
          // re-seating on each of them would re-derive the counter from a
          // curPattern that moves underneath it - hold the chord across a bar line
          // and the seat and the bar-end advance cancel out, leaving nothing
          // queued and the pattern repeating.
          if (newChord){
            const byte off = (byte)(PlayingPattern() - group.firstPattern);   // [5E]
            if (isRunning && off <= group.length){
              byte q = (byte)(off + 1);
              if (q > group.length) q = 0;
              group.pos   = q;
              nextPattern = group.firstPattern + q;
            }
            else {
              group.pos   = 0;
              nextPattern = group.firstPattern;
            }
            // Queue it here rather than leaving it to the test below, which also
            // waits on stepsBtn.justPressed and so is equally deaf to the second
            // pad of a chord. Nothing armed for the coming bar line is a bar of
            // whatever is already playing - the repeat this was reported as.
            if (PlayingPattern() != nextPattern) selectedPatternChanged = TRUE;
          }
          doublePush = TRUE;
        }
        //Only one pattern selected
        else if (!doublePush){
          group.length = 0;//should be 0 to play the right next pattern
          nextPattern = FirstBitOn() + curBank * NBR_PATTERN;
          group.pos = 0;
        }
        if(PlayingPattern() != nextPattern && stepsBtn.justPressed) {                                          // [zabox] [1.027] [5E]
          // [B-26] Preload on the key press in BOTH sync modes. In SYNC the
          // prepared back buffer simply waits and the ISR flips it on the bar
          // line; the old code deliberately skipped the load here, which is
          // why the load then landed after the bar and ate step 0.
          selectedPatternChanged = TRUE;
          if (seq.ptrnChangeSync == SYNC && isRunning) needLcdUpdate = TRUE;//selected pattern changed so we need to update display
        }
      }
    }

    //--------------------------------sequencer run direction-----------------------
    if (shiftBtn && dirBtn.justPressed){
      if (seq.dir++ >= MAX_SEQ_DIR) seq.dir = FORWARD;
    }

    //--------------------------------pattern next update---------------------------
    /* if (trackPosNeedIncremante && group.length ){// && stepCount > 0)
     if (group.pos > group.length) group.pos = 0;
     nextPattern = group.firstPattern + group.pos;
     group.pos++;
     if(PlayingPattern() != nextPattern) selectedPatternChanged = TRUE;
     trackPosNeedIncremante = FALSE;
     needLcdUpdate = TRUE;
     }
     else*/
    // [B-01] Bar-end commit moved out of this mode block — see the unified
    // commit below, ahead of the pattern track load/save section.

  }//END IF MODE PLAY PATTERN

  //===================================================================================================================================

  //===================================================================================================================================

  //////////////////////////MODE TRACK WRITE/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  if (curSeqMode == TRACK_WRITE && !PanelBusy()){                                     // [F26][F25]
    //-------------------------------select pattern-----------------------------------
    if (inStepNow && !StepModifierHeld()){                                              // [F17]

      if (bankBtn.pressed){
        if(FirstBitOn() >= MAX_BANK) curBank = MAX_BANK;
        else curBank = FirstBitOn();
        nextPattern = curBank * NBR_PATTERN + (curPattern % NBR_PATTERN);
        if(PlayingPattern() != nextPattern) selectedPatternChanged = TRUE;
      }
      else if(numBtn.pressed){
        trk.next = FirstBitOn();
        selectedTrackChanged = TRUE;
        needLcdUpdate = TRUE;
      }
      else{
        nextPattern = FirstBitOn() + curBank * NBR_PATTERN;
        if(PlayingPattern() != nextPattern) selectedPatternChanged = TRUE;
      }
    }
    //decremente track position
    if (backBtn.justPressed){
      // [B-15] trk.pos is unsigned, so the old "trk.pos < 0" test was always
      // false and underflow was caught only incidentally by the > MAX test.
      // Guard before decrementing instead.
      if (trk.pos == 0 || trk.pos >= MAX_PTRN_TRACK) trk.pos = 0;                     // [3B] index, not count
      else trk.pos--;
      nextPattern = track[trkBuffer].patternNbr[trk.pos];
      if(PlayingPattern() != nextPattern) selectedPatternChanged = TRUE;
      needLcdUpdate = TRUE;
    }
    //incremente track position
    if (fwdBtn.justPressed){
      trk.pos++;
      if (trk.pos >= MAX_PTRN_TRACK) trk.pos = MAX_PTRN_TRACK - 1;                     // [3B] index, not count
      nextPattern = track[trkBuffer].patternNbr[trk.pos];
      if(PlayingPattern() != nextPattern) selectedPatternChanged = TRUE;
      needLcdUpdate = TRUE;
    }
    //go to first measure
    if (clearBtn.justPressed){
      trk.pos = 0;
      nextPattern = track[trkBuffer].patternNbr[trk.pos];
      if(PlayingPattern() != nextPattern) selectedPatternChanged = TRUE;
      needLcdUpdate = TRUE;
    }
    if(shiftBtn){
      //go to last measure
      if (numBtn.pressed){
        trk.pos = track[trkBuffer].length;
        nextPattern = track[trkBuffer].patternNbr[trk.pos];
        if(PlayingPattern() != nextPattern) selectedPatternChanged = TRUE;
        needLcdUpdate = TRUE;
      }
      //delete current pattern in the current position
   // if(backBtn.justPressed) {
      if (backBtn.justPressed && track[trkBuffer].length){                                                  // [zabox] fixes crashs when deleting the last track pos
        if (trk.pos < (track[trkBuffer].length - 1)) {                                                      // [zabox] delete only valid track pos
        for (unsigned int a = trk.pos + 1; a < track[trkBuffer].length; a++){
          track[trkBuffer].patternNbr[a] = track[trkBuffer].patternNbr[a + 1];
        }
        trk.pos +=1;//to stay in the same position
        track[trkBuffer].length = track[trkBuffer].length - 1;//decremente length by 1 du to deleted pattern
        nextPattern = track[trkBuffer].patternNbr[trk.pos];
        if(PlayingPattern() != nextPattern) selectedPatternChanged = TRUE;
        trackNeedSaved = TRUE;
        needLcdUpdate = TRUE;
        }
        else {
          trk.pos +=1;//to stay in the same position
        }
      }
      //insert a pattern
      if (fwdBtn.justPressed) {                                                                              
        if (trk.pos < (track[trkBuffer].length + 1)) {                                                     // [zabox] insert only inside track
        // [B-15] The loop ran while a >= trk.pos, so with trk.pos == 0 the last
        // iteration read patternNbr[-1] - an out-of-bounds read of whatever RAM
        // precedes the track array. Stop one short: position trk.pos is written
        // by the insert itself immediately below.
        for (unsigned int a = track[trkBuffer].length + 1; a > trk.pos; a--){
          track[trkBuffer].patternNbr[a] = track[trkBuffer].patternNbr[a - 1];
        }
        trk.pos -=1;//to stay in the same position
        track[trkBuffer].patternNbr[trk.pos] = curPattern;
        track[trkBuffer].length = track[trkBuffer].length + 1;//decremente length by 1 du to deleted pattern
        nextPattern = track[trkBuffer].patternNbr[trk.pos];
        if(PlayingPattern() != nextPattern) selectedPatternChanged = TRUE;
        trackNeedSaved = TRUE;
        needLcdUpdate = TRUE;
        }
        else {
          trk.pos -=1;//to stay in the same position
        }
      }
    }//end shift

    //write selected pattern in the current track position
    if (enterBtn.justRelease && !trackJustSaved){
      track[trkBuffer].patternNbr[trk.pos] = curPattern;
      trk.pos++;
      if (trk.pos >= MAX_PTRN_TRACK) trk.pos = MAX_PTRN_TRACK - 1;                     // [3B] index, not count
      nextPattern = track[trkBuffer].patternNbr[trk.pos];
      if(PlayingPattern() != nextPattern) selectedPatternChanged = TRUE;
      track[trkBuffer].length = trk.pos;
      trackNeedSaved = TRUE;
      needLcdUpdate = TRUE;
    }
  }//END IF MODE TRACK WRITE

    //////////////////////////MODE TRACK PLAY/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  if (curSeqMode == TRACK_PLAY)
  {
    if (trackPosNeedIncremante){//(endMeasure)
      trk.pos++;
      if (trk.pos >= track[trkBuffer].length) trk.pos = 0;
      nextPattern = track[trkBuffer].patternNbr[trk.pos];
      // [6B] Set with the change, not beside it. A track position that repeats
      // the pattern already playing queues nothing, and a flag left standing
      // would restart the lanes on whatever free change came next instead.
      if(PlayingPattern() != nextPattern){
        selectedPatternChanged = TRUE;
        barLineChange = TRUE;
      }
      trackPosNeedIncremante = FALSE;
      needLcdUpdate = TRUE;

    }
    // [F26] Only the button handling is gated, not the position advance above:
    // a track has to keep playing while its name is being typed.
    //go to first measure
    if (clearBtn.justPressed && !PanelBusy()){
      trk.pos = 0;
      nextPattern = track[trkBuffer].patternNbr[trk.pos];
      if(PlayingPattern() != nextPattern) selectedPatternChanged = TRUE;
      needLcdUpdate = TRUE;
    }
    if(inStepNow && !PanelBusy() && !StepModifierHeld()){                              // [F17]
      trk.next = FirstBitOn();
      selectedTrackChanged = TRUE;
      trackChangeManual = TRUE;                                                    // [3C]
      needLcdUpdate = TRUE;
    }
  }

  //------------------------------------Commit queued pattern at bar end---------------------
  // [B-01] Previously this lived in two places: the group case at the end of
  // SeqParameter(), and the single-pattern case INSIDE "if (curSeqMode ==
  // PTRN_PLAY)". Entering MUTE replaces curSeqMode, so the single-pattern case
  // stopped running and a queued change was stranded until MUTE was exited.
  // v1.025 fixed only the group half of this; the other half stayed broken.
  //
  // Now mode-independent for the three pattern-oriented modes. TRACK_PLAY and
  // TRACK_WRITE are excluded — they consume trackPosNeedIncremante themselves.
  // Placed ahead of the selectedPatternChanged handler below so the commit is
  // serviced in the same loop pass, matching the old PTRN_PLAY latency.
  if (curSeqMode == PTRN_PLAY || curSeqMode == PTRN_STEP || curSeqMode == MUTE){
    if (trackPosNeedIncremante){
      if (group.length){
        group.pos++;
        if (group.pos > group.length) group.pos = 0;
        nextPattern = group.firstPattern + group.pos;
        needLcdUpdate = TRUE;
      }
      // [B-26] !nextPatternReady: curPattern is now updated when the swap
      // actually takes effect, not when the load finishes, so without this
      // guard an already-armed change would be re-loaded on every pass.
      //
      // [5E] PlayingPattern(), not curPattern. This runs at the bottom of the
      // pass, and a bar line that fell after the top left curPattern naming the
      // pattern before the one now sounding - so this test answered for last bar,
      // queued nothing when the two happened to match, and the bar that followed
      // replayed what was already going. That is the chain repeat, and it is why
      // it took a particular interleave to see.
      // [6B] Same bar line, same restart - a chain running with pCh on FREE takes
      // this route too. With pCh on SYNC nextPatternReady is set instead and the
      // ISR zeroes the phases itself, which is why the flag is not raised there.
      if (PlayingPattern() != nextPattern && !nextPatternReady){
        selectedPatternChanged = TRUE;
        barLineChange = TRUE;
      }
      trackPosNeedIncremante = FALSE;
    }
  }

  //======================================================================================================================================================================================================================================================================
  //------------------------------------Update pattern track load/save-----------------------
  if (selectedTrackChanged)
  {
    selectedTrackChanged = FALSE;
    needLcdUpdate = TRUE;
    trackNeedSaved = FALSE;
    LoadTrack(trk.next);
    trk.current = trk.next;
    nextPattern = track[trkBuffer].patternNbr[trk.pos];
    if(PlayingPattern() != nextPattern) selectedPatternChanged = TRUE;
    //trkBuffer = !trkBuffer;
  }
  if (trackNeedSaved && enterBtn.hold && !PanelBusy())                                      // [F25] ENTER-hold executes there
  {
    trackNeedSaved = FALSE;
    SaveTrack(trk.current);
    MsgShow(MSG_TRACK, 0);
    trackJustSaved = TRUE;
    timeSinceSaved = millis();
  }
  //this function is to not incremente trk.pos when released enterBtn after Saved track
  if (millis() - timeSinceSaved > HOLD_TIME){                                               
    trackJustSaved = FALSE;
  }

  // [5B] Reconcile an armed swap with what is now selected.
  //
  // A pattern change is armed a whole bar before it lands, and until [5B] nothing
  // revisited it. Pick a new chain in that bar and the old one's next pattern was
  // still loaded and still armed, so it played once before the new chain took
  // over: with 1-2 looping, selecting 2-3 while 2 played gave 1, then 2-3.
  //
  // The selection code could not catch it on its own. Choosing chain 2-3 while
  // pattern 2 plays sets nextPattern to 2 - the pattern already playing - so its
  // `curPattern != nextPattern` test says nothing has changed and arms nothing.
  // Nothing had changed about which pattern to play NEXT; what changed was that
  // the armed one was no longer wanted, and that is a different question.
  //
  // Asked here, once, after every path that can move nextPattern - the selection
  // blocks, the bar-end chain advance, a track change, program change. One place,
  // because the fault is not in any of them: it is in the gap between choosing
  // and landing, which they all share.
  if (nextPatternReady && armedPattern != nextPattern){
    if (nextPattern == PlayingPattern()){        // [5E]
      // Nothing to change to any more - the armed pattern is simply cancelled and
      // what is playing keeps playing. Interrupts off: the ISR tests and clears
      // this flag, and a swap landing between the read and the write would leave
      // us clearing a flag that had already fired.
      uint8_t sreg = SREG;
      cli();
      nextPatternReady = FALSE;
      SREG = sreg;
    }
    else selectedPatternChanged = TRUE;   // load the one now wanted instead
  }

  if (selectedPatternChanged)
  {
    selectedPatternChanged = FALSE;
    needLcdUpdate = TRUE;//selected pattern changed so we need to update display
    patternNeedSaved = FALSE;
    // [F4] The snapshots describe the pattern being left. Restoring one into a
    // different pattern would be corruption dressed up as a feature.
    UndoDiscard();

    // [B-26] Disarm before touching the back buffer. LoadPattern() blocks for
    // 30-50 ms on I2C; if the bar line fell inside that window while the ISR
    // was still armed, it would flip to a half-written buffer.
    nextPatternReady = FALSE;

    // Prepare the BACK buffer completely, while the current pattern keeps
    // playing. Previously all of this ran AFTER the bar line, so step 0 - and
    // step 1 at high tempo or 1/32 - still fired from the outgoing pattern.
    LoadPattern(nextPattern);                    // loads into pattern[!ptrnBuffer]
    InitPatternBuf(!ptrnBuffer);
    SetHHPatternBuf(!ptrnBuffer);                // [B-21] InstToStepWord() removed

    // [3C] Tap mode was missing from this list, so a pattern chosen there landed
    // mid-bar however pCh was set - and the BANK+encoder scroll made that easy to
    // meet, because it works in every mode. Tap is a pattern mode; a change made
    // in it is the same change.
    const boolean syncable = (curSeqMode == PTRN_PLAY || curSeqMode == PTRN_STEP
                           || curSeqMode == PTRN_TAP  || curSeqMode == MUTE
                           || (curSeqMode == TRACK_PLAY && trackChangeManual));
    trackChangeManual = FALSE;
    // [6B] Consumed here whatever route the change takes, the same as
    // trackChangeManual above: a flag that outlives the change it describes would
    // restart the lanes on the next unrelated one.
    const boolean atBarLine = barLineChange;
    barLineChange = FALSE;
    if (isRunning && seq.ptrnChangeSync == SYNC && syncable){
      armedPattern = nextPattern;                // [5A] what this swap will start
      nextPatternReady = TRUE;                   // ISR flips it exactly on the bar line
    }
    else {
      // Free change, or stopped: take effect immediately. The flip must not be
      // split by the Timer1 ISR.
      armedPattern = nextPattern;                // [5A] same fact, recorded the same way
      uint8_t sreg = SREG;
      cli();
      ptrnBuffer = !ptrnBuffer;
      muxArmedForNext = FALSE;                   // anything armed came from the old pattern
      swappedPattern = armedPattern;             // [5A] and captured by the swap
      patternSwapped = TRUE;
      SREG = sreg;
      // [F27] A free change lands mid-bar, so the lanes anchor to where the
      // playhead is rather than restarting - the bar has not restarted either.
      //
      // [6B] Except when it has. A track advance and a chain advance are queued
      // by the bar line itself and then taken free, so the pattern underneath the
      // lanes changes at step zero - the same moment, and for the same reason, as
      // the synced swap in Clock.inc that restarts them outright. Folding there
      // carried the outgoing pattern's phase into the incoming one, so every lane
      // with a length of its own opened the new pattern part-way through itself.
      if (atBarLine){
        for (byte z = 0; z < NBR_INST; z++) lanePhase[z] = 0;
        lanePhaseFresh = TRUE;                 // the step about to fire is phase 0
      }
      else LaneSyncAll();
    }
  }

  // [B-05] Finalising a swap - main-loop work that used to sit inside the Timer1
  // ISR - moved above the bar-end commit. See the note there. [3F]

  if (patternWasEdited)
  {//update Pattern
    patternWasEdited = FALSE;
    SetHHPattern();                                                                 // [B-21] InstToStepWord() removed
    patternNeedSaved = TRUE;
    // Serial.println("patternupdated");
  }

  // [3C] CLEAR+ENTER puts the pattern back to what is on the chip. CLEAR is the
  // panel's undo-shaped key and ENTER is the saved state, so the pair reads as
  // "back to saved" - and it has to be tested before the save below, or the same
  // press would write the mangled version out first.
  // [3N] Not on a config page. ENTER there means "save the setup" - SeqConf.inc
  // says so and acts on it - but PanelBusy() only counts the paste page, so this
  // ran as well and the one press did both, or did the pattern instead when the
  // setup had nothing to write. Which looked like a config save that worked
  // occasionally. One key, one meaning, decided by where you are.
  if (enterBtn.justPressed && BtnHeld(&clearBtn) && !shiftBtn && !instBtn && !PanelBusy() && !seq.configMode){
    RevertToSaved();
    MsgShow(MSG_REVERT, 0);
  }
  else if (patternNeedSaved && enterBtn.justPressed && !instBtn && !shiftBtn && !BtnHeld(&clearBtn) && !PanelBusy() && !seq.configMode)    // [F26][F25]
  {
    patternNeedSaved = FALSE;
    SavePattern(curPattern);//pattern saved
    MsgShow(MSG_SAVED, 0);
  }

  // [SysEx] A restore has written new bytes into the EEprom behind the copy in
  // RAM. Wait until the transfer has been quiet for 500 ms - a full restore is
  // ~900 page messages and reloading after each one would crawl - then pull the
  // current pattern back in through the normal path.
  if (patternRestored && (millis() - timeSinceRestore > 500)){
    patternRestored = FALSE;
    // A restored track is as stale in RAM as a restored pattern, and it is the
    // one that decides which patterns play next.
    if (trackRestored){
      trackRestored = FALSE;
      LoadTrack(trk.current);
    }
    nextPattern = curPattern;
    selectedPatternChanged = TRUE;
    MsgShow(MSG_SAVED, 0);
  }

  if (enterBtn.justRelease) needLcdUpdate = TRUE;

  //////////////////////////MODE MUTE//////////////////////////////////////

  if (curSeqMode == MUTE && !PanelBusy() && !seq.configMode){
    if (StepModifierHeld()){
      // nothing here acts on this press
    }
    // [3C] CLEAR + a pad solos that voice, which is the same sentence the group
    // gesture makes one word further along: a modifier says what kind of change,
    // the pad says which voice. Pressing MUTE to get here and then reaching for
    // a different key to name the instrument was the odd one out.
    //
    // It is a mask, so several voices can be soloed together - soloInst has
    // always been one and ActiveMute() has always inverted it as one. Pressing a
    // soloed voice again drops it, and dropping the last one leaves solo.
    else if (BtnHeld(&clearBtn)){
      if (inStepPressed){
        soloInst ^= MuteMaskFromButtons(inStepPressed, seq.muteModeHH);
        needLcdUpdate = TRUE;
      }
    }
    else if (instBtn){                                                   // [3C] was CLEAR
      // FALSE: a group always names the hi-hat lanes separately. See
      // MuteMaskFromButtons() for why this one place differs.
      if (inStepPressed) mutePending ^= MuteMaskFromButtons(inStepPressed, FALSE);
    }
    else {
      if (mutePending){
        muteInst ^= mutePending;
        mutePending = 0;
        needLcdUpdate = TRUE;
      }
      MuteButtonGet();
    }

    // The encoder press is "everything back on". Solo goes with the mutes: it is
    // the same question - which voices can be heard - and leaving one of the two
    // standing after a gesture that means "clear it" is how you end up hunting
    // for why the machine is still nearly silent.
    // [3Z] The press, not the level. Held down, this cleared the mutes on every
    // pass - so a pad pressed while the encoder was still down was undone as fast
    // as it was set, and the row would not take an edit until the hand came off.
    if (encBtn.justPressed){
      muteInst = 0;                                                                   // [B-02] LEDs derive from this
      mutePending = 0;
      if (soloInst){ soloInst = 0; needLcdUpdate = TRUE; }
    }
  }
  else mutePending = 0;                       // leaving MUTE abandons an unapplied group
  

}


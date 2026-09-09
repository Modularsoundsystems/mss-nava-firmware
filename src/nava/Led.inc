//-------------------------------------------------
//                  NAVA v1.x
//              Sequencer Led
//-------------------------------------------------

////////////////////////Function//////////////////////

// [B-02] Derive mute/gate LED words from state so they cannot diverge.
// Walks the BUTTON map, not the instrument map: buttons 0/1 both drive BD,
// 2/3 both drive SD, etc., and muteLedsOrder[] already encodes that pairing.
unsigned int MuteLedsFromMask(unsigned int mask)
{
  unsigned int leds = 0;
  for (byte a = 0; a < NBR_STEP_BTN; a++){
    if (mask & (1U << muteOut[a])) leds |= muteLedsOrder[a];
  }
  return leds;
}


void SetLeds()
{
  //------------------------------Menu LEDS-------------------------------
  // [F28] Steady while held, blinking while latched - a lit LED with your hands
  // off the panel has to look different from one you are holding down.
  shiftLed = shiftLatchBtn.pressed ? HIGH : (shiftLatchBtn.latched ? blinkTempo : LOW);
  instLed  = instLatchBtn.pressed  ? HIGH : (instLatchBtn.latched  ? blinkTempo : LOW);
  clearLed = clearBtn.pressed      ? HIGH : (clearBtn.latched      ? blinkTempo : LOW);
  shufLed  = shufBtn.pressed       ? HIGH : (shufBtn.latched       ? blinkTempo : LOW);
  scaleLed = scaleBtn.pressed;
  // [F28] Steady while held, blinking while latched. Identical to "held" was
  // the wrong call: a lit LED with your hands off the panel has to look
  // different from one you are holding down, or you will not notice the mode.
  lastStepLed = lastStepBtn.pressed ? HIGH : (lastStepBtn.latched ? blinkTempo : LOW);
  if (isRunning){
    menuLed = (LED_PLAY & ~LED_STOP) | shiftLed<<1 | instLed<<2 | clearLed<<3 | shufLed<<4 | lastStepLed<<5 | scaleLed<<6 ;   // parenthesised, same precedence
    //stepLeds = pattern[ptrnBuffer].inst[curInst] ^ (blinkFast << curStep-1);
  }
  else{
    menuLed = (LED_PLAY*blinkTempo) | LED_STOP | shiftLed<<1 | instLed<<2 | clearLed<<3 | shufLed<<4 | lastStepLed<<5 | scaleLed<<6;
    //stepLeds = pattern[ptrnBuffer].inst[curInst] ^ (blinkTempo << curStep);
  }

  //------------------------------Config LEDS-------------------------------
  trackLed =((curSeqMode == TRACK_WRITE)? blinkTempo:((curSeqMode == TRACK_PLAY)? 1:0));
  ptrnLed =((curSeqMode == PTRN_STEP)? blinkTempo:((curSeqMode == PTRN_PLAY)? 1:0));
  // [F9] Blinking means armed - the recording sense of a blinking indicator -
  // and steady means tap mode with recording off.
  tapLed =((curSeqMode == PTRN_TAP)? (tapRecordArm ? blinkTempo : HIGH):((tapBtn.pressed) ? 1:0));
  if (keyboardMode){
    backLed = HIGH;
    fwdLed = HIGH;
  }
  else{
    backLed = backBtn.pressed;
    fwdLed = fwdBtn.pressed;
  }

  dirLed = dirBtn.pressed;
  scaleLeds = 1 << scaleBtn.counter;
  guideLed = guideBtn.pressed  ? HIGH
           : guideBtn.latched  ? blinkTempo
           : guideBtn.counter;
  if (curSeqMode == PTRN_STEP && curInst == EXT_INST) guideLed = blinkTempo;
  bankLed = bankBtn.pressed;

  if (instBtn && curInst == TOTAL_ACC) enterLed = HIGH;
  else if (patternNeedSaved || trackNeedSaved || seq.setupNeedSaved) enterLed = blinkTempo;
  else enterLed = LOW;

  if (curSeqMode == MUTE) muteLed = HIGH;
  else if (soloInst) muteLed = blinkTempo;                  // [F16] solo is a mode; say so
  else muteLed = LOW;

  if (seq.configMode) tempoLed = blinkTempo;
  else  tempoLed = tempoBtn.pressed;

  configLed = numLed | scaleLeds<<1 | trackLed<<5 | backLed<<6 | fwdLed<<7 | enterLed<<8 | ptrnLed<<9 | tapLed<<10 | dirLed<<11 |guideLed<<12 | bankLed<<13 | muteLed<<14 | tempoLed<<15;

  //------------------------------Step LEDS-------------------------------
  
  //Update inst selected Leds
  switch (curInst){
  case BD:
    instSlctLed = 0x03;
    break;
  case SD:
    instSlctLed = 0x0C;
    break;
  case LT:
    instSlctLed = 0x30;
    break;
  case MT:
    instSlctLed = 0xC0;
    break;
  case HT:
    instSlctLed = 0x300;
    break;
  case RM:
    instSlctLed = 0x400;
    break;
  case HC:
    instSlctLed = 0x800;
    break;
  // [4V][4W] The LEDs say which press would get you here, so they follow the
  // setting: under 2nd one pad each, under BTH both pads for either - which was
  // always right for a chord and is exactly wrong for a single press.
  case CH:
    instSlctLed = (seq.ohSelect == OH_SEL_2ND) ? 0x1000 : 0x3000;
    break;
  case RIDE:
    instSlctLed = 0x8000;
    break;
  case CRASH:
    instSlctLed = 0x4000;
    break;
  case OH:
    instSlctLed = (seq.ohSelect == OH_SEL_2ND) ? 0x2000 : 0x3000;
    break;
  case TRIG_OUT:
  case EXT_INST:
  case TOTAL_ACC:
    instSlctLed = 0;//Clear step leds when TOTAL_ACC... selected
    break;
  }

  // [F26] The name cursor owns the step LEDs while the editor is open. Using
  // them instead of a blinking LCD block keeps the panel doing the pointing
  // and saves redrawing the display several times a second.
  if (nameEditMode){
    SetDoutLed(1U << nameCursor, configLed, menuLed);
    return;
  }


  {
    const boolean trigMon = (tempoBtn.pressed && !instBtn && !shiftBtn && !seq.configMode);
    static boolean trigMonWas = FALSE;
    static unsigned int trigShown = 0;
    static byte trigHold = 0;

    // The word accumulates whether anyone is watching or not, so on the way in it
    // holds the union of everything played since the last look - which would open
    // the page with every pad lit once. Emptied on the edge instead.
    if (trigMon != trigMonWas){
      trigMonWas = trigMon;
      if (trigMon){
        uint8_t sreg = SREG;
        cli();
        trigMonitor = 0;
        SREG = sreg;
        trigShown = 0;
        trigHold  = TRIG_MON_HOLD;
      }
    }

    if (trigMon){
      if (trigHold) trigHold--;
      else {
        uint8_t sreg = SREG;
        cli();
        trigShown   = trigMonitor;   // read and empty together, or a hit that
        trigMonitor = 0;             // lands between the two is lost
        SREG = sreg;
        trigHold = TRIG_MON_HOLD;
      }
      // Through the same pad map the mute page uses, so a voice's two pads both
      // light for it and the row means the same thing it does everywhere else.
      SetDoutLed(MuteLedsFromMask(trigShown), configLed, menuLed);
      return;
    }
  }

  // [3C] The same preview on the pads: where the scroll has got to inside its
  // bank, with the one you are on blinking underneath it.
  if ((bankBtn.pressed && encPtrnArmed) || (trkBtn.pressed && encTrkArmed)){
    const boolean isTrk = (trkBtn.pressed && encTrkArmed);
    const byte sel = isTrk ? encTrkScroll : (byte)(encPtrnScroll % NBR_PATTERN);
    const byte cur = isTrk ? trk.current   : (byte)(curPattern % NBR_PATTERN);
    unsigned int m = 1U << sel;
    const boolean sameBank = isTrk || ((encPtrnScroll / NBR_PATTERN) == (curPattern / NBR_PATTERN));
    if (sameBank && blinkFast) m |= 1U << cur;
    SetDoutLed(m, configLed, menuLed);
    return;
  }

  // [F26] Where the cursor is inside its bank, against the entry actually
  // playing - the two are usually different while browsing, and that difference
  // is the whole state of the page.
  if (browseMode){
    const byte cur = browseIsTrack ? trk.current : curPattern;
    unsigned int m = 1U << (browseIsTrack ? browseSel : (browseSel % NBR_PATTERN));
    if (browseIsTrack || (cur / NBR_PATTERN) == (browseSel / NBR_PATTERN)){
      if (blinkFast) m |= 1U << (browseIsTrack ? cur : (cur % NBR_PATTERN));
    }
    SetDoutLed(m, configLed, menuLed);
    return;
  }

  // [F18] Which steps of this instrument carry a condition, with the one being
  // edited blinking. A condition is invisible in the pattern otherwise - the
  // trig looks identical whether it plays every bar or one in eight.
  // [F21] On the links page the source lane is what wants seeing - a rule is
  // aimed at a part, so show the part.
  if (condMode && condPage == 1){
    SetDoutLed(pattern[ptrnBuffer].inst[curInst] ^ (blinkFast << curStep), configLed, menuLed);
    return;
  }

  if (condMode){
    unsigned int m = 0;
    for (byte s = 0; s < NBR_STEP; s++){
      if (pattern[ptrnBuffer].stepCond[curInst][s]) bitSet(m, s);
    }
    if (condStep) m ^= (unsigned int)(blinkFast << (condStep - 1));
    SetDoutLed(m, configLed, menuLed);
    return;
  }

  // [3Q] Both cursors, for any page showing one instrument's steps. The bright
  // one is where that lane actually is; the master step is added at
  // blinkVeryFast, which reads as a dimmer light rather than a second blink, so
  // the bar reference stays visible without competing with it.
  //
  // Only when they have parted company: on a lane that follows the pattern they
  // are the same step, and XORing it twice would cancel it out.
  //
  // Step edit has drawn these two since F27. The generator pages showed the
  // master step alone - which on a polyrhythmic lane is the one position that
  // does not tell you where the lane you are shaping has got to.


  // [F29] The mask itself, so the hand and the eye agree about which toggle is
  // which without counting cells on the display.
  //
  // [5G] While the chord is held, which is now the only place the mask is edited.
  // The eight groups are the low eight bits of pasteMask in the same order as
  // the pads that toggle them, so it goes straight to the LEDs with nothing in
  // between - lit is carried, dark is not, the same thing the upper and lower
  // case on the display are saying.
  if (copyHold || pasteHold){
    SetDoutLed((unsigned int)seq.pasteMask, configLed, menuLed);
    return;
  }

  // [F23] Which of the three the encoder is on. The dump itself drives these
  // LEDs directly as it runs, so they go from being the choice to being the
  // progress bar without the page having to say so.
  if (seq.configMode && seq.configPage == CONF_PAGE_SYSEX){
    SetDoutLed((unsigned int)1 << dumpWhat, configLed, menuLed);
    return;
  }

  // [F25] Which part of the bank the pending operation covers, eight patterns
  // per LED. Sixteen indicators cannot show 128 slots exactly, but they can
  // show that a range reaches somewhere it was not meant to.
  if (utilMode){
    unsigned int span = 0;
    const byte last = (byte)(utilFirst + utilCount - 1);
    for (byte b = 0; b < 16; b++){
      const byte blo = (byte)(b * 8);
      if (utilFirst <= (byte)(blo + 7) && last >= blo) span |= (1U << b);
    }
    SetDoutLed(span, configLed, menuLed);
    return;
  }

  //Update step Leds depends sequencer Mode
  switch (curSeqMode){
    //------------------------------
    unsigned int temp;// tmep data of stepLeds;
  case TRACK_PLAY:
    if (isRunning) stepLeds = (1 << trk.current) ^ (blinkFast << curStep); 
    else stepLeds = blinkTempo << trk.current;
    break;
  case TRACK_WRITE:
    if (bankBtn.pressed){
      stepLeds = 1 << curBank;
    }
    else if (numBtn.pressed){
      stepLeds = 1 << trk.current;
    }
    else{
      if (isRunning) stepLeds = (1 <<(curPattern % NBR_PATTERN)) ^ (blinkFast<< curStep); 
      else stepLeds = blinkTempo << (curPattern % NBR_PATTERN); 
    }
    break;
    //------------------------------
  case PTRN_PLAY:
    if (bankBtn.pressed){
      stepLeds = 1 << curBank;
    }
    else{
      if (isRunning){
        // [4Y] The pattern PLAYING, not the one queued.
        //
        // These marked nextPattern, which outside a chain is the same pattern
        // almost always - so it read correctly and was wrong the whole time. In a
        // chain nextPattern is the one due at the next bar line, advanced by the
        // bar-end commit, so the lit pad ran a pattern ahead of the music for as
        // long as the chain played. Reported as exactly that.
        //
        // The queued pattern is still worth seeing - in SYN mode a change waits a
        // whole bar and nothing else says it is coming - so it keeps a marker of
        // its own, on the faster blink. Two rates, two facts, neither pretending
        // to be the other.
        const unsigned int playing = 1U << (PlayingPattern() % NBR_PATTERN);   // [5E]
        const unsigned int queued  = (nextPattern != PlayingPattern())
                                     ? (1U << (nextPattern % NBR_PATTERN)) : 0;
        if (group.length){
          temp = 0;
          for (int a = 0; a <= group.length; a++){
            bitSet(temp,(group.firstPattern % NBR_PATTERN) + a);
          }
          stepLeds = (temp & ~(blinkTempo ? 0 : playing)) ^ (blinkFast<< curStep);
        }
        else {
          stepLeds = (blinkTempo ? playing : 0) ^ (blinkFast<< curStep);
        }
        if (queued && blinkFast) stepLeds |= queued;
      }
      else if (!isRunning){
        // if (group.length){
        temp = 0;
//Serial.println(group.length);
        for (int a = 0; a <= group.length; a++){
          bitSet(temp,(group.firstPattern % NBR_PATTERN) + a);
        }
        stepLeds = temp & ~(!blinkTempo << (nextPattern % NBR_PATTERN));
        /* }
         else {
         stepLeds = blinkTempo << (curPattern % NBR_PATTERN); 
         }*/
      }
    }
    break;
    //------------------------------
  case PTRN_STEP:
  case PTRN_TAP:
    // [F8] ...but not while an insert or delete is being aimed. INST there means
    // "this lane only" and the pad is choosing a position, so the useful thing to
    // watch is the step appearing or vanishing, not the instrument map.
    //display selected inst on steps Leds
    if (instBtn && !CutKeyHeld()){
      if(flagLedIntensity >= 8){
        stepLeds = instSlctLed ;
        flagLedIntensity = 0;
      }
      else{
        if (curInst == OH) stepLeds = instSlctLed & LED_MASK_OH;
        else if (curFlam) stepLeds = instSlctLed;                                                                  // [zabox] [1.027] flam
        else stepLeds = instSlctLed & LED_MASK;
        flagLedIntensity++;
      }
    }
    else if (BtnHeld(&shufBtn)) {                                                                                    // [zabox] [1.028] flam
      if (flagLedIntensity >= 8) {
        stepLeds = ~(1 << 7);
        flagLedIntensity = 0;
      }
      else {
        // [3E] shuffle 0 is "no shuffle" and no step button owns it, so nothing
        // should light. It used to shift by shuffle - 1, which at 0 is a shift
        // by -1 - undefined, and on AVR it lands as a shift by 31 that happens
        // to give 0. Correct by accident, and only while the compiler agreed.
        const byte shuf = pattern[ptrnBuffer].shuffle;
        stepLeds = (shuf ? (1U << (shuf - 1)) : 0U) | (1U << (pattern[ptrnBuffer].flam + 8));
        flagLedIntensity++;
      }
      
    }
    else if(isRunning && !instBtn){
      stepLedsHigh = stepLedsLow = 0;//initialize step Leds variable 
      for (int stp = 0; stp < NBR_STEP; stp++){
        // [B-33] Light the steps that belong to the mode being edited: in flam
        // mode the flam steps, otherwise the plain ones. This tested velocity
        // bit 7, which format v2 retired in favour of its own bitmask - so in
        // flam mode the test was never true, the else branch cleared both LED
        // words, and no step lit whatever you did to it.
        const boolean isFlam = bitRead(pattern[ptrnBuffer].flamMask[curInst], stp);   // [zabox] [1.027] flam
        if (((curFlam != 0) == isFlam) && bitRead(pattern[ptrnBuffer].inst[curInst], stp)) {
          if (pattern[ptrnBuffer].velocity[curInst][stp] > instVelLow[curInst]) bitSet(stepLedsHigh, stp);
          else                                                                  bitSet(stepLedsLow, stp);
        }
        else {
          bitClear(stepLedsHigh, stp);
          bitClear(stepLedsLow, stp);
        }
      }
      // [F27] Two cursors, because with per-instrument lengths there are two
      // positions worth knowing. The bright blinking one is where the lane being
      // edited actually is; the master step is added at blinkVeryFast, which
      // toggles fast enough to read as a dimmer light rather than a second
      // blink, so the bar reference stays visible without competing.
      //
      // Only when they have parted company - on a lane that follows the pattern
      // they are the same step, and drawing it twice would just cancel out.
      byte    lanePos  = curStep;
      boolean laneShow = TRUE;
      if (pattern[ptrnBuffer].lane[curInst][0]){
        const byte p = lanePhase[curInst];
        if (p < NBR_STEP) lanePos = p;
        else              laneShow = FALSE;   // inside its padding: off the visible row
      }
      const unsigned int laneCur   = laneShow ? ((unsigned int)blinkFast << lanePos) : 0;
      const unsigned int masterCur = (lanePos != curStep || !laneShow)
                                     ? ((unsigned int)blinkVeryFast << curStep) : 0;

      //this function is to fade low velocity leds
      if(flagLedIntensity >= 8){
        stepLeds = stepLedsHigh | (stepLedsLow ^ laneCur);//B1111111111111111;   // parenthesised, same precedence
        flagLedIntensity = 0;
      }
      else{
        stepLeds = stepLedsHigh ^ laneCur;//B1010101010101010;
        flagLedIntensity++;
      }
      stepLeds ^= masterCur;
    }
    else if (!isRunning && !instBtn){
      //Display Bank number
      if (bankBtn.pressed){
        stepLeds = 1 << curBank;
      }
      //display selected pattern
      else{
        // if (group.length)
        temp = 0;
       // Serial.println(group.length);
        for (int a = 0; a <= group.length; a++){
          bitSet(temp,(group.firstPattern % NBR_PATTERN) + a);
        }
        stepLeds = temp & ~(!blinkTempo << (curPattern % NBR_PATTERN));
      }
    }
    break;
    //------------------------------
  case MUTE:
    // [B-02] Derived, not stored. Shows tempMuteInst too, so a CLEAR-hold is
    // now visible rather than silently desynchronising the display.
    stepLeds = MuteLedsFromMask(ActiveMute());
    // [F13] Members of a pending group blink, so what is about to change is
    // visible before it changes rather than after.
    if (mutePending && blinkFast) stepLeds ^= MuteLedsFromMask(mutePending);
    // [3C] Soloed voices blink too, and against an otherwise dark row - solo
    // silences everything else, so without this the display says "everything is
    // muted" and not which one thing is not.
    if (soloInst && blinkTempo) stepLeds ^= MuteLedsFromMask(soloInst);
    break;
  }
  //Send OUTPUTS now !
  SetDoutLed(stepLeds, configLed , menuLed);
}


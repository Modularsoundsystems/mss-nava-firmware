//-------------------------------------------------
//                  NAVA v1.x
//                 Encoder function
//-------------------------------------------------

////////////////////////Function//////////////////////

void EncGet()
{
  // [3E] Read the encoder once per pass, before any branch below can return.
  // Every EncGet(value, dif) in the firmware is reached from here, so by the
  // time one is called the detent for this pass is already decided.
  //
  // The edge is consumed whether or not the branch that runs this pass looks at
  // it - the same rule [3A] settled on for buttons, and for the same reason: a
  // detent stored up while a page ignored the knob, arriving on the value you
  // moved to next, is worse than a detent dropped.
  EncScan();

  //////////////////////////////////NAME EDIT [F26]/////////////////////////////////
  if (nameEditMode){
    NameEditEncoder();
    return;
  }

  //////////////////////////////////BROWSER [F26]///////////////////////////////////
  if (browseMode){
    BrowseEncoder();
    return;
  }

  //////////////////////////////////PATTERN UTILITIES [F25]/////////////////////////
  if (utilMode){
    UtilEncoder();
    return;
  }

  //////////////////////////////////TRIG CONDITIONS [F18]///////////////////////////
  if (condMode){
    CondEncoder();
    return;
  }

  // [F29] The paste page has no encoder function, and without this the
  // TOTAL_ACC branch below would happily edit total accent underneath it.
  // [4X] The firmware page has none either, and has to say so here for the same
  // reason: a page with nothing to edit is not a page that lets the knob fall
  // through to whatever is behind it.
  if (seq.configMode && seq.configPage == CONF_PAGE_FW) return;

  // [5G] The paste mask has no encoder function either, and it is reachable from
  // anywhere now rather than from one config page - so the guard has to be here,
  // ahead of every held-modifier branch below, or holding the chord would edit
  // whatever those would have edited.
  if (copyHold || pasteHold) return;

  // [F23] The dump page: the encoder chooses what to send, and nothing else on
  // the page reads it.
  if (seq.configMode && seq.configPage == CONF_PAGE_SYSEX){
    const byte v = (byte)constrain(EncGet(dumpWhat, 1), 0, DUMP_MAX);
    if (v != dumpWhat){ dumpWhat = v; needLcdUpdate = TRUE; }
    return;
  }

  //////////////////////////////////HELD MODIFIERS [3C]/////////////////////////////
  // The encoder edits whatever is being held. Nothing here is reachable while a
  // page is open - those return above - so the rule is "a page owns the knob,
  // otherwise the modifier does, otherwise it is the tempo".

  // INST: walk the voices. Live, this is the difference between finding pad 11
  // and turning a knob, and it reaches the voices whose pads are shared.
  //
  // [3E] Not from total accent. voiceOrder[] is the eleven voices that own a
  // lane and total accent is not one of them, so the search below fell through
  // with at = 0 and the first detent jumped to BD - and since INST is still
  // under your thumb from the INST+ENTER that selected total accent, the first
  // thing you did on that page threw you off it. That is the "it only shows for
  // an instant" fault: the screen was never the problem, the knob was. INST+pad
  // still leaves, which is the gesture that names where it is going.
  // [5Q] Every voice that is not on the list, and only on an actual detent.
  //
  // [3E] excluded TOTAL_ACC and stopped there, but TRIG_OUT and EXT_INST are not
  // in voiceOrder[] either - so the search fell through with at = 0, and with no
  // detent at all EncGet() returned 0, giving voiceOrder[0] = BD. Merely HOLDING
  // INST rewrote curInst to BD on every pass. That is why INST+STOP looked like it
  // did nothing: it set TRIG_OUT, and the next pass with INST still down took it
  // straight back off again. Same for INST+SHIFT+GUIDE and the external lane.
  //
  // Two guards, either of which alone would have been enough, and both of which
  // say something true: nothing happens without a detent, and a voice that is not
  // on the list is not a place you can walk from. INST+pad still leaves, which is
  // the gesture that names where it is going.
  if (instBtn && encStep && (curSeqMode == PTRN_STEP || curSeqMode == PTRN_TAP)){
    byte at = 0xFF;
    for (byte i = 0; i < NBR_VOICE; i++) if (voiceOrder[i] == curInst){ at = i; break; }
    const byte v = (at == 0xFF) ? 0xFF
                 : (byte)constrain(EncGet((int)at, 1), 0, NBR_VOICE - 1);
    if (v != 0xFF && voiceOrder[v] != curInst){
      curInst = voiceOrder[v];
      curFlam = 0;
      needLcdUpdate = TRUE;
    }
    return;
  }

  // BANK: walk the patterns. Committed when the key comes up - see Seq.inc.
  // [3C] Not under SHIFT. SHIFT+BANK is COPY and SHIFT+TRK is WRITE - holding
  // either of those and turning the knob would queue a pattern change on top of
  // the gesture you meant, which lands as "copy did nothing" because the pattern
  // moved out from under it.
  // [5G] Not while BANK is holding a copy open. Without this, letting go of
  // SHIFT hands the encoder, the screen and the sixteen LEDs to the pattern
  // scroll - and with encPtrnArmed never set, the commit on BANK's release
  // cannot fire either, so this one guard covers both halves of that collision.
  if (bankBtn.pressed && !instBtn && !shiftBtn && !copyHold){
    if (!encPtrnArmed){ encPtrnScroll = curPattern; encPtrnArmed = TRUE; NamePeek(encPtrnScroll, FALSE); }
    // [4F] Not inside constrain(): that macro names its first argument three
    const int t = EncGet((int)encPtrnScroll, 1);
    const byte v = (byte)constrain(t, 0, MAX_PTRN - 1);
    if (v != encPtrnScroll){ encPtrnScroll = v; NamePeek(v, FALSE); needLcdUpdate = TRUE; }
    return;
  }

  // TRACK PLAY held: walk the tracks, same deal.
  if (trkBtn.pressed && !shiftBtn && !instBtn){
    if (!encTrkArmed){ encTrkScroll = trk.current; encTrkArmed = TRUE; NamePeek(encTrkScroll, TRUE); }
    const byte v = (byte)constrain(EncGet(encTrkScroll, 1), 0, MAX_TRACK - 1);
    if (v != encTrkScroll){ encTrkScroll = v; NamePeek(v, TRUE); needLcdUpdate = TRUE; }
    return;
  }

  // LAST STEP: the pattern's length, which the pads can only set absolutely.
  // A knob can nudge it, which is what shortening a bar live actually wants.
  if (lastStepBtn.pressed && !instBtn){                    // [6M] no latch to honour
    const byte v = (byte)constrain(EncGet(pattern[ptrnBuffer].length, 1), 0, NBR_STEP - 1);
    if (v != pattern[ptrnBuffer].length){
      if (!lastStepEncDirty){ UndoPush(); lastStepEncDirty = TRUE; }
      pattern[ptrnBuffer].length = v;
      patternWasEdited = TRUE;
      needLcdUpdate = TRUE;
    }
    return;
  }

  // [F19] SHUF held is already the swing page - the pattern's own shuffle is on
  // the step buttons there, so the encoder is free and lands on the one value
  // the step buttons cannot reach: the swing of the instrument you have
  // selected. 0 is "follow the pattern", which is what every instrument starts
  // as, so the page opens saying the pattern is in charge and you take voices
  // off it one at a time.
  //
  // [3E] Moved up here with the other held modifiers, where the file's own rule
  // says it belongs - a page owns the knob. Further down the chain it sat below
  // the total accent branch, so with total accent selected the swing page opened
  // and the encoder still edited the accent.
  //
  // Carries the gates it used to get from its position in that chain: this page
  // only draws in the two pattern modes and only outside the config pages, and a
  // branch that has moved ahead of the ones that used to shadow it has to say so
  // itself.
  if (BtnHeld(&shufBtn) && !shiftBtn && !seq.configMode
      && (curSeqMode == PTRN_STEP || curSeqMode == PTRN_TAP)){          // [F3]
    byte v = pattern[ptrnBuffer].instSwing[curInst];
    v = (byte)EncGet(v, 1);
    v = constrain(v, 0, MAX_SHUF_TYPE);
    if (v != pattern[ptrnBuffer].instSwing[curInst]){
      pattern[ptrnBuffer].instSwing[curInst] = v;
      patternWasEdited = TRUE;
      needLcdUpdate = TRUE;
    }
    return;
  }

  //////////////////////////////////TOTAL ACCENT////////////////////////////////////
  // [3E] Tap as well as step. The screen has always drawn in both, so in tap the
  // page came up and the knob did nothing.
  if (curInst == TOTAL_ACC && (curSeqMode == PTRN_STEP || curSeqMode == PTRN_TAP)){
    pattern[ptrnBuffer].totalAcc = EncGet(pattern[ptrnBuffer].totalAcc, 1);
    pattern[ptrnBuffer].totalAcc = constrain(pattern[ptrnBuffer].totalAcc, 0, 13);
    static byte prevTotalAcc;
    if (pattern[ptrnBuffer].totalAcc != prevTotalAcc){
      prevTotalAcc = pattern[ptrnBuffer].totalAcc;
      patternWasEdited = TRUE;
      needLcdUpdate = TRUE;
    }
  }
  ///////////////////////////////////TRACK WRITE////////////////////////////////////
  else if (curSeqMode == TRACK_WRITE && !tempoBtn.pressed && !seq.configMode){
    switch (curIndex){
      //track position
    case 0://track position
      if(instBtn) trk.pos = EncGet(trk.pos, 10);
      else trk.pos = EncGet(trk.pos, 1);
      // [B-35] Was 999, left over from v1's 1022-slot track. 3B-2 shrank
      // patternNbr[] to 496 and missed this, so a track position past 495
      // indexed up to 503 bytes beyond the array - straight into whatever
      // globals follow it, on a write.
      trk.pos = constrain(trk.pos, 0u, (unsigned int)(MAX_PTRN_TRACK - 1));
      // [B-16] The break used to sit INSIDE this if, with case 1..3 nested in it
      // too. Legal C - the switch jumps straight to the label - but it meant
      // that whenever curIndex was 0 and trk.pos had NOT changed, execution ran
      // off the end of the if and fell through into case 1, editing the track
      // pattern number, then case 2, editing the track length. Turning the
      // encoder on track position silently rewrote two other fields.
      static unsigned int prevTrkPos;
      if (trk.pos != prevTrkPos){
        prevTrkPos = trk.pos;
        nextPattern = track[trkBuffer].patternNbr[trk.pos];
        if(PlayingPattern() != nextPattern) selectedPatternChanged = TRUE;
        needLcdUpdate = TRUE;
      }
      break;
    case 1:// track pattern
      if(instBtn) nextPattern = EncGet(nextPattern, 16);
      else nextPattern = EncGet(nextPattern, 1);
      nextPattern = constrain(nextPattern, 0, MAX_PTRN - 1);
      static unsigned int prevNextPattern;
      if (nextPattern != prevNextPattern){
        prevNextPattern = nextPattern;
        if(PlayingPattern() != nextPattern) selectedPatternChanged = TRUE;
        needLcdUpdate = TRUE;
      }
      break;
    case 2://track length
      if(instBtn) track[trkBuffer].length = EncGet(track[trkBuffer].length, 10);
      else track[trkBuffer].length = EncGet(track[trkBuffer].length, 1);
      track[trkBuffer].length = constrain(track[trkBuffer].length, 0u, (unsigned int)(MAX_PTRN_TRACK - 1));   // [B-35]
      static unsigned int prevTrkLength;
      if (track[trkBuffer].length != prevTrkLength){
        prevTrkLength = track[trkBuffer].length;
        trackNeedSaved = TRUE;
        needLcdUpdate = TRUE;
      }
      break;
    case 3://track number
      //toDo
      break;
    }
  }

  ///////////////////////////////////KEYBOARD MODE////////////////////////////////////
  else if (keyboardMode){
    switch (curIndex){
      //track position
    case 0://external instrument note index
      if(instBtn) noteIndex = EncGet(noteIndex, 10);
      else noteIndex = EncGet(noteIndex, 1);
      noteIndex = constrain(noteIndex, 0, 99);
      // [B-16] Same misnesting as TRACK_WRITE above: with the break inside this
      // if, leaving the note index unchanged fell through into case 1 and
      // rewrote the external note itself, then case 2 and the stack length.
      static unsigned int prevNoteIndex;
      if (noteIndex != prevNoteIndex){
        prevNoteIndex = noteIndex;
        needLcdUpdate = TRUE;
      }
      break;
    case 1://external instrument note
      if(instBtn) pattern[ptrnBuffer].extNote[noteIndex] = EncGet(pattern[ptrnBuffer].extNote[noteIndex], 12);
      else pattern[ptrnBuffer].extNote[noteIndex] = EncGet(pattern[ptrnBuffer].extNote[noteIndex], 1);
      pattern[ptrnBuffer].extNote[noteIndex] = constrain(pattern[ptrnBuffer].extNote[noteIndex], 0, 127);
      static unsigned int prevExtNote;
      if (pattern[ptrnBuffer].extNote[noteIndex] != prevExtNote){
        prevExtNote = pattern[ptrnBuffer].extNote[noteIndex];
        needLcdUpdate = TRUE;
      }
      break;
    case 2://external instrument notes stack length
      if(instBtn) pattern[ptrnBuffer].extLength = EncGet(pattern[ptrnBuffer].extLength, 10);
      else pattern[ptrnBuffer].extLength = EncGet(pattern[ptrnBuffer].extLength, 1);
      pattern[ptrnBuffer].extLength = constrain(pattern[ptrnBuffer].extLength, 0, 99);
      static unsigned int prevExtLength;
      if (pattern[ptrnBuffer].extLength != prevExtLength){
        prevExtLength = pattern[ptrnBuffer].extLength;
        patternNeedSaved = TRUE;
        needLcdUpdate = TRUE;
      }
      break;
    case 3://octave
      keybOct = EncGet(keybOct, 1);
      keybOct = constrain(keybOct, 0, 7);
      static unsigned int prevKeybOct;
      if (keybOct != prevKeybOct){
        prevKeybOct = keybOct;
        needLcdUpdate = TRUE;
      }
      break;
    }
  }
  ///////////////////////////////////CONFIG MODE////////////////////////////////////
  else if (seq.configMode){                                                               //  [zabox] rewrite for two complete pages & no wrong encoder updates
    
    if (seq.configPage == CONF_PAGE_HAT){
      if (encStep){
        const boolean v = (encStep > 0);
        if (curIndex == 0 && v != seq.hhPriority){
          seq.hhPriority = v; seq.setupNeedSaved = TRUE; needLcdUpdate = TRUE;
        }
        else if (curIndex == 1 && v != seq.muteModeHH){
          seq.muteModeHH = v; seq.setupNeedSaved = TRUE; needLcdUpdate = TRUE;
        }
        else if (curIndex == 2 && (byte)v != seq.ohSelect){
          seq.ohSelect = (byte)v; seq.setupNeedSaved = TRUE; needLcdUpdate = TRUE;
        }
      }
      return;
    }

    if (seq.configPage == CONF_PAGE_SYNC) {    
      
  //---------------------Page 1----------------------------------------------------
    
    switch (curIndex){
      //track position
      case 0:
      
        seq.sync = EncGet(seq.sync, 1);                       //sync select
        seq.sync = constrain(seq.sync, 0, 2);
        static byte prevSeqSync;
        if (seq.sync != prevSeqSync){
          prevSeqSync = seq.sync;
          seq.syncChanged = TRUE;
          seq.setupNeedSaved = TRUE;
          needLcdUpdate = TRUE;
        }
        break;
      case 1:
  
        seq.defaultBpm = EncGet(seq.defaultBpm, 1);
        seq.defaultBpm = constrain(seq.defaultBpm, (unsigned int)MIN_BPM, (unsigned int)MAX_BPM);   // defaultBpm is unsigned
        static unsigned int prevDefaultBpm;
        if (seq.defaultBpm != prevDefaultBpm){
          prevDefaultBpm = seq.defaultBpm;
          seq.setupNeedSaved = TRUE;
          needLcdUpdate = TRUE;
        }
        break;
      case 2:
  
        seq.TXchannel = EncGet(seq.TXchannel, 1);
        seq.TXchannel = constrain(seq.TXchannel, 1, MIDI_CHAN_OFF);        // [5I] 17 is OFF
        static unsigned int prevTX;
        if (seq.TXchannel != prevTX){
          prevTX = seq.TXchannel;
          seq.setupNeedSaved = TRUE;
          needLcdUpdate = TRUE;
        }
        break;
      case 3:
        seq.RXchannel = EncGet(seq.RXchannel, 1);
        seq.RXchannel = constrain(seq.RXchannel, 1, MIDI_CHAN_OFF);        // [5I] 17 is OFF
        static unsigned int prevRX;
        if (seq.RXchannel != prevRX){
          prevRX = seq.RXchannel;
          // [5I] Never hand the library 17: read() treats anything at or above
          // MIDI_CHANNEL_OFF as "input disabled" and would take the clock with it.
          // The handlers gate themselves on seq.RXchannel, and no incoming message
          // carries channel 17, so OFF filters itself once the library is looking
          // at a real channel.
          MIDI.setInputChannel(seq.RXchannel > MAX_MIDI_CHAN ? 1 : seq.RXchannel);
          seq.setupNeedSaved = TRUE;
          needLcdUpdate = TRUE;
        }
        break;
      }
    }
    else if (seq.configPage == CONF_PAGE_BEHAV) {
            
  //---------------------Page 2----------------------------------------------------
    
    switch (curIndex){
      //track position
      case 0:
        seq.ptrnChangeSync = EncGet(seq.ptrnChangeSync, 1);               //pattern change sync select
        seq.ptrnChangeSync = constrain(seq.ptrnChangeSync, 0, 1);
        static boolean prevPtrnSyncChange;
        if (seq.ptrnChangeSync != prevPtrnSyncChange){
          prevPtrnSyncChange = seq.ptrnChangeSync;
          // seq.syncChanged = TRUE;
          seq.setupNeedSaved = TRUE;
          needLcdUpdate = TRUE;
        }
        break;
      case 1:
        seq.singleTrig = EncGet(seq.singleTrig, 1);                              // [F7]
        seq.singleTrig = constrain(seq.singleTrig, 0, 1);
        static boolean prev_singleTrig;
        if (seq.singleTrig != prev_singleTrig){
          prev_singleTrig = seq.singleTrig;
          seq.setupNeedSaved = TRUE;
          needLcdUpdate = TRUE;
        }
        break;
      case 2:                                                                    // [4B][4X]
        seq.tapOverMute = EncGet(seq.tapOverMute, 1);
        seq.tapOverMute = constrain(seq.tapOverMute, 0, 1);
        static boolean prev_tapOverMute;
        if (seq.tapOverMute != prev_tapOverMute){
          prev_tapOverMute = seq.tapOverMute;
          seq.setupNeedSaved = TRUE;
          needLcdUpdate = TRUE;
        }
        break;
      case 3:                                                                    // [6Z] cym
        seq.cymAccent = EncGet(seq.cymAccent, 1);
        seq.cymAccent = constrain(seq.cymAccent, 0, 1);
        static boolean prev_cymAccent;
        if (seq.cymAccent != prev_cymAccent){
          prev_cymAccent = seq.cymAccent;
          // The live buffer is re-normalised on the spot, so turning it off puts
          // the cymbals back to strong without waiting for a pattern load.
          InitPatternBuf(ptrnBuffer);
          patternWasEdited = TRUE;
          seq.setupNeedSaved = TRUE;
          needLcdUpdate = TRUE;
        }
        break;
      }
    }
    
    
  }

  // [F11] The roll rate, two ways. A pad held in tap mode claims the encoder -
  // that is the only gesture in the mode with a hand on a pad and a hand free,
  // and tempo is the last thing you would reach for mid-roll. SHIFT does the
  // same without a pad, so the rate can be set up before the roll rather than
  // discovered during it.
  else if (curSeqMode == PTRN_TAP
           && (shiftBtn || (inStepNow && !instBtn && !BtnHeld(&clearBtn)
                            && !BtnHeld(&shufBtn) && !BtnHeld(&lastStepBtn)
                            && !StepModifierHeld()))){
    rollRate = (byte)EncGet(rollRate, 1);
    rollRate = constrain(rollRate, 0, ROLL_MAX);
    static byte prevRollRate;
    if (rollRate != prevRollRate){
      prevRollRate = rollRate;
      needLcdUpdate = TRUE;
    }
  }
  // [F30] SHIFT makes the encoder move tempo a tenth at a time, carrying into
  // the whole number at either end so the fine control is not a separate range
  // you can get stuck inside. Coarse is still the default, because a bar's worth
  // of turning to cross ten BPM would be worse than no fine control at all.
  //
  // Tap mode is the exception - SHIFT there is the roll rate, above. Tempo is
  // not what a hand is doing in tap mode.
  else if (shiftBtn){
    int t = (int)seq.bpm * 10 + (int)seq.bpmFine;
    t = EncGet(t, 1);
    t = constrain(t, MIN_BPM * 10, MAX_BPM * 10);
    const unsigned int b = (unsigned int)(t / 10);
    const byte f = (byte)(t % 10);
    if (b != seq.bpm || f != seq.bpmFine){
      seq.bpm = b;
      seq.bpmFine = f;
      seq.setupNeedSaved = TRUE;
      TimerSetFrequency();
      needLcdUpdate = TRUE;
    }
  }
  else{
    seq.bpm = EncGet(seq.bpm, 1);
    if (seq.bpm <= MIN_BPM) seq.bpm = MIN_BPM;
    if (seq.bpm >= MAX_BPM) seq.bpm = MAX_BPM;
    static unsigned int curBpm;
    if( seq.bpm != curBpm)
    {
      curBpm = seq.bpm;
      // [B-09] In slave mode the timer's period is measured from the incoming
      // clock, so writing the internal tempo into it here would fight the
      // interpolator for one MIDI clock on every detent.
      if (seq.sync != SLAVE) TimerSetFrequency();
      if (curSeqMode != PTRN_STEP || tempoBtn.pressed) needLcdUpdate = TRUE;
    }
  }
}


// [3E] Read the encoder pins and decide this pass's detent. Called once, from
// the top of EncGet(), and it is the only thing that moves encoder_A_prev.
void EncScan()
{
  encoder_A = PINB & B1;// Read encoder pins
  encoder_B = PINB & B10;
  encStep = 0;
  // A has gone from high to low: one detent, direction from B.
  if ((!encoder_A) && (encoder_A_prev)) encStep = encoder_B ? 1 : -1;
  encoder_A_prev = encoder_A;     // Store value of A for next time
}


int EncGet(int value, int dif)
{
  if (encStep > 0) value += dif;
  else if (encStep < 0){
    // [3C] The decrement used to carry a clamp that belonged to two specific
    // v1 fields: with INST held it would only subtract when the value was
    // already >= 10 or >= 16, and for curIndex 3 it would not subtract at all.
    // That was the coarse track-position and note-index stepping written into
    // the encoder primitive rather than into its callers - so INST+encoder
    // walking the voices could go up and never down, and stuck wherever the
    // cursor happened to be.
    //
    // A floor at zero is all a shared primitive can honestly know. Every
    // caller constrains its own range on the way out, and a coarse step that
    // would undershoot now lands on zero instead of refusing to move.
    value -= dif;
    if (value < 0) value = 0;
  }
  return value;
}


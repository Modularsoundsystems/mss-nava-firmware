//-------------------------------------------------
//                  NAVA v1.x
//                 SEQ configuration
//-------------------------------------------------

/////////////////////Function//////////////////////
void SeqConfiguration()
{
  if(seq.syncChanged){                               
    SetSeqSync();
    seq.syncChanged = FALSE;
  }
  // [3T] Not while running. The setup is a multi-page EEprom write, and the
  // pages you can now reach mid-set are exactly the ones you would be pressing
  // ENTER near. The flag stays up, so the ENTER LED keeps asking and the write
  // happens the moment you stop - the setting is live either way, it is only
  // the trip to the chip that waits.
  if (seq.setupNeedSaved && enterBtn.justPressed && !isRunning){
    SaveSeqSetup();
    seq.setupNeedSaved = FALSE;
    MsgShow(MSG_SETUP, 0);
  }
  // [F29] Paste page: the step buttons are the toggles. One button, one field
  // group, which is the same gesture the panel already uses for mutes.
  // [3T] The pads toggle the mask on the paste page, and also while SHIFT+COPY or
  // SHIFT+PASTE is held - the message already shows the mask at that moment, so
  // the one time you most want to change it is the one time you were only allowed
  // to read it. The copy or paste itself has already fired on the press; what you
  // set here is what the next one carries.
  // [5G] One editor now. The config page that used to be the other one has gone;
  // holding the chord is the whole of it.
  if ((copyHold || pasteHold) && inStepPressed){
    for (byte a = 0; a < 8; a++){
      if (bitRead(inStepPressed, a)){
        seq.pasteMask ^= (byte)(1 << a);
        seq.setupNeedSaved = TRUE;
        needLcdUpdate = TRUE;
      }
    }
  }

  // [F23] The dump page. ENTER *held*, not pressed: a full dump blocks the panel
  // for the best part of a minute and there is no way to call it back, so it
  // gets the same deliberate gesture the pattern utilities use for their
  // irreversible writes. A plain ENTER press on a config page means "save the
  // setup" everywhere else, and that has to keep meaning that here.
  if (seq.configMode && seq.configPage == CONF_PAGE_SYSEX){
    static boolean dumpArmed = FALSE;
    if (enterBtn.justPressed) dumpArmed = TRUE;
    if (dumpArmed && enterBtn.hold){
      dumpArmed = FALSE;
      switch (dumpWhat){
      case DUMP_PTRN:
        SysExDumpPattern(curPattern);
        SysExSendEnd();
        break;
      case DUMP_TRK:
        for (byte n = 0; n < MAX_TRACK; n++) SysExDumpTrack(n);
        SysExSendEnd();
        break;
      case DUMP_SETUP:
        SysExDumpSetup();
        SysExSendEnd();
        break;
      default:
        SysExDumpAll();       // draws its own progress and blocks
        break;
      }
      needLcdUpdate = TRUE;
    }
    if (enterBtn.justRelease) dumpArmed = FALSE;
  }

  if (!seq.configMode) seq.setupNeedSaved = FALSE;

}


void SetSeqSync() 
{
    //Sync configuration
    switch (seq.sync){                             // [zabox] [1.028] added expander mode
    case MASTER: 
      initTrigTimer();                          
      DisconnectMidiHandleRealTime();
      DisconnectMidiHandleNote();
      TimerStart();//cf timer
      break;
    case SLAVE:
      // [B-09] The timer runs in slave mode now - it is what spaces the four
      // internal ticks belonging to each MIDI clock. It starts at the internal
      // tempo and the first clock intervals measure the real one; until then the
      // tick budget keeps it from running away.
      initTrigTimer();
      DisconnectMidiHandleNote();
      ConnectMidiHandleRealTime();
      slaveTicks = 0;
      slaveLastClock = micros();
      TimerStart();
      break;
    case EXPANDER:
      TimerStop();
      initExpTimer();                 
      DisconnectMidiHandleRealTime();
      ConnectMidiHandleNote();
      stepLeds = 0;
      configLed = 0;
      menuLed = 0;
      break;
    }
}


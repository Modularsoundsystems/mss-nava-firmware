//-------------------------------------------------
//                  NAVA v1.x
//                 keyboard mode
//-------------------------------------------------

/////////////////////Function//////////////////////
// [5T] The slot the next note would go into: one past the last one written, or
// the top of the array if the stack is full. The stack is never empty of meaning -
// extLength 0 is one note - so this is always at least 1.
static byte KeybAppendSlot()
{
  if (ExtStackEmpty()) return 0;          // [5V] the first note goes in slot 0
  const byte n = (byte)(pattern[ptrnBuffer].extLength + 1);
  return (n > MAX_EXT_INST_NOTE - 1) ? (byte)(MAX_EXT_INST_NOTE - 1) : n;
}

// Sound the slot under the cursor, if it holds a note. The append slot does not,
// and neither does any slot past it.
static void KeybNoteOn()
{
  if (ExtStackEmpty()) return;                                  // [5V]
  if (noteIndex > pattern[ptrnBuffer].extLength) return;
  if (!pattern[ptrnBuffer].extNote[noteIndex]) return;          // [5Y] empty slot
  keybNoteSounding = pattern[ptrnBuffer].extNote[noteIndex];
  keybNoteActive   = TRUE;
  MidiSendNoteOn(seq.TXchannel, keybNoteSounding, HIGH_VEL);
}

// [5R] again: take off the note that went on, not one worked out from the cursor
// afterwards - the cursor may have moved, and on the append slot there was never
// a note to take off at all.
static void KeybNoteOff()
{
  if (!keybNoteActive) return;
  keybNoteActive = FALSE;
  MidiSendNoteOff(seq.TXchannel, keybNoteSounding);
}

void KeyboardUpdate()
{
  if (isRunning && keyboardMode){
    keyboardMode = 0;
    needLcdUpdate = TRUE;
  }
  
  if (numBtn.justPressed && curInst == EXT_INST && curSeqMode == PTRN_STEP){
    keyboardMode = !keyboardMode;
    //stop sequencer when keyboard mode ON
    if (keyboardMode){
      InitMidiNoteOff();
      isStop = TRUE;
      isRunning = FALSE;
   //   MIDI.sendRealTime(Stop);//;MidiSend(STOP_CMD);
      stopBtn.counter = 1;//like a push on stop button
    }
    needLcdUpdate = TRUE;
  }

  /////////////////////////////KeyboardMode//////////////////////////////
  if(keyboardMode)
  {
    if (scaleBtn.justPressed){
      keybOct++;
      if (keybOct >= MAX_OCT) keybOct = MAX_OCT - 1;
      needLcdUpdate = TRUE;
    }
    if (lastStepBtn.justPressed){
      // [B-15] keybOct is a byte: "< 0" was always false. Guard before decrementing.
      if (keybOct == 0 || keybOct > MAX_OCT - 1) keybOct = 0;
      else keybOct--;
      needLcdUpdate = TRUE;
    }
    // [5T] The cursor stops at the end of the STACK, not at the end of the array.
    //
    // It ran to MAX_EXT_INST_NOTE - 1, so walking forward left the stack entirely
    // and read slots that had never been written this session. Those hold whatever
    // was last there, and it plays: notes 108 and 21 came back from a stack whose
    // real contents were three notes around octave 3. Playback was reading the
    // right slots the whole time - only the cursor was off the end - which is why
    // the two disagreed.
    //
    // One past the last note is allowed and is the append slot: the cursor has to
    // be able to reach the place the next note goes. It holds nothing, so nothing
    // sounds there - a slot with no note in it should not play one.
    if (backBtn.justPressed){
      // [B-15] noteIndex is a byte: "< 0" was always false. Guard before decrementing.
      if (noteIndex == 0 || noteIndex > MAX_EXT_INST_NOTE - 1) noteIndex = 0;
      else noteIndex--;
      KeybNoteOn();
      needLcdUpdate = TRUE;
    }
    if (backBtn.justRelease) KeybNoteOff();

    if (fwdBtn.justPressed){
      const byte append = KeybAppendSlot();
      if (noteIndex < append) noteIndex++;
      KeybNoteOn();
      needLcdUpdate = TRUE;
    }
    if (fwdBtn.justRelease) KeybNoteOff();

    // [5O] CLEAR rewinds the cursor; CLEAR held empties the stack. There was no
    // way to shorten one at all before - the length only ever grew - so a stack
    // that had picked up stray slots could not be put right from the panel.
    if (clearBtn.justPressed){
      noteIndex = 0;
      needLcdUpdate = TRUE;
    }
    // [5V] Unconditional. Gating it on extLength meant a stack of exactly one
    // note - extLength 0 - could not be emptied, which is the one case where you
    // most want to start again.
    if (BtnHeld(&clearBtn)){
      for (byte i = 0; i < MAX_EXT_INST_NOTE; i++) pattern[ptrnBuffer].extNote[i] = 0;
      pattern[ptrnBuffer].extLength = 0;
      noteIndex = 0;
      patternWasEdited = TRUE;
      MsgShow(MSG_CLEARED, 0);          // [5Y] say so - a silent wipe is unprovable
      needLcdUpdate = TRUE;
    }

    if (isStop){
      // [3A] Was the only place stepBtn[].pressed was ever written, while two
      // other consumers read it. A step released outside keyboard mode left it
      // latched HIGH and that step stopped responding in step-write mode.
      for (byte a = 0; a < NBR_STEP_BTN; a++){
        if (bitRead(inStepPressed, a)){
          pattern[ptrnBuffer].extNote[noteIndex] = a + (12* keybOct);
          MidiSendNoteOn(seq.TXchannel, a + 12*keybOct, HIGH_VEL);
          // [5O] The stack is as long as the furthest note written, not as far as
          // the cursor has been walked.
          //
          // It was `extLength = noteIndex - 1` taken after the increment - so the
          // length followed the CURSOR, and << and >> move the cursor. Walk out to
          // slot 9 to look at something, enter one note, and the stack became nine
          // long with slots 0 to 8 never written: playback then sent note 12 over
          // and over, because an unwritten slot holds 0 and MidiSendNoteOn() adds
          // the octave offset. Reported as exactly that - the right notes on the
          // keyboard page, note 12 on the wire.
          if (noteIndex > pattern[ptrnBuffer].extLength)
            pattern[ptrnBuffer].extLength = noteIndex;
          noteIndex++;
          patternWasEdited = TRUE;
          if (noteIndex > MAX_EXT_INST_NOTE - 1) noteIndex = MAX_EXT_INST_NOTE - 1;
          needLcdUpdate = TRUE;
        }
        if (bitRead(inStepReleased, a)){
          MidiSendNoteOff(seq.TXchannel, a + 12*keybOct);
        }
      }
    }
  }
}









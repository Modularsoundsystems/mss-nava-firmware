//-------------------------------------------------
//                  NAVA v1.x
//                  LCD update
//-------------------------------------------------

#include "string.h"

/////////////////////Function//////////////////////

// [3A] Push the frame to the panel, writing only what changed.
//
// Runs of differing cells are written in one go; a matching cell ends the run
// because re-addressing costs one command (~206 us) and so does one character,
// meaning a single-cell gap is a wash and anything wider is a saving.
void LcdFlush()
{
  for (byte r = 0; r < LCD_ROWS; r++){
    byte c = 0;
    while (c < LCD_COLS){
      if (lcdFb[r][c] == lcdShadow[r][c]){ c++; continue; }
      const byte start = c;
      while (c < LCD_COLS && lcdFb[r][c] != lcdShadow[r][c]) c++;
      lcdHw.setCursor(start, r);
      for (byte i = start; i < c; i++){
        lcdHw.write(lcdFb[r][i]);
        lcdShadow[r][i] = lcdFb[r][i];
      }
    }
  }
}

// begin() leaves the panel cleared, so both frames start as spaces and the
// first flush correctly writes nothing.
void LcdBufInit()
{
  for (byte r = 0; r < LCD_ROWS; r++){
    for (byte c = 0; c < LCD_COLS; c++){
      lcdFb[r][c] = ' ';
      lcdShadow[r][c] = ' ';
    }
  }
}

//Initialise IO PORT and libraries
void LcdUpdate()
{
  {
    static byte prevLatch = 0xFE;
    static unsigned int prevBpm10 = 0xFFFF;
    // [5M] Held or latched, matching what LcdPrintState() draws. Watching only
    // the latches would leave the letter for a held modifier appearing whenever
    // something else happened to ask for a redraw, and never on its own.
    const byte lat = (byte)(((shiftLatchBtn.pressed || shiftLatchBtn.latched) ?  1 : 0)
                          | ((instLatchBtn.pressed  || instLatchBtn.latched)  ?  2 : 0)
                          | ((clearBtn.pressed      || clearBtn.latched)      ?  4 : 0)
                          | ((shufBtn.pressed       || shufBtn.latched)       ?  8 : 0)
                          | ((lastStepBtn.pressed   || lastStepBtn.latched)   ? 16 : 0)
                          | ((guideBtn.pressed      || guideBtn.latched)      ? 32 : 0));
    // [3H] What is actually on screen, not just our own tempo - in slave mode
    // the number comes from the incoming clock and changes with no press behind
    // it at all. Going stale counts as a change too, which is what puts the
    // dashes up when the master stops.
    const unsigned int bpm10 = TempoShown();

    if (lat != prevLatch || bpm10 != prevBpm10){
      prevLatch = lat; prevBpm10 = bpm10;
      needLcdUpdate = TRUE;
    }
  }

  if (tempoBtn.pressed) {                                                        // [1.028]
    if(curSeqMode != PTRN_PLAY && curSeqMode != PTRN_STEP && curSeqMode != PTRN_TAP
       && !shiftBtn && !seq.configMode) {
        lcd.setCursor(9,1);                                                     // [F30] the tenth needs two more cells
        LcdPrintTempo();
    }
  }
  //display total  accent
  /*if (enterBtn.pressed && curSeqMode == PTRN_PLAY){
   LcdPrintTotalAcc();
   }*/
  // [3D] A live message owns the display. Copy and paste hold for as long as
  // their own keys are down, plus a short tail so a quick press is still
  // readable; everything else is timed. Expiry asks for one redraw to hand the
  // screen back.
  if (msgKind){
    // [5G] Through the hold flags, so the message stays up for the whole gesture
    // rather than expiring the moment SHIFT is released - which is exactly when
    // you start editing the mask it is showing you.
    const boolean held = (msgKind == MSG_COPY  && copyHold)
                      || (msgKind == MSG_PASTE && pasteHold);
    const unsigned int life = (msgKind == MSG_COPY || msgKind == MSG_PASTE)
                              ? MSG_TAIL : MSG_TIME;
    if (held) msgSince = millis();
    if ((millis() - msgSince) > life){ msgKind = MSG_NONE; }
    needLcdUpdate = TRUE;
  }

  if(needLcdUpdate){

    needLcdUpdate = FALSE;

    if (msgKind){
      LcdDrawMsg();
    }
    // [3C] BANK or TRACK held with the encoder armed: a one-screen preview of
    // what letting go will select. Scrolling blind was the whole fault - the
    // value moved and nothing said so, on the display or on the LEDs.
    else if ((bankBtn.pressed && encPtrnArmed) || (trkBtn.pressed && encTrkArmed)){
      const boolean isTrk = (trkBtn.pressed && encTrkArmed);
      lcd.setCursor(0,0);
      if (isTrk){
        lcd.print(F("Track "));
        LcdPrintNumbered(encTrkScroll, TRUE);
        lcd.print(F("   "));
      }
      else {
        lcd.print(F("Ptrn  "));
        lcd.print((char)((encPtrnScroll / NBR_PATTERN) + 65));
        LcdPrint2((encPtrnScroll % NBR_PATTERN) + 1);
        lcd.print(F("    "));
      }
      // The one you are on is worth marking, because most of the time you are
      // scrolling away from it and want to know how far.
      lcd.setCursor(15,0);
      lcd.print((isTrk ? (encTrkScroll == trk.current)
                       : (encPtrnScroll == curPattern)) ? (char)126 : ' ');
      lcd.setCursor(0,1);
      lcd.print(browseName);
      lcd.print(F("     "));
    }
    // [F26] The browser. Number on the left so the pads still mean something
    // while it is open, name across the second line, and a marker on the entry
    // you are actually playing so scrolling never loses the way home.
    else if (browseMode){
      lcd.setCursor(0,0);
      if (browseIsTrack){
        lcd.print(F("Trk "));
        LcdPrintNumbered(browseSel, TRUE);
      }
      else {
        lcd.print(F("Ptn "));
        lcd.print((char)((browseSel / NBR_PATTERN) + 65));
        LcdPrint2((browseSel % NBR_PATTERN) + 1);
        lcd.print(F(":"));
      }
      // [3C] How long it is, so a choice can be made before it is heard.
      lcd.print(F(" "));
      if (!browseLen) lcd.print(F("---"));
      else            LcdPrint3(browseLen);
      lcd.print(browseIsTrack ? F("pos ") : F("stp "));
      lcd.setCursor(15,0);
      lcd.print((browseSel == (browseIsTrack ? trk.current : curPattern)) ? (char)126 : ' ');
      lcd.setCursor(0,1);
      lcd.print(browseName);
      lcd.print(F("     "));
    }
    // [F18] Trig conditions. Step 0 is "ALL", the instrument as a whole, so the
    // per-instrument and per-step pairs share one page rather than needing two.
    // [F21] Links. Source, target, rule - "when may this play, given what that
    // one is doing". NOT is selective choke: separate analog voices cannot cut
    // a note that has already started, so the answer is not to start it.
    else if (condMode && condPage == 1){
      const byte lk = pattern[ptrnBuffer].instLink[curInst];
      lcd.setCursor(0,0);
      lcd.print(F("tgt rule src    "));                                     // [6R][6W]
      lcd.setCursor(cursorPos[curIndex],0);
      lcd.print(letterUpLink[curIndex]);

      lcd.setCursor(0,1);
      char nm[4];

      // [6W] Target, rule, source. The source is what a pad selects, so it sits at
      // the far end where the cursor rarely has to go; the encoder opens on the
      // target, which is the one field the pads cannot reach.
      const byte ti = (byte)(lk >> 4);
      if (ti >= NBR_VOICE) lcd.print(F("--- "));
      else {
        strcpy_P(nm, (char*)pgm_read_word(&(selectInstString[voiceOrder[ti]])));
        lcd.print(nm);
        lcd.print(F(" "));
      }

      switch (lk & 0x0F){
      case LINK_AND: lcd.print(F("AND ")); break;
      case LINK_NOT: lcd.print(F("NOT ")); break;
      case LINK_PRE: lcd.print(F("PRE ")); break;
      case LINK_NPR: lcd.print(F("NPR ")); break;
      default:       lcd.print(F("--- ")); break;
      }

      strcpy_P(nm, (char*)pgm_read_word(&(selectInstString[curInst])));
      lcd.print(nm);
      lcd.print(F("     "));
    }
    else if (condMode){
      const byte v = (condStep == 0) ? pattern[ptrnBuffer].instCondProb[curInst]
                                     : pattern[ptrnBuffer].stepCond[curInst][condStep - 1];
      lcd.setCursor(0,0);
      lcd.print(F("prb cnd stp ins "));                                     // [6R]
      lcd.setCursor(cursorPos[curIndex],0);
      lcd.print(letterUpCond[curIndex]);

      lcd.setCursor(0,1);
      // [6R] Probability first: it is what this page is opened for, and the two
      // that follow it are addresses rather than values.
      //
      // Shown as a percentage, because "9 sixteenths" is not how anyone thinks
      // about how often a note should happen.
      const byte p = (byte)(v & 0x0F);
      if (p == 0) lcd.print(F("--- "));
      else { LcdPrint3((unsigned int)((p * 100U) / 16U)); lcd.print(F(" ")); }

      char cn[4];
      strcpy_P(cn, (char*)pgm_read_word(&(nameCond[(v >> 4) < COND_COUNT ? (v >> 4) : 0])));
      lcd.print(cn);
      lcd.print(F(" "));

      if (condStep == 0) lcd.print(F("ALL "));
      // Four cells like every other field: "ALL " is four, so "01" needs two
      // trailing spaces, not one, or the whole row shifts left by a character.
      else { if (condStep < 10) lcd.print(F("0")); lcd.print(condStep); lcd.print(F("  ")); }

      char instName[4];
      strcpy_P(instName, (char*)pgm_read_word(&(selectInstString[curInst])));
      lcd.print(instName);
      lcd.print(F("  "));            // names are 2 or 3 wide; clip rather than leave a tail
    }
    else if (utilMode){
      lcd.setCursor(0,0);
      lcd.print(utilOp == UTIL_OP_PST ? F("op  frm to  src ")
                                      : F("op  frm to  dst "));   // [4T]
      lcd.setCursor(cursorPos[curIndex],0);
      lcd.print(letterUpUtil[curIndex]);

      lcd.setCursor(0,1);
      lcd.print(utilOp == UTIL_OP_CLR ? F("CLR ")
              : utilOp == UTIL_OP_PST ? F("PST ") : F("MOV "));   // [4T]
      LcdPrint3(utilFirst + 1);
      lcd.print(F(" "));
      // [3E] The last pattern of the range, not how many there are. "clear 009
      // to 016" is the sentence; "clear 009, count 008" is arithmetic you have
      // to do standing over a machine about to erase eight patterns. The count
      // is still what is stored and what UtilExecute() acts on.
      LcdPrint3(utilFirst + utilCount);
      lcd.print(F(" "));
      // [4T] The fourth field is where the operation's third number goes, and
      // only the move has one. A paste shows where the buffer came from instead:
      // it is the one thing about a group paste you cannot see anywhere else on
      // the panel, and "paste what, exactly" is the question worth answering
      // before erasing part of thirty-two patterns.
      if (utilOp == UTIL_OP_MOV){ LcdPrint3(utilDest + 1); lcd.print(F(" ")); }
      else if (utilOp == UTIL_OP_PST){
        if (bufferHasPattern){
          lcd.print((char)((bufferSource / NBR_PATTERN) + 65));
          LcdPrint2((bufferSource % NBR_PATTERN) + 1);
          lcd.print(F(" "));
        }
        else lcd.print(F("--- "));
      }
      else lcd.print(F("--- "));
    }
    else if (nameEditMode){
      const boolean isTrack = (curSeqMode == TRACK_PLAY || curSeqMode == TRACK_WRITE);
      lcd.setCursor(0,0);
      LcdPrintNumbered(isTrack ? trk.current : curPattern, isTrack);   // 4 cells
      LcdPrintName(ActiveName());
      lcd.print(F(" "));
      // A caret under the character being edited. The step LEDs show the same
      // thing, but the eye is already on the display while typing and a cursor
      // you have to look away to find is not a cursor.
      lcd.setCursor(0,1);
      LcdClearLine();
      lcd.setCursor(4 + nameCursor, 1);
      lcd.print((char)94);
    }
    else if (seq.configMode)
    {
      lcd.setCursor(0,0);
      switch (seq.configPage){
      case CONF_PAGE_SYNC:
        lcd.print(F("syn bpm mTX mRX "));
        lcd.setCursor(cursorPos[curIndex],0);
        lcd.print(letterUpConfPage1[curIndex]);
        lcd.setCursor(0,1);
        LcdClearLine();
        lcd.setCursor(0,1);
        char  sync[4];                                                                  // [B-04] "MST"/"SLV"/"EXP" need 4
        strcpy_P(sync, (char*)pgm_read_word(&(nameSync[seq.sync])));
        lcd.print(sync);
        lcd.setCursor(4,1);
        lcd.print(seq.defaultBpm);
        // [5I] OFF rather than a number when the channel is out of range - the
        // field is three cells wide either way, so nothing shifts.
        lcd.setCursor(9,1);
        if (seq.TXchannel > MAX_MIDI_CHAN) lcd.print(F("OFF"));
        else { lcd.print(seq.TXchannel); lcd.print(F("  ")); }
        lcd.setCursor(13,1);
        if (seq.RXchannel > MAX_MIDI_CHAN) lcd.print(F("OFF"));
        else { lcd.print(seq.RXchannel); lcd.print(F("  ")); }
        break;
      case CONF_PAGE_BEHAV:// [4X] when things land, and what a played pad does
        lcd.print(F("pCh sTr mut cym "));                                       // [6Z]
        lcd.setCursor(cursorPos[curIndex],0);
        lcd.print(letterUpConfPage2[curIndex]);
        lcd.setCursor(0,1);
        LcdClearLine();
        lcd.setCursor(0,1);
        char  ptrnSyncChange[4];                                                        // [B-04] "SYN"/"FRE" need 4
        strcpy_P(ptrnSyncChange, (char*)pgm_read_word(&(namePtrnChange[seq.ptrnChangeSync])));
        lcd.print(ptrnSyncChange);
        lcd.setCursor(4,1);                                                             // [F7]
        lcd.print(seq.singleTrig ? F("ON ") : F("OFF"));
        lcd.setCursor(8,1);                                                             // [4B] tap play over mute
        lcd.print(seq.tapOverMute ? F("ON ") : F("OFF"));
        lcd.setCursor(12,1);                                                            // [6Z] cymbal accent
        lcd.print(seq.cymAccent ? F("ON ") : F("OFF"));
        break;
      // [4X] The hi-hats, all three answers in one place. They used to be spread
      // over two pages - priority and mute pairing on one, open-hat selection on
      // another - which is one subject filed under two headings.
      case CONF_PAGE_HAT:
        lcd.print(F("hhP mte oh      "));
        lcd.setCursor(cursorPos[curIndex],0);
        lcd.print(letterUpHat[curIndex]);
        lcd.setCursor(0,1);
        LcdClearLine();
        lcd.setCursor(0,1);                                                             // [3B] which lane wins a shared step
        char  hhPrio[4];
        strcpy_P(hhPrio, (char*)pgm_read_word(&(nameHHprio[seq.hhPriority])));
        lcd.print(hhPrio);
        lcd.setCursor(4,1);
        char  mute[4];                                                                  // [B-04] "C/O" needs 4
        strcpy_P(mute, (char*)pgm_read_word(&(nameMute[seq.muteModeHH])));
        lcd.print(mute);
        lcd.setCursor(8,1);                                                             // [4W] which pad names OH
        lcd.print(seq.ohSelect == OH_SEL_2ND ? F("2nd") : F("BTH"));
        break;
      // [4X] Which firmware is actually on this chip. No fields and no cursor: it
      // is the one page that answers rather than asks, which is why it stays
      // readable while the sequencer runs - the question comes up mid-set, and
      // stopping to answer it costs the take you were checking on.
      case CONF_PAGE_FW:
        lcd.print(F(FW_VERSION));
        lcd.print(F("        "));
        lcd.setCursor(8,0);
        lcd.print(F(FW_DATE));
        lcd.print(F("        "));
        lcd.setCursor(0,1);
        lcd.print(F("build "));
        lcd.print(F(FW_BUILD));
        lcd.print(F("          "));
        break;
      case CONF_PAGE_SYSEX:// [F23] send a dump from the panel
        lcd.print(F("SysEx dump      "));
        lcd.setCursor(0,1);
        LcdClearLine();
        lcd.setCursor(0,1);
        switch (dumpWhat){
        case DUMP_PTRN:  lcd.print(F("this pattern")); break;
        case DUMP_TRK:   lcd.print(F("all tracks  ")); break;
        case DUMP_SETUP: lcd.print(F("setup only  ")); break;
        default:         lcd.print(F("everything  ")); break;
        }
        // Says how to fire it, because a page that does something irreversible
        // should not also be a guessing game about which key does it.
        lcd.setCursor(12,1);
        lcd.print(F("HOLD"));
        break;
      }
    }
    else if (seq.sync == EXPANDER) {                                               // [1.028] Expander
      lcd.setCursor(0,0);
      lcd.print(F("    Expander    "));
      lcd.setCursor(0,1);
      lcd.print(F("                "));
    }

    else{
      switch (curSeqMode){
      case PTRN_PLAY:
        // [3B] The name gets the whole top line, because it is the one thing on
        // this screen that is not also on a LED somewhere. Location, tempo and
        // length go underneath in fixed columns, and the queued pattern sits at
        // the right where it can appear and vanish without moving anything else.
        lcd.setCursor(0,0);
        LcdPrintName(pattern[ptrnBuffer].name);
        lcd.print(F("  "));
        LcdPrintState();

        lcd.setCursor(0,1);
        lcd.print(char(curBank+65));
        LcdPrint2((curPattern % NBR_PATTERN) + 1);
        lcd.print(F(" "));                     // [3H] the bpm field carries a source cell now
        LcdPrintBpm();
        lcd.print(F(" len"));
        LcdPrint2(pattern[ptrnBuffer].length + 1);
        break;
      case PTRN_STEP:
      case PTRN_TAP:
        // [3E] SHUF first. A held page owns the display, the same way it owns the
        // knob in Enc.inc - with total accent selected this tested the other way
        // round, so holding SHUF left the accent screen up while the encoder
        // underneath it was editing the instrument's swing.
        if (BtnHeld(&shufBtn)){
          // [3E] The two values with step buttons get the two bars: shuffle on
          // top, flam back on the bottom with the bar it gave up in F19. The
          // selected instrument's swing is the digit in the corner of the top
          // line, where the flam number used to sit - it is the only value on
          // this page the pads cannot reach, so it belongs beside the knob that
          // sets it rather than taking a whole line of its own.
          //
          // [3F] Each bar is as wide as the row of pads that sets it: seven for
          // shuffle, which pads 1-7 set and which has no pad for "none", and
          // eight for flam on pads 9-16. So an empty shuffle bar is genuinely no
          // shuffle, and the cell counts can be read straight off the panel.
          lcd.setCursor(0,0);
          lcd.print(F("Shuf"));
          LcdPrintBar(MAX_SHUF_TYPE, pattern[ptrnBuffer].shuffle);
          lcd.print(F(" "));                    // [3F] the bar must not touch the name
          {
            char nm[4];                                                           // [B-04] "TRG" needs 4
            strcpy_P(nm, (char*)pgm_read_word(&(selectInstString[curInst])));
            lcd.print(nm);
            // 0 is "follow the pattern", not a swing of nothing, so it reads as
            // a dash - the instrument is simply not being steered separately.
            const byte sw = pattern[ptrnBuffer].instSwing[curInst];
            if (sw) lcd.print(sw);
            else    lcd.print(F("-"));
          }

          lcd.setCursor(0,1);
          lcd.print(F("Flam"));
          LcdPrintBar(MAX_FLAM_TYPE, (byte)(pattern[ptrnBuffer].flam + 1));
          lcd.print(F("   "));
          lcd.print(pattern[ptrnBuffer].flam);
        }
        else if (curInst == TOTAL_ACC){
          LcdPrintTotalAcc();
        }
        else if (keyboardMode){
          lcd.setCursor(0,0);
          lcd.print(F("idx not len oct "));
          lcd.setCursor(cursorPos[curIndex],0);
          lcd.print(letterUpExtInst[curIndex]);
          // [3J] Every value under its own label, and every number zero-padded so
          // the columns stay put as they cross ten - the same reason the rest of
          // the panel uses LcdPrint2. This drew the numbers unpadded, so `idx`
          // and `len` shifted a cell sideways at 10 and again at 100.
          lcd.setCursor(0,1);
          LcdPrint2(noteIndex + 1);                                               // idx, cells 0-1
          lcd.print(F("  "));

          // The note in this slot: name and octave, four cells. Names are two
          // cells already (" C", "C#"), and the octave is 0-10 because the note
          // itself is 0-127 - so ten fills the fourth cell that nine pads.
          {
            const byte n = pattern[ptrnBuffer].extNote[noteIndex];
            char note[3];                                                         // [B-04] "C#" needs 3
            strcpy_P(note, (char*)pgm_read_word(&(nameNote[n % 12])));
            lcd.print(note);                                                      // not, cells 4-7
            const byte oct = (byte)(n / 12);
            lcd.print(oct);
            if (oct < 10) lcd.print(F(" "));
          }

          LcdPrint2(pattern[ptrnBuffer].extLength + 1);                           // len, cells 8-9
          lcd.print(F("  "));

          // [3J] Where the pads are, rather than an offset to convert in your
          // head. Pad 1 is always C of this octave and the sixteen run up from
          // it, so the range you are about to play is on the screen in the form
          // you can act on. It was "+0", which says nothing about which notes
          // that is.
          lcd.print(F("C"));                                                      // oct, cells 12-15
          lcd.print(keybOct);
          lcd.print(F("  "));
        }
        else{
          // [3B] Step and tap are the same screen with one field swapped, which
          // is what they are: the same job with the pads doing something else.
          // Top line is what you are pointed at, bottom line is where you are.
          //
          // The scale is on the faceplate LEDs and the playhead is on the step
          // LEDs, so neither is repeated. What is left is the instrument, its
          // own loop length (or the roll rate, in tap) and the numbers.
          char instName[4];                                             // [B-04] "TRG" needs 4
          strcpy_P(instName, (char*)pgm_read_word(&(selectInstString[curInst])));
          if (curFlam) instName[1] = instName[1] + 32;   // lower case = flam armed

          lcd.setCursor(0,0);
          lcd.print(instName);
          lcd.print(F("    "));
          lcd.setCursor(4,0);
          if (curSeqMode == PTRN_TAP){
            lcd.print(F("rol"));
            lcd.print(rollName[rollRate]);
          }
          else {
            lcd.print(F("lan"));
            const byte loop = LaneLoop(curInst);
            if (!loop) lcd.print(F("--"));            // this lane follows the pattern
            else       LcdPrint2(loop);
          }
          lcd.setCursor(9,0);
          // [4H] CYC rather than REC when a tap replaces last cycle's part. It
          // is the same armed state with a different consequence, so it belongs
          // in the same cell rather than in one of its own.
          lcd.print((curSeqMode == PTRN_TAP && tapRecordArm)
                    ? (cycRec ? F("CYC ") : F("REC ")) : F("    "));
          LcdPrintState();

          lcd.setCursor(0,1);
          lcd.print(char(curBank+65));
          LcdPrint2((curPattern % NBR_PATTERN) + 1);
          lcd.print(F(" "));                     // [3H] the bpm field carries a source cell now
          LcdPrintBpm();
          lcd.print(F(" len"));
          LcdPrint2(pattern[ptrnBuffer].length + 1);
        }
        break;
      case MUTE:
        // [3F] Its own two lines. This used to write two or three cells over
        // whatever the mode you came from had left on the panel, which is why the
        // tempo went stale here: nothing on this screen ever wrote it, so those
        // cells kept whatever the last full redraw of another mode had put there
        // and no bpm change could reach them. It also meant the screen depended
        // on which door you came in through, and only two of the five doors were
        // handled.
        lcd.setCursor(0,0);
        lcd.print(F("MUTE "));
        // Solo is the state most worth saying out loud - it is a mask that
        // outlives the gesture that set it, and a voice missing because
        // something else is soloed looks exactly like a voice that is broken.
        lcd.print(soloInst ? F("SOLO") : F("    "));
        {
          byte n = 0;
          for (byte z = 0; z < NBR_INST; z++) if (bitRead(muteInst, z)) n++;
          lcd.print(F(" m"));
          LcdPrint2(n);
        }
        LcdPrintState();

        lcd.setCursor(0,1);
        lcd.print(char(curBank+65));
        LcdPrint2((curPattern % NBR_PATTERN) + 1);
        lcd.print(F(" "));                     // [3H] the bpm field carries a source cell now
        LcdPrintBpm();
        lcd.print(F(" len"));
        LcdPrint2(pattern[ptrnBuffer].length + 1);
        break;
      case TRACK_WRITE:
        lcd.setCursor(0,0);
        lcd.print(F("pos ptr len num "));
        lcd.setCursor(cursorPos[curIndex],0);
        lcd.print(letterUpTrackWrite[curIndex]);
        lcd.setCursor(0,1);
        LcdClearLine();
        lcd.setCursor(0,1);
        lcd.print(trk.pos + 1);                                                 // [zabox] 
        lcd.setCursor(4,1);
        lcd.print((char)((curPattern / 16) + 65));
        lcd.print((curPattern - (((curPattern / 16)*NBR_PATTERN)) + 1));        // [zabox] step button alignement
        lcd.setCursor(8,1);
        lcd.print(track[trkBuffer].length);
        lcd.setCursor(13,1);
        lcd.print(trk.current + 1);
        break;
      case TRACK_PLAY:
        lcd.setCursor(0,0);                                                     // [F26]
        LcdPrintNumbered(trk.current, TRUE);
        LcdPrintName(track[trkBuffer].name);
        lcd.print(F("  "));
        lcd.setCursor(0,1);
        LcdClearLine(); 
        lcd.setCursor(0,1);
        lcd.print(F("pos:"));
        lcd.print(trk.pos + 1);                                                 // [zabox]
        lcd.setCursor(8,1);
        lcd.print(F("ptrn:"));
        lcd.print((char)((curPattern / 16) + 65));
        lcd.print((curPattern - (((curPattern / 16)*NBR_PATTERN)) + 1));        // [zabox] step button alignement
        break;
      }
    }
  }
  // [3A] One flush per pass covers everything drawn this pass.
  LcdFlush();
}

// [3B] The state zone: cells 13..15 of line 0, on the HOME screens only.
//
// Two things live here, and they are the two the panel cannot otherwise tell
// you. A latched modifier is a mode you entered on purpose and will forget you
// are in - its own LED blinks, but only if you happen to look at that key. A
// running effect has a number, and the number is what you need when you meant
// slot 12 and got slot 11.
//
// Edit pages do not carry it. You are editing on a page, not performing, and
// they need all four of their labels.
void LcdPrintState()
{
  lcd.setCursor(13, 0);

  // [5K] The modifiers in play, in a fixed order. Which one is 'first' matters
  // less than that a given key always appears in the same place for a given set,
  // so the field is read at a glance rather than decoded.
  //
  // [5M] Held counts as well as latched. The latch was singled out because it is
  // the state you forget you are in, but a held modifier is the reason the pads
  // are doing something other than what the mode says - and while several of
  // these open a page of their own, SHIFT, INST and CLEAR do not, so on the home
  // screens their letter is the only thing that says why the panel changed.
  char l[6];
  byte n = 0;
  if (shiftLatchBtn.pressed || shiftLatchBtn.latched) l[n++] = 'S';
  if (instLatchBtn.pressed  || instLatchBtn.latched)  l[n++] = 'I';
  if (clearBtn.pressed      || clearBtn.latched)      l[n++] = 'C';
  if (shufBtn.pressed       || shufBtn.latched)       l[n++] = 'F';
  if (lastStepBtn.pressed   || lastStepBtn.latched)   l[n++] = 'L';
  if (guideBtn.pressed      || guideBtn.latched)      l[n++] = 'G';

  // [5K] All three cells belong to the latch. One letter used to be the whole of
  // it, with a star the moment a second key latched - which threw away something
  // the panel knew and the LEDs can only tell you if you happen to be looking at
  // the right key. Three letters, and with more than three latched the third cell
  // becomes the star - rightmost, so the letters that do fit stay where they were.
  for (byte i = 0; i < 3; i++){
    if (n > 3 && i == 2) lcd.print('*');
    else if (i < n)      lcd.print(l[i]);
    else                 lcd.print(' ');
  }
}

// [3B] Tempo as five plain cells, "120.0" / " 99.5", for the HOME lines. The
// older LcdPrintTempo() leads with a glyph and a dash and is still what the
// TEMPO-held overlay wants; this is the one that has to sit in a column.
// [3H] The tenths the panel should be showing: ours when we are the clock, the
// clock's when we are following one, 0 for "nothing to show". One place, so the
// readout and the redraw watcher cannot disagree about what is on screen.
unsigned int TempoShown()
{
  if (seq.sync == MASTER) return (unsigned int)(seq.bpm * 10 + seq.bpmFine);
  if (!slaveBpm10) return 0;                                    // no clock seen yet
  if ((millis() - slaveClockMs) > SLAVE_BPM_STALE) return 0;    // and it has stopped
  return slaveBpm10;
}

// [3H] Six cells now: one for the source, five for the number.
//
// Slave mode used to print "ext" and leave it there, so the tempo you were
// actually running at - the one thing worth knowing when you are checking a sync
// connection - was the one thing the panel would not say, while the firmware
// measured it accurately enough to clock the sequencer off it.
//
// 'S' rather than a glyph because it has to be legible at one character, and a
// space in master mode so that screen is unchanged.
void LcdPrintBpm()
{
  lcd.print((seq.sync == SLAVE) ? 'S' : ' ');

  const unsigned int t = TempoShown();
  if (!t){
    // Following a clock that is not running, or not there. Dashes rather than a
    // frozen number, which would be indistinguishable from a live one.
    lcd.print(F("  ---"));
    return;
  }
  const unsigned int whole = t / 10;
  if (whole < 100) lcd.print(F(" "));
  lcd.print(whole);
  lcd.print(F("."));
  lcd.print(t % 10);
}

//clear 16 character line-------------------------------------
void LcdClearLine()
{
  lcd.print(F("                "));//16 empty space to clear a line
}

// [3E] LcdPrintScale() removed. It drew "1/16", "1/32", "1/8t" or "1/16t" out of
// custom characters 0-4, and had had no caller since 3B put the scale on the
// faceplate LEDs and stopped repeating it on the display. Those five CGRAM slots
// are what the bar ramp is built from.

//Print tempo---------------------------------------------------------
void LcdPrintTempo()
{
  lcd.write(LCD_GLYPH_NOTE);
  // [3H] The dash carries the source on this overlay, the way the leading cell
  // does on the home lines - there is no room here for a cell of its own.
  lcd.print((seq.sync == SLAVE) ? 'S' : '-');
  if (seq.sync == MASTER){
    // [F30] Seven cells: glyph, dash, up to three digits, point, tenth.
    // [B-29] bpm is 30..250 and the hold-TEMPO readout does not clear the line
    // first, so crossing 100 -> 99 left the old hundreds digit stranded and
    // showed "990". The pad is on the end now rather than after the number, so
    // the decimal point stays put as the integer part changes width.
    lcd.print(seq.bpm);
    lcd.print(F("."));
    lcd.print(seq.bpmFine);
    if (seq.bpm < 100) lcd.print(F(" "));
  }
  else {
    // [3H] The clock we are following, on the same five cells the internal tempo
    // uses. Three solid blocks used to sit here saying only "not mine".
    const unsigned int t = TempoShown();
    if (!t) lcd.print(F("---  "));
    else {
      lcd.print(t / 10);
      lcd.print(F("."));
      lcd.print(t % 10);
      if (t < 1000) lcd.print(F(" "));
    }
  }
}

// [3E] One bar of `cells`, the first `filled` of them solid and the rest light.
// Callers pass their own idea of "filled", because the pages do not agree on
// what zero means: shuffle 0 is no shuffle and draws an empty bar, while flam 0
// is the shortest interval rather than the absence of one and draws a single
// block. Each knows which it is; the bar does not have to.
//
// This replaced LcdPrintLine(), which drew a row of character 161 for the track
// and left each caller to poke a 219 over one cell. Two calls per bar, two sets
// of coordinates to keep in step, and both characters at the mercy of whichever
// ROM the panel shipped with.
void LcdPrintBar(byte cells, byte filled)
{
  for (byte i = 0; i < cells; i++) lcd.write((i < filled) ? LCD_BAR_FULL : LCD_BAR_LIGHT);
}

//Lcd print saved !!!!
// [3D] Draw the live message. Called only while one exists, and it owns both
// lines - a message you have to read around is one you will not read.
void LcdDrawMsg()
{
  lcd.setCursor(0,0);
  switch (msgKind){
  case MSG_SETUP:  lcd.print(F("SETUP SAVED     ")); break;
  case MSG_TRACK:  lcd.print(F("TRACK SAVED     ")); break;
  case MSG_REVERT: lcd.print(F("REVERTED        ")); break;
  case MSG_NOUNDO: lcd.print(F("NOTHING TO UNDO ")); break;
  case MSG_NOREDO: lcd.print(F("NOTHING TO REDO ")); break;   // [6S]
  case MSG_UNDO:
  case MSG_REDO:
    // Levels still in hand, not levels used - "how many more times can I do
    // this" is the question a hand on the key is actually asking.
    lcd.print(msgKind == MSG_REDO ? F("REDO      ") : F("UNDO      "));   // [6S]
    lcd.print(msgArg);
    lcd.print(F("/"));
    lcd.print(UNDO_LEVELS);
    lcd.print(F("   "));
    break;
  case MSG_COPY:
  case MSG_PASTE:
    lcd.print(msgKind == MSG_PASTE ? F("PASTED  into ") : F("COPIED  from "));
    lcd.print((char)((curPattern / NBR_PATTERN) + 65));
    LcdPrint2((curPattern % NBR_PATTERN) + 1);
    break;
  case MSG_CLEARED: lcd.print(F("EXT STACK EMPTY ")); break;   // [5Y]
  default:         lcd.print(F("SAVED           ")); break;
  }

  lcd.setCursor(0,1);
  if (msgKind == MSG_COPY || msgKind == MSG_PASTE){
    // The mask, at the moment it is being applied rather than only on the page
    // where it was set. Upper case is carried, lower case is not.
    LcdPrintPasteMask();
  }
  else {
    // Everything else says what it applied to, which is the other half of
    // trusting that it happened.
    lcd.print((char)(curBank + 65));
    LcdPrint2((curPattern % NBR_PATTERN) + 1);
    lcd.print(F(" "));
    LcdPrintName(pattern[ptrnBuffer].name);
    lcd.print(F(" "));
  }
}


// [F26] Print the 11-character name at the cursor. Never trusts the stored
// bytes: a record that predates naming, or one that lost a page, would
// otherwise push control characters at the panel.
void LcdPrintName(const char *nm)
{
  for (byte i = 0; i < NAME_LEN; i++){
    const char c = nm[i];
    lcd.print((c >= ' ' && c <= '~') ? c : ' ');
  }
}

// nnn: prefix, absolute 1..128 for a pattern, 1..16 for a track. Absolute
// rather than the bank letter because a name is for finding one record among
// all of them, which is the question a bank letter cannot answer.
void LcdPrintNumbered(byte n, boolean isTrack)
{
  if (isTrack){
    lcd.print(F("T"));
    if (n + 1 < 10) lcd.print(F("0"));
    lcd.print(n + 1);
  }
  else {
    if (n + 1 < 100) lcd.print(F("0"));
    if (n + 1 < 10)  lcd.print(F("0"));
    lcd.print(n + 1);
  }
  lcd.print(F(":"));
}

// [3B] Two digits, zero padded. Columns that never move are most of what makes
// a two-line display readable at a glance.
void LcdPrint2(unsigned int v)
{
  if (v < 10) lcd.print(F("0"));
  lcd.print(v);
}

// Fixed three digits, so the utility fields stay in their columns instead of
// shuffling sideways as the numbers cross 10 and 100.
void LcdPrint3(unsigned int v)
{
  if (v < 100) lcd.print(F("0"));
  if (v < 10)  lcd.print(F("0"));
  lcd.print(v);
}

void LcdPrintPasteMask()
{
  static const char letters[] = "TVFSNXEC";
  for (byte i = 0; i < 8; i++){
    const char c = letters[i];
    lcd.print((seq.pasteMask & (1 << i)) ? c : (char)(c + 32));
    lcd.print(F(" "));
  }
}


//Lcd print Total Accent
void LcdPrintTotalAcc()
{
  // [3D] A bar on a track, like shuffle and flam. Three pages that each set one
  // number had three different ways of drawing it; a bar reads at a glance and a
  // "- x +" does not, and the value is worth having in figures beside it since
  // total accent is a number people quote to each other.
  lcd.setCursor(0,0);
  lcd.print(F("Total accent  "));
  LcdPrint2(pattern[ptrnBuffer].totalAcc);
  lcd.setCursor(0,1);
  // [3F] Thirteen cells for the thirteen steps of boost; zero accent is an empty
  // bar, which is what no accent looks like.
  LcdPrintBar(MAX_TOTAL_ACC, pattern[ptrnBuffer].totalAcc);          // [3E]
  lcd.print(F("   "));
}

//Lcd print initialize EEprom
void  LcdPrintEEpromInit()
{
  lcd.setCursor(0,0);
  lcd.print(F("  init EEprom ? "));
  lcd.setCursor(0,1);
  LcdClearLine();
  lcd.setCursor(0,1);
  lcd.print(F("press PLAY/ENTER"));

}


//Lcd print TM2 adjustement
void  LcdPrintTM2Adjust()
{
  lcd.setCursor(0,0);
  lcd.print(F("Adjust TM2 until"));
  lcd.setCursor(0,1);
  LcdClearLine();
  lcd.setCursor(0,1);
  lcd.print(F("TP1 is +5V..."));
}


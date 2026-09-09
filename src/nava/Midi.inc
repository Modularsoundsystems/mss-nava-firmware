//-------------------------------------------------
//                  NAVA v1.x
//              Midi Function
//-------------------------------------------------

/////////////////////Function//////////////////////
void MidiRead()
{
}
//initialize midi real time variable
void  InitMidiRealTime()
{
  midiStart = LOW;
  midiStop = LOW;
  midiContinue = LOW;
}

//intialize note off when stop or patternChanged  or ext instrument loop back
void InitMidiNoteOff()
{
  if(midiNoteOnActive){
    midiNoteOnActive = FALSE;
    MidiSendNoteOff(seq.TXchannel, midiNoteOnValue);      // [5R]
  }
}

//Send note OFF
// [5I] channel > 16 is OFF - see the note at HandleNoteOn() below. Every note the
// firmware sends passes through these two and every caller hands them
// seq.TXchannel, so one test each is the whole of the transmit side.
void MidiSendNoteOff (byte channel, byte value)
{
  if (channel > MAX_MIDI_CHAN) return;
  MIDI.sendNoteOff(value + 12, 0, channel);
  /* MidiSend(channel + 0x80);
   MidiSend(value);
   MidiSend(0x00);*/
}
//Send note ON
void MidiSendNoteOn (byte channel, byte value, byte velocity)
{
  if (channel > MAX_MIDI_CHAN) return;
  MIDI.sendNoteOn(value + 12, velocity, channel);
  /* MidiSend(channel + 0x90);
   MidiSend(value);
   MidiSend(velocity);*/
}

#define CLOCK_RING 4
static unsigned long clockSum   = 0;
static byte          clockCount = 0;
static unsigned long clockRing[CLOCK_RING];
static byte          clockRingAt   = 0;
static byte          clockRingUsed = 0;

void HandleClock()
{
  const unsigned long now = micros();
  const unsigned long dt  = now - slaveLastClock;
  slaveLastClock = now;
  slaveClockMs   = millis();                    // [3H] for "is the clock still there"

  // A quarter of a clock is one internal tick. The window rejects the first byte
  // after a start - where the previous timestamp means nothing - and anything
  // outside roughly 30 to 250 BPM, rather than steering the timer with garbage.
  if (dt > 9000UL && dt < 90000UL){
    TimerSetPeriodUs((unsigned int)(dt >> 2));

    // [3H] Measure a quarter note's worth before showing anything. One interval
    // carries every bit of jitter the sender, the cable and the opto-isolator
    // put into it, and a readout flickering between 119.4 and 120.7 is worse
    // than no readout - you cannot tell a drifting master from a noisy one.
    clockSum += dt;
    if (++clockCount >= 24){
      // [3H-2] Push the quarter into the ring and read the tempo off all of it.
      clockRing[clockRingAt] = clockSum;
      clockRingAt = (byte)((clockRingAt + 1) % CLOCK_RING);
      if (clockRingUsed < CLOCK_RING) clockRingUsed++;

      unsigned long span = 0;
      for (byte i = 0; i < clockRingUsed; i++) span += clockRing[i];

      // n clocks across `span` us: a quarter note is span * 24 / n, so tenths of
      // a BPM are 10 * 60e6 * n / (24 * span) = 25e6 * n / span. n tops out at 96,
      // and 25e6 * 96 is 2.4e9 - inside an unsigned long, with 1.9e9 to spare, so
      // the half-span added to round rather than truncate cannot overflow it
      // either. Rounding is not fussiness: truncating showed a 90.0 master as
      // 89.9, which is a display that is wrong by a tenth on a clock that is
      // exactly right.
      const unsigned long n = (unsigned long)(24U * clockRingUsed);
      const unsigned int  v = (unsigned int)((25000000UL * n + (span >> 1)) / span);

      // A deadband of two tenths. Even across four quarters the endpoint latency
      // leaves the last digit twitching between neighbours on a perfectly steady
      // master, and a digit that never settles reads as an unstable clock when
      // the clock is fine. Anything real moves further than this.
      const unsigned int d = (v > slaveBpm10) ? (unsigned int)(v - slaveBpm10)
                                              : (unsigned int)(slaveBpm10 - v);
      if (!slaveBpm10 || d >= 2) slaveBpm10 = v;

      clockSum   = 0;
      clockCount = 0;
    }
  }
  else {
    clockSum   = 0;                             // a rejected interval poisons the average
    clockCount = 0;
  }

  // The ISR decrements this, so the read-modify-write has to be atomic.
  uint8_t sreg = SREG;
  cli();
  const byte t = (byte)(slaveTicks + 4);
  slaveTicks = (t > SLAVE_TICK_MAX) ? (byte)SLAVE_TICK_MAX : t;
  SREG = sreg;
}

// [B-09] Transport edges reset the interpolator. A budget left over from before
// a stop would be spent the instant the clock came back, and a stale timestamp
// would make the first interval after a start look like however long the gap was.
static void SlaveClockReset()
{
  uint8_t sreg = SREG;
  cli();
  slaveTicks = 0;
  SREG = sreg;
  slaveLastClock = micros();
  clockSum   = 0;                               // [3H] and the half-built measurement
  clockCount = 0;
  // [3H-2] The ring too. Quarters measured either side of a stop are not four
  // consecutive quarters of anything, and the span across the gap is not a tempo.
  clockRingAt   = 0;
  clockRingUsed = 0;
}

//handle start
void HandleStart()
{
  SlaveClockReset();
  midiStart = HIGH;
}

//handle stop
void HandleStop()
{
  SlaveClockReset();
  midiStop = HIGH;
}

//handle Continue
void HandleContinue()
{
  SlaveClockReset();
  midiContinue = HIGH;
}


// [3I] Program change selects a pattern.
//
// Nothing did before: the firmware connected handlers for clock, start, stop,
// continue, note on, note off and SysEx, and no others, so program change and
// bank select were simply not listened for. Neither is a thing that broke - it
// had never been wired.
//
// 128 patterns and 128 program numbers, so the whole bank is reachable in one
// message and **bank select has nothing left to do**. Program n is the nth
// pattern: 0 is A01, 16 is B01, 127 is H16.
//
// It sets the same two flags a pad press sets and then gets out of the way, so
// everything downstream is identical - the record is preloaded into the back
// buffer, and `pCh` decides whether the swap happens now or on the bar line.
// A pattern change from the outside should land exactly like one from the panel.
void HandleProgramChange(byte channel, byte number)
{
  if (channel != seq.RXchannel) return;
  if (number >= MAX_PTRN) return;

  // Track modes own the pattern - the track says what plays and when - and a
  // program change arriving in the middle of that is two hands on one wheel.
  if (curSeqMode == TRACK_PLAY || curSeqMode == TRACK_WRITE) return;

  if (number == curPattern) return;

  // Naming one pattern ends any chain, the same as choosing one on the pads.
  // A saved chain that this pattern belongs to is picked up again on the swap,
  // [4Z] and it ends any chain, the same as picking one pattern on the pads.
  group.length   = 0;
  group.pos      = 0;

  nextPattern = number;
  selectedPatternChanged = TRUE;
  needLcdUpdate = TRUE;
}

//Connect midi real time callback                         // [zabox] [1.028]
void ConnectMidiHandleRealTime()
{
  MIDI.setHandleClock(HandleClock); 
  MIDI.setHandleStart(HandleStart);
  MIDI.setHandleStop(HandleStop);
  MIDI.setHandleContinue(HandleContinue);
}

//Disconnect midi real time callback
void DisconnectMidiHandleRealTime()
{
  MIDI.disconnectCallbackFromType(Clock);
  MIDI.disconnectCallbackFromType(Start);
  MIDI.disconnectCallbackFromType(Stop);
  MIDI.disconnectCallbackFromType(Continue);
}


//Connect midi real time callback                         // [zabox] [1.028]
void ConnectMidiHandleNote()
{
  MIDI.setHandleNoteOn(HandleNoteOn);
  MIDI.setHandleNoteOff(HandleNoteOff);
}

//Disconnect midi real time callback                      // [zabox] [1.028]
void DisconnectMidiHandleNote()
{
  MIDI.disconnectCallbackFromType(NoteOn);
  MIDI.disconnectCallbackFromType(NoteOff);
}


//Handle noteON
void HandleNoteOn(byte channel, byte pitch, byte velocity)
{
  //Midi note On with 0 velocity as Midi note Off
  if (velocity == 0){
    if (channel == seq.RXchannel){
      switch (pitch){
      case 35:
      case 36:
        MidiTrigOff(BD);
        break;
      case 38:
      case 40:
        MidiTrigOff(SD);
        break;
      case 41:
        MidiTrigOff(LT);
        break;
      case 45:
      case 47:
      case 48:
        MidiTrigOff(MT);
        break;
      case 50:
        MidiTrigOff(HT);
        break;
      case 34:
        MidiTrigOff(RM);
        break;
      case 39:
        MidiTrigOff(HC);
        break;
      case 42:
        MidiTrigOff(CH);
        break;
      case 46:
        MidiTrigOff(OH);
        break;
      case 49:
        MidiTrigOff(CRASH);
        break;
      case 51:
        MidiTrigOff(RIDE);
        break;
      case 60:
        TRIG_HIGH;
        break;
      }
    }
  }
  else{
    if (channel == seq.RXchannel){
      switch (pitch){
      case 35:
      case 36:
        MidiTrigOn(BD, velocity);
        break;
      case 38:
      case 40:
        MidiTrigOn(SD, velocity);
        break;
      case 41:
        MidiTrigOn(LT, velocity);
        break;
      case 45:
      case 47:
      case 48:
        MidiTrigOn(MT, velocity);
        break;
      case 50:
        MidiTrigOn(HT, velocity);
        break;
      case 34:
        MidiTrigOn(RM, velocity);
        break;
      case 39:
        MidiTrigOn(HC, velocity);
        break;
      case 42:
        MidiTrigOn(CH, velocity);
        break;
      case 46:
        MidiTrigOn(OH, velocity);
        break;
      case 51:
        MidiTrigOn(RIDE, velocity);
        break;
      case 49:
        MidiTrigOn(CRASH, velocity);
        break;
      case 60:
        if ((~ActiveMute()) & 1) {                                   // [1.028] mute  // [B-02]
          TRIG_LOW;
          trigCounterStart = TRUE;                                   // [zabox] [1.027] solves short trig 1 pulse
        }
        break;
      }
    }
  }
}

//Handle noteOFF
void HandleNoteOff(byte channel, byte pitch, byte velocity)
{
  if (channel == seq.RXchannel){
    switch (pitch){
    case 35:
    case 36:
      MidiTrigOff(BD);
      break;
    case 38:
    case 40:
      MidiTrigOff(SD);
      break;
    case 41:
      MidiTrigOff(LT);
      break;
    case 45:
    case 47:
    case 48:
      MidiTrigOff(MT);
      break;
    case 50:
      MidiTrigOff(HT);
      break;
    case 34:
      MidiTrigOff(RM);
      break;
    case 39:
      MidiTrigOff(HC);
      break;
    case 42:
      MidiTrigOff(CH);
      break;
    case 46:
      MidiTrigOff(OH);
      break;
    case 51:
      MidiTrigOff(RIDE);
      break;
    case 49:
      MidiTrigOff(CRASH);
      break;
    case 60:
      if (gateInst & 1) TRIG_HIGH;                             // [zabox] [1.028] gate
      break;
    }
  }   
}


//MidiTrigOn insturment
void MidiTrigOn(byte inst, byte velocity)
{
  
  if (instWasMidiTrigged[inst] == FALSE && (~(ActiveMute() >> inst) & 1)) {                                                        // [B-02]                                                           // [zabox] [1.028] expander
 
    SetMuxTrigMidi(inst, velocity);                                                            
    
    if (inst == OH) {
      SetDoutTrig((1 << HH) | (lastDoutTrig & (~(1 << HH_SLCT))));
      triggerTime[HH] = TCNT2;      
    } 
    else if (inst == CH) {
      SetDoutTrig((1 << HH) | (lastDoutTrig | (1 << HH_SLCT)));
      triggerTime[HH] = TCNT2;                                                
    }
    else {
      SetDoutTrig((1 << inst) | (lastDoutTrig));                         
      triggerTime[inst] = TCNT2;                                                    
    }
    
    if (showTrigLeds) {
      stepLeds = ((1 << ledMap[inst]) | lastStepLeds);
    }
    instWasMidiTrigged[inst] = TRUE;
  }
}

//MidiTrigOff insturment
void MidiTrigOff(byte inst)
{
  instWasMidiTrigged[inst] = FALSE;
  
  
  if ((gateInst >> inst) & 1U) SetMuxTrigMidi(inst, 0);                                               // [1.028]
  
}

//Midi send all note off
void SendAllNoteOff()
{
   if (seq.TXchannel <= MAX_MIDI_CHAN) MIDI.sendControlChange(ALL_NOTE_OFF , 0, seq.TXchannel);   // [5I]	
}


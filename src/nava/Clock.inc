//-------------------------------------------------
//                  NAVA v1.x
//                  BPM
//-------------------------------------------------

/////////////////////Function//////////////////////
//Timer interrupt
ISR(TIMER1_COMPA_vect) {
  // [B-09] In slave mode the timer paces the ticks but the MIDI clock authorises
  // them. No budget, no tick: the sequencer stops when the clock does, without
  // needing to be told.
  if (seq.sync == SLAVE){
    if (!slaveTicks) return;
    slaveTicks--;
  }
  CountPPQN();
}


ISR(TIMER2_COMPA_vect) {     // [zabox] [v1.028] 2ms trig off isr. improves led flickering a lot and improves external instrument midi out lag

  TRIG_TIMER_STOP;           // [zabox] one shot
  TRIG_TIMER_ZERO;           // [zabox] reset
  SetDoutTrig(tempDoutTrig);
}  
  

ISR(TIMER3_COMPA_vect) {       // [zabox] flam

  FLAM_TIMER_STOP;
  FLAM_TIMER_ZERO;

  SetMuxFlam(flamStep);                                                             // [B-07] the step this flam belongs to

  SetDoutTrig((stepValueFlam & (~ActiveMute())) | tempDoutTrig);//Send TempDoutTrig too to prevet tick noise on HH circuit // [B-02] parenthesised

  TRIG_TIMER_START;
  stepValueFlam = 0;

  // [B-07] SetMuxFlam() just re-charged the S/H caps to flamStep's velocities.
  // If the next step's mux had already been armed (fast tempo / short scale,
  // where the flam interval can outrun the half-step precompute), restore it,
  // otherwise the next step would fire with this step's velocities.
  if (muxArmedForNext && evtStepValue) SetMux(nextStepIndex);

}
  
  
// [B-07] Resolve the step index for sequencer position 'count'.
// Split out of the fire path so direction handling happens during the
// precompute, not while the trigger edge is waiting to go out.
static byte ResolveDir(int count)
{
  const byte len = pattern[ptrnBuffer].length;
  if (count < 0 || count > len) count = 0;   // stepCount starts at -1 before the first PLAY


  switch (seq.dir){
  case FORWARD:
    return count;
  case BACKWARD:
    return len - count;
  case PING_PONG:
    if (nextStepIndex == len && changeDir == 1) changeDir = 0;
    else if (nextStepIndex == 0 && changeDir == 0)  changeDir = 1;
    return changeDir ? count : (len - count);
  case RANDOM:
    return random(0, len + 1);
  }
  return count;
}


static unsigned long SubTicks(SubGrid *g, byte n, byte scale)
{
  if (n == g->forN && scale == g->forScale) return g->ticks;
  g->forN = n; g->forScale = scale; g->ticks = 0;
  if (n < 1) return 0;

  if (scale / n < 2){
    for (byte p = 0; p < scale; p += 2) g->ticks |= (1UL << p);
  }
  else {
    for (byte k = 0; k < n; k++) g->ticks |= (1UL << (byte)(((unsigned int)k * scale) / n));
  }
  return g->ticks;
}
static SubGrid rollGrid   = {0, 0xFF, 0};

// [F10][F11] Fire every instrument being held in tap mode.
//
// Built like a tap rather than like a step: each voice gets its cap charged by
// SetMuxTrigMidi() at the velocity of the pad that is down, and the trigger word
// is OR'd onto whatever is already out. That OR is what lets a roll tick share a
// tick with the sequencer's own step - at x1 they always do - without either one
// erasing the other's triggers.
static void RollFire()
{
  // [4B] A roll is played, not sequenced. `tap mut` on the config page decides
  // whether a muted voice still answers the pad - muting is a statement about
  // the pattern, and a hand on the pad is a statement about now.
  const unsigned int w = seq.tapOverMute ? rollMask : (rollMask & (~ActiveMute()));
  if (!w) return;

  unsigned int trig = 0;
  boolean hat = FALSE;

  for (byte z = 0; z < NBR_INST; z++){
    if (!bitRead(w, z)) continue;
    // rollVel is written on the press that started the hold, so it is always
    // set by the time a roll tick comes round; the fallback is for the one scan
    // where the mask is built before that press has been served.
    SetMuxTrigMidi(z, rollVel[z] ? rollVel[z] : instVelHigh[z]);
    if (z == CH || z == OH) hat = TRUE;
    else                    trig |= ((unsigned int)1 << z);
  }

  // CH and OH are one voice reached through HH, with tempDoutTrig picking the
  // decay - the same shape the tap handler uses, and it has to persist past the
  // 2 ms pulse because that is what TIMER2 restores.
  if (hat){
    trig |= ((unsigned int)1 << HH);
    tempDoutTrig = bitRead(w, CH) ? B10 : 0;
  }

  SetDoutTrig(trig | lastDoutTrig | tempDoutTrig);
  TRIG_TIMER_ZERO;
  TRIG_TIMER_START;

  // [F10] Quantised: the hit is written at the step the recorder is currently
  // pointing at, so a roll lands on the grid instead of wherever the hand was.
  // Rates finer than one hit per step therefore fill the steps they pass over -
  // the pattern holds one trig per step, and that is the quantising.
  if (tapRecordArm){
    for (byte z = 0; z < NBR_INST; z++){
      if (!bitRead(w, z)) continue;
      bitSet(tempInst[z], TapIndex(z));
      // The velocity has to be written here, with the hit. The trig word is
      // committed at the end of the measure but the velocity array is not part
      // of it, so a hit recorded without this one line played at whatever level
      // the lane already held - weak on a lane that had been cleared, silent on
      // one that had never been played at all.
      pattern[ptrnBuffer].velocity[z][TapIndex(z)] = rollVel[z] ? rollVel[z] : instVelHigh[z];
    }
  }
}


static unsigned int BuildStepWord(byte s)
{
  unsigned int sv = 0;
  for (byte z = 0; z < NBR_INST; z++)
  {
    if (LaneSilent(z)) continue;
    // [F18] Conditions and probability were decided in the precompute, half a
    // step ago. Deciding here would put a random() call per instrument on the
    // trigger edge, and would let the two calls of this function - one to charge
    // the caps, one to fire - disagree about what plays.
    if (!bitRead(evtCondMask, z)) continue;
    if (bitRead(pattern[ptrnBuffer].inst[z], LaneIndex(z, s))) sv |= (1U << z);
  }
  sv |= ((unsigned int)bitRead(metronome, s) << RM);          //Set stepvalue depending metronome
  return sv;
}

// [B-07] Everything expensive about a step happens here, half a step early:
// direction, the flam scan, and SetMux()'s DAC writes and S/H settling delays.
// Nothing here touches a trigger output.
void PrepareNextStep()
{
  // [F27] Advance the per-instrument phases, once per step.
  //
  // [6A] "Once per step" was left to an invariant that nothing enforced: the
  // boundary only calls PrepareNextStep() when `muxArmedForNext` is false, i.e.
  // when the half-step precompute was missed. But three places in the main loop
  // flip the buffer and clear muxArmedForNext deliberately - a free pattern swap,
  // revert to saved, and a paste into the live pattern - and any of them landing
  // in the second half of a step makes the boundary call this a SECOND time for
  // the same step. Everything else here is a function of
  // stepCount and comes out the same twice; the phases are counters, and came out
  // one step further on.
  //
  // So every lane with a length of its own opened the new pattern at its step 2,
  // and picked up another step at every change after that. Lanes on `---` were
  // untouched, because LaneIndex() hands those the master step directly - which is
  // what made it look like a property of having set a length at all.
  //
  // lanePhaseFresh is tested first and still wins: it means "lanePhase is already
  // correct for the step about to fire", which is a stronger statement than this
  // one and is set by the paths that zero the phases outright.
  //
  // Compare-and-reset rather than modulo: no divide, and it is off the trigger
  // edge either way.
  if (lanePhaseFresh) lanePhaseFresh = FALSE;
  else if (!lanePhaseStepDone){
    for (byte z = 0; z < NBR_INST; z++){
      const byte loop = LaneLoop(z);
      if (!loop) continue;
      if (++lanePhase[z] >= loop) lanePhase[z] = 0;
    }
  }
  lanePhaseStepDone = TRUE;

  const byte s = ResolveDir(stepCount);
  nextStepIndex = s;

  evtCondMask = BuildCondMask(s);                             // [F18] decide once, here

  // [B-13] Shuffle polarity now follows the musical position of the step rather
  // than free-running per fire. In BACKWARD and PING_PONG the old toggle was
  // not mirrored, so the swing landed on the wrong side of the beat in reverse.
  shufPolarity = s & 1;

  evtStepValue = BuildStepWord(s);


  setFlam();                                                  // [zabox] [1.027] if changed, update flam interval

  evtFlamWord = 0;
  if (evtStepValue) SetMux(s);                                // charges the S/H caps for step s

  muxArmedForNext = TRUE;
}

// [F19] Send one swing group: the instruments of the current step whose delay
// lands on this tick. Everything about *what* plays was decided in FireStep;
// this only decides when it leaves.
static void EmitGroup(unsigned int w)
{
  if (!w) return;

  unsigned int m = ActiveMute();

  // The hat rides with whichever group carries it. Its shared cap has to be
  // charged at the moment it fires - charging it early is exactly the mid-decay
  // step [B-37] exists to remove, and with swing "early" can be six ticks - and
  // the decay-select bit has to go out with the trigger that uses it.
  if (w & ((unsigned int)1 << HH)){
    SetMuxHat(curStep, TRUE, evtOhOn);
    tempDoutTrig = evtChOn ? B10 : 0;
  }
  else m |= ((unsigned int)1 << HH);

  // [7A] The cymbals ride with their group for the same reason the hat does, and
  // through the mute mask rather than around it: a muted cymbal may still be
  // ringing from before the mute, and moving a ringing cap is the whole fault.
  // Before SetDoutTrig() below, so the cap is charged and settled by the time the
  // trigger that uses it goes out.
  if ((w & ~m) & ((unsigned int)1 << CRASH)) SetMuxCym(CRASH, curStep, TRUE);
  if ((w & ~m) & ((unsigned int)1 << RIDE))  SetMuxCym(RIDE,  curStep, TRUE);

  SetDoutTrig((w & (~m)) | tempDoutTrig);
  TRIG_TIMER_ZERO;
  TRIG_TIMER_START;

  // A flam belongs to the group its instrument is in. Timer3 holds one pending
  // hit, so two groups that both carry flams share it - the second restarts the
  // interval and drags the first along. That needs flammable voices to have been
  // given different swing values, which is a stranger thing to ask for than it
  // is to get exactly right.
  const unsigned int f = evtFlamWord & w;
  if (f){
    stepValueFlam |= f;
    FLAM_TIMER_START;
  }
}

// [F19] Sort the step's instruments into the tick each one leaves on.
static void SwingDistribute()
{
  for (byte d = 0; d < SWING_SLOTS; d++) swingWord[d] = 0;
  if (!evtStepValue) return;

  byte maxDelay = (byte)(pattern[ptrnBuffer].scale / 2);
  maxDelay = (maxDelay > 1) ? (byte)(maxDelay - 1) : 0;
  if (maxDelay > SWING_SLOTS - 1) maxDelay = SWING_SLOTS - 1;

  for (byte z = 0; z < NBR_INST; z++){
    if (!bitRead(evtStepValue, z)) continue;

    // HH is not a lane of its own - it is the trigger the winning hi-hat lane
    // fires through, so it has to leave in the same group as that lane or the
    // decay select would arrive without the hit that needs it.
    const byte src = (z == HH) ? (evtChOn ? (byte)CH : (byte)OH) : z;

    byte d;

    {
      byte t = pattern[ptrnBuffer].instSwing[src];
      if (!t) t = pattern[ptrnBuffer].shuffle;
      if (t < 1) t = 1;
      if (t > MAX_SHUF_TYPE) t = MAX_SHUF_TYPE;

      const int off = shuffle[t - 1][shufPolarity];
      d = (off < 0) ? (byte)(-off) : 0;
    }

    if (d > maxDelay) d = maxDelay;
    swingWord[d] |= ((unsigned int)1 << z);
  }
}

// [B-07] Constant-time trigger path: mask, one SPI write, timers. Independent
// of how many instruments sound, which is what removes the density-correlated
// jitter. Mute is still applied here, at fire time, so muting stays sample-
// accurate rather than taking effect a step late.
void FireStep()
{
  curStep    = nextStepIndex;
  flamStep  = curStep;                 // the flam timer renders THIS step

  // [B-03] Re-read the pattern here, at fire time. The precompute froze the
  // instrument word half a step ago - but tapStepCount advances on that very
  // tick, so a note tapped in the second half of the preceding step (exactly
  // the window the look-ahead counter exists to serve) was written to this step
  // AFTER its word had been frozen. The note then stayed silent until the bar
  // came round again, which is the "tap lands a step late" symptom.
  //
  // Only the bitmask is rebuilt. SetMux()'s DAC writes and S/H settling stay in
  // the precompute, which is what removes the density-correlated jitter; this
  // is 16 bitReads and costs the same on a one-instrument step as on a
  // ten-instrument one, so the trigger edge stays deterministic.
  //
  // A note tapped inside that window fires from a cap the tap handler has
  // already charged via SetMuxTrigMidi(), so its velocity is right too.
  evtStepValue = BuildStepWord(curStep);
  const unsigned int am = ActiveMute();                                                         // [B-02]

  evtChOn = !LaneSilent(CH) && bitRead(evtCondMask, CH) &&
            bitRead(pattern[ptrnBuffer].inst[CH], LaneIndex(CH, curStep)) && !bitRead(am, CH);
  evtOhOn = !LaneSilent(OH) && bitRead(evtCondMask, OH) &&
            bitRead(pattern[ptrnBuffer].inst[OH], LaneIndex(OH, curStep)) && !bitRead(am, OH);

  if (evtChOn && evtOhOn){
    if (seq.hhPriority == HH_PRIO_CH) evtOhOn = FALSE;
    else                              evtChOn = FALSE;
  }
  if (!evtChOn) evtStepValue &= ~((unsigned int)1 << CH);
  if (!evtOhOn) evtStepValue &= ~((unsigned int)1 << OH);
  evtTrigOut = !LaneSilent(TRIG_OUT) && bitRead(evtCondMask, TRIG_OUT) &&
               bitRead(pattern[ptrnBuffer].inst[TRIG_OUT], LaneIndex(TRIG_OUT, curStep));
  evtExtInst = !LaneSilent(EXT_INST) && bitRead(evtCondMask, EXT_INST) &&
               bitRead(pattern[ptrnBuffer].inst[EXT_INST], LaneIndex(EXT_INST, curStep));

  // [B-36] evtChOn / evtOhOn already have mute folded in, so this is simply
  // "neither lane can sound" - the only case where the shared hat voice should
  // be silenced. Taking HH out of the word does what the old temp_muteInst bit
  // did, and does it once rather than once per group.
  //
  // [B-37]/[F19] When the hat IS playing, its cap is charged by the group that
  // carries it rather than here: with swing that group can be six ticks away,
  // and charging early is the mid-decay step [B-37] exists to remove. The
  // refresh below keeps a step that plays no hat costing the same as one
  // that does.
  if (!evtChOn && !evtOhOn){
    evtStepValue &= ~((unsigned int)1 << HH);
    SetMuxHat(curStep, FALSE, FALSE);
  }

  // [7A] And the cymbals: whichever of the two is not going to fire this step has
  // its cap refreshed with the level it already holds. The one that is firing gets
  // its new level from EmitGroup(), at the trigger. Exactly one charge each per
  // step, so the cost of a step does not depend on what is in it.
  if (!bitRead(evtStepValue, CRASH) || bitRead(am, CRASH)) SetMuxCym(CRASH, curStep, FALSE);
  if (!bitRead(evtStepValue, RIDE)  || bitRead(am, RIDE))  SetMuxCym(RIDE,  curStep, FALSE);

  stepValue = evtStepValue;            // kept for the DEBUG block
  stepValueFlam = 0;                   // each group adds the flams it carries


  // [F19] Group 0 leaves now; the rest drain over the ticks that follow.
  SwingDistribute();
  swingPos = 1;
  EmitGroup(swingWord[0]);

  if (evtTrigOut){
    TRIG_LOW;//Trigout
    trigCounterStart = TRUE;
  }

  //Trig external instrument-------------------------------------
  // [5V] Nothing to play from an empty stack, and [5Y] nothing from an empty
  // SLOT either. An unwritten slot holds zero, and zero was being sent as a note
  // - note 12 once the octave offset is added - so any gap in a stack, however it
  // got there, came out as a stream of the same low note.
  //
  // The cost is that a stack cannot hold pad 1 at octave 0. That is MIDI note 12,
  // a C five octaves below middle C, which no external instrument is going to do
  // anything useful with; and the alternative is a sentinel that would change the
  // stored format and break the records' compatibility with stock firmware.
  if (evtExtInst && !ExtStackEmpty() && pattern[ptrnBuffer].extNote[noteIndexCpt])
  {
    InitMidiNoteOff();
    midiNoteOnValue = pattern[ptrnBuffer].extNote[noteIndexCpt];   // [5R]
    MidiSendNoteOn(seq.TXchannel, midiNoteOnValue, HIGH_VEL);
    midiNoteOnActive = TRUE;
    noteIndexCpt++;//incremente external inst note index
  }
  if (noteIndexCpt > pattern[ptrnBuffer].extLength){
    noteIndexCpt = 0;
  }

  muxArmedForNext = FALSE;   // caps now belong to the step just fired
  lanePhaseStepDone = FALSE; // [6A] and the next step's phases are still owed
}

//Tick proceed each pulse
void CountPPQN()
{
  blinkVeryFast =! blinkVeryFast;
  if (ppqn % (PPQN/2) == 0) blinkTempo = !blinkTempo;
  if (ppqn  % (pattern[ptrnBuffer].scale/2) == 0) blinkFast = LOW;
 // if (ppqn % 4 == 0) MIDI.sendRealTime(Clock);      //MidiSend(CLOCK_CMD);//as NAVA seq is 96ppqn we need to send clock each 4 internal ppqn
  if (ppqn % 4 == 0) {                                
    while (!(UCSR1A & (1 << UDRE1))) {};                                            // [zabox] directly adressing the uart fixes the midi clock lag
    UDR1 = CLOCK_CMD; //Tick
  }

  // [B-09] Slave used to pulse this straight from HandleClock(). Now that ppqn
  // advances properly under an external clock, the same division that serves
  // master serves slave - and it is the tick counter's phase rather than the
  // arrival time of a MIDI byte, so anything chained downstream gets the
  // interpolated clock rather than the jittery one.
  if (seq.sync != EXPANDER){                                                        // [zabox] has to be 0/2 for the correct phase
    if (ppqn % 4 == 0){
      DIN_CLK_HIGH;                                                               
      dinClkState = HIGH;
    }
    else if (ppqn % 4 == 2) {
      DIN_CLK_LOW;
      dinClkState = LOW;
    }    
  }
  

  if (isRunning)
  {
    const byte scale = pattern[ptrnBuffer].scale;

    // [F19] Drain the swing groups before anything else this tick. Ahead of the
    // half-step precompute deliberately: that recharges the caps for the step
    // firing next, and a group leaving after it would fire from the wrong
    // velocities. The delay clamp already keeps them apart; this ordering means
    // the clamp is a margin rather than the only thing holding it together.
    if (swingPos < SWING_SLOTS){
      EmitGroup(swingWord[swingPos]);
      swingPos++;
    }

    // [B-07] Half-step point: do ALL the slow work for the step that fires next.
    // This is where SetMux() charges the S/H caps, so by the time the trigger
    // tick arrives everything is settled and firing costs a mask plus one SPI
    // write. It is also where tapStepCount has always advanced.
    if (ppqn % scale == (scale/2)) {
      tapStepCount++;
      if (tapStepCount > pattern[ptrnBuffer].length) tapStepCount = 0;
      PrepareNextStep();
    }

    if (ppqn % scale == 0) stepChanged = TRUE;//Step changed ?


    // [B-34] Re-phase after a scale or length change.
    //
    // tapStepCount advances at the half-step point and stepCount at the
    // boundary. Both derive from ppqn through `scale`, so changing scale
    // between those two events leaves them a different number of advances
    // apart - and nothing re-established the relationship afterwards, so the
    // offset persisted until the next PLAY. Changing scale and changing it
    // straight back is the worst case, because it re-phases twice: taps then
    // land one or two steps from where they were played, which reads as the
    // recorder being late rather than as a phase error.
    //
    // Length matters for the same reason: the two counters wrap at it
    // independently, so moving it mid-bar can wrap one and not the other.
    //
    // Done at the boundary, before FireStep() has run and before stepCount is
    // incremented, which is exactly the point where the two are defined to be
    // equal - the same invariant CONTINUE restores in [B-03].
    {
      static byte prevScale;
      static byte prevLength;
      const byte len = pattern[ptrnBuffer].length;
      if (scale != prevScale || len != prevLength){
        prevScale  = scale;
        prevLength = len;
        seqNeedRephase = TRUE;
      }
      if (seqNeedRephase && (ppqn % scale == 0)){
        seqNeedRephase = FALSE;
        if (stepCount > len) stepCount = 0;
        tapStepCount = stepCount;
        shufPolarity = stepCount & 1;
      }
    }

    // [F19] The step starts on the grid now and its instruments leave on their
    // own ticks, so the whole-pattern shuffle offset has come off this test and
    // become the default every instrument inherits. With no per-instrument
    // values set every instrument lands in the same group, and the result is
    // note-for-note what the offset used to produce.
    if ((ppqn % scale == 0) && stepChanged)
    {//Each Step
      stepChanged = FALSE;//flag that we already trig this step
      blinkFast = HIGH;

      // [B-07] First step after PLAY/CONTINUE, or a loop stall that swallowed
      // the half-step window: fall back to computing it here. Costs the old
      // jitter for that one step only.
      if (!muxArmedForNext) PrepareNextStep();

      FireStep();

      // [F11] A roll slower than one hit per step lands on the boundary only,
      // every `den` steps. Checked here, before the counters move on, so "every
      // fourth step" counts the step that just fired rather than the next one.
      //
      // [3X] Counted on rollPhase, not stepCount. stepCount resets at the bar, so
      // a rate that does not divide the bar was re-anchored to it every time round
      // - /2 and /4 divide sixteen and hid it, /3 does not and restarted every bar.
      // A triplet roll that restarts on the downbeat is not a triplet.
      if (rollMask && rollDen[rollRate] > 1 && curSeqMode == PTRN_TAP
          && (rollPhase % rollDen[rollRate]) == 0) RollFire();

      stepCount++;
      if (++rollPhase >= 12) rollPhase = 0;      // [3X] free-runs past the bar line

    }

    // [F10][F11] Roll driver. After the step fire, so a roll tick that shares a
    // tick with the step merges into it rather than replacing it - at x1 that is
    // every single hit. It rides the same shuffled grid the step does, or a roll
    // would play straight against a swung pattern and every x1 hit would arrive
    // as a flam a few ticks off the note it was meant to double.
    if (rollMask && rollRate && rollDen[rollRate] == 1 && curSeqMode == PTRN_TAP){
      const unsigned int t = ppqn + shuffle[(pattern[ptrnBuffer].shuffle)-1][shufPolarity];
      if (SubTicks(&rollGrid, rollNum[rollRate], scale) & (1UL << (byte)(t % scale))) RollFire();
    }
    if (stepCount > pattern[ptrnBuffer].length){// && (ppqn % 24 == pattern[ptrnBuffer].scale - 1))
      endMeasure = TRUE;
      trackPosNeedIncremante = TRUE;
      stepCount = 0;
      barCount++;                 // [F18] what "X of every Y" counts
      // [4D] A new cycle, so every voice is up for replacement again. This is
      // the whole of the cyclic-record state machine: one bitmask, cleared here,
      // set by the first tap each voice receives. Playing on through the bar
      // line replaces last cycle's part; stopping leaves it alone.
      cycTouched = 0;

      // [B-05][B-06][B-26] Bar-end pattern change is now nothing but a pointer
      // flip. Previously this block ALSO ran InitPattern() and SetHHPattern(),
      // which write pattern[ptrnBuffer] from inside the ISR while the main loop
      // could be writing the same struct (B-06); and nextPatternReady was
      // test-and-cleared non-atomically in both the ISR and the main loop, so
      // an unlucky interleave toggled ptrnBuffer twice and played the stale
      // buffer (B-05).
      //
      // The back buffer is now fully prepared loop-side at the moment the change
      // is queued (see the selectedPatternChanged handler in Seq.inc). That also
      // kills B-26: the old code called LoadPattern() - 30-50 ms of blocking I2C
      // - AFTER the bar line, so step 0, and step 1 at high tempo or 1/32, still
      // fired from the outgoing pattern.
      //
      // Ownership is one-way: the loop only ever sets nextPatternReady, the ISR
      // only ever clears it. Test-and-clear happens solely here, with interrupts
      // already disabled, so it is atomic.
      if (nextPatternReady){
        nextPatternReady = FALSE;
        ptrnBuffer = !ptrnBuffer;
        // [5A] Capture what this swap started, here, where it is still true. The
        // loop finalises curPattern from it - and by the time it gets to that,
        // the chain may already have advanced and re-armed for the next bar.
        swappedPattern = armedPattern;
        patternSwapped = TRUE;      // loop finalises curPattern / LCD / MIDI
        muxArmedForNext = FALSE;    // anything armed came from the old pattern

        for (byte z = 0; z < NBR_INST; z++) lanePhase[z] = 0;
        lanePhaseFresh = TRUE;
      }
    }
//    if (ppqn % pattern[ptrnBuffer].scale == 4 && stepCount == 0){ 
//      endMeasure = FALSE;
//    }
  
  }
  
  ppqn++;                                                                 // [1.028] more consistent to run the counter from 0-95
  if (ppqn >= PPQN) ppqn = 0;
  
}

//
void Metronome(boolean state)
{
  if (state){
    metronome = 0x1111;//trig RM every time
    pattern[ptrnBuffer].velocity[RM][0] = 127;
    pattern[ptrnBuffer].velocity[RM][4] = 30;
    pattern[ptrnBuffer].velocity[RM][8] = 30;
    pattern[ptrnBuffer].velocity[RM][12] = 30;
  }
  else{
    metronome = 0;
    pattern[ptrnBuffer].velocity[RM][0] = HIGH_VEL;
    pattern[ptrnBuffer].velocity[RM][4] = HIGH_VEL;
    pattern[ptrnBuffer].velocity[RM][8] = HIGH_VEL;
    pattern[ptrnBuffer].velocity[RM][12] = HIGH_VEL;
  }
}


//-------------------------------------------------
//                  NAVA v1.x
//                  Dio utility
//-------------------------------------------------

/////////////////////Function//////////////////////
//Update Velocity of each instruments---------------------------------------
// [B-07] Takes the step index instead of reading the global curStep, so the S/H
// caps can be charged for a step that has not played yet. The flam word is
// reported through evtFlamWord, not stepValueFlam: stepValueFlam belongs to the
// step currently sounding, which the flam timer is still going to render.
// [B-37] Charge the hi-hat's shared sample-and-hold, at fire time.
//
// Called unconditionally from FireStep(), whether or not the hat plays, so the
// trigger path stays constant-time - a fixed ~25 us on every step rather than
// one that depends on what is in the step, which is exactly what [B-07] set out
// to remove. When the hat is not playing the cap is refreshed with the level it
// already holds, so a hat still ringing from an earlier step is left alone.
//
// This is also now the only place the hat's level is decided, which retires the
// three-way agreement between SetMux(), SetMuxFlam() and FireStep() that the
// CH/OH priority rule used to need.
static byte hatCapLevel = 0;

void SetMuxHat(byte stp, boolean playing, boolean useOh)
{
  if (playing){
    const unsigned int acc = (bitRead(pattern[ptrnBuffer].inst[TOTAL_ACC], stp))
                             ? (pattern[ptrnBuffer].totalAcc * 4) : 0;
    const byte z = useOh ? (byte)OH : (byte)CH;
    int v = (int)pattern[ptrnBuffer].velocity[z][LaneIndex(z, stp)] + (int)acc;
    if (v > MAX_VEL) v = MAX_VEL;
    hatCapLevel = (byte)v;
  }
  SetDacA(hatCapLevel);
  PORTA = (PORTA & 0b00011111) | muxAddr[2];//Mask to clear last 3 bits of the PORTA
  SelectSecondMux();
}

// [7A] The same discipline for the two cymbals, and for the same reason.
//
// [B-37] moved the hat off the half-step precompute because CH and OH share one
// sample-and-hold and the hat rings for a long time: setting the NEXT step's
// level halfway through the current note changed the level of a voice that was
// still sounding, and the ear reads a level step in the middle of a decay as a
// second articulation - a flam.
//
// Crash and ride ring longer than either hat. They were never heard doing it
// because their level never changed: InitPatternBuf() forced both to full on
// every load, so the precompute wrote the same number into the same cap forever.
// [6Z] gave them a weak level, and with that the fault they had always had
// underneath became audible on the first weak-to-strong pair.
//
// Unconditional, like the hat: charged at the trigger by EmitGroup() when the
// voice plays, refreshed with the level it already holds by FireStep() when it
// does not. Exactly one charge per step either way, so the trigger path stays
// constant-time - which is what [B-07] is for, and is worth more than the ~30 us
// it costs on steps with no cymbal in them.
//
// Not gated on seq.cymAccent, deliberately. Total accent adds totalAcc*4 to the
// level at the steps it marks, so the cap was already being moved mid-decay with
// the switch off - a quieter version of the same fault, present since long before
// [6Z] and fixed here with it.
static byte cymCapLevel[2] = {0, 0};        // [0] CRASH, [1] RIDE

void SetMuxCym(byte z, byte stp, boolean playing)
{
  const byte i = (z == CRASH) ? 0 : 1;
  if (playing){
    const unsigned int acc = (bitRead(pattern[ptrnBuffer].inst[TOTAL_ACC], stp))
                             ? (pattern[ptrnBuffer].totalAcc * 4) : 0;
    int v = (int)pattern[ptrnBuffer].velocity[z][LaneIndex(z, stp)] + (int)acc;
    if (v > MAX_VEL) v = MAX_VEL;
    cymCapLevel[i] = (byte)v;
  }
  SetDacA(cymCapLevel[i]);
  PORTA = (PORTA & 0b00011111) | muxAddr[(z == CRASH) ? 3 : 4];
  SelectSecondMux();
}

void SetMux(byte stp)
{

  unsigned int temp_stepValue = evtStepValue & (~ActiveMute());                                                             // [B-02]  [zabox] [1.028] solves velocity change in fading instuments (when muted) in combination with total accent
  const unsigned int acc = (bitRead(pattern[ptrnBuffer].inst[TOTAL_ACC], stp)) ? (pattern[ptrnBuffer].totalAcc * 4) : 0;

  evtFlamWord = 0;

 //Select second Multiplexer
  for (byte a = 0; a < 5; a++){
    // [B-37] The hat is charged at fire time by SetMuxHat(), not here. Every
    // other voice can have its sample-and-hold set half a step early - that is
    // what [B-07] moved off the trigger edge - because their level is taken at
    // the trigger and the note is finished long before the next charge. The hat
    // is the exception: it rings for a long time and CH and OH share one cap, so
    // setting the *next* step's level halfway through the current note changed
    // the level of a hat that was still sounding. A strong CH followed by a weak
    // one stepped down in the middle of its own decay, which the ear reads as a
    // second articulation - a flam. Worst after OH, which rings longest.
    //
    // [7A] CRASH and RIDE join it, charged at fire time by SetMuxCym(). They ring
    // longer than the hat does; the only reason they were not doing this already
    // is that their level never used to change.
    if (muxInst[a + 5] == CH || muxInst[a + 5] == CRASH || muxInst[a + 5] == RIDE) continue;

    if(bitRead(temp_stepValue, muxInst[a + 5])){
      // [B-31] Bit 7 is the flam flag and belongs to the five first-mux voices
      // only, so it must never reach the DAC here: velocity+128 clamps to full
      // scale and throws the weak/strong distinction away.
      SetDacA(pattern[ptrnBuffer].velocity[muxInst[a + 5]][LaneIndex(muxInst[a + 5], stp)] + acc);   // [F27]
      PORTA = (PORTA & 0b00011111) | muxAddr[a];//Mask to clear last 3 bits of the PORTA
      SelectSecondMux();//Set the value to the multiplexer out
    }
  }
  //DeselectSecondMux();//Unselect Multiplexer


  //Select First Multiplexer
  for (byte a = 0; a < 5; a++){
    if(bitRead (temp_stepValue, muxInst[a])){
      if (bitRead(pattern[ptrnBuffer].flamMask[muxInst[a]], LaneIndex(muxInst[a], stp)))                        // [F27]
        evtFlamWord |= ((unsigned int)1 << muxInst[a]);                               // [B-07] belongs to the pending step
      SetDacA(pattern[ptrnBuffer].velocity[muxInst[a]][LaneIndex(muxInst[a], stp)] + acc);//Set DAC Value       // [F27]
      //Set Multiplexer address
      PORTA = (PORTA & 0b00011111) | muxAddr[a];//Mask to clear last 3 bits of the PORTA
      SelectFirstMux();//Set the value to the multiplexer out
    }
  }
  //DeselectFirstMux();//Unselect Multiplexer

}


// [B-07] Renders the flam hit for the step it belongs to (flamStep), not for
// whatever the playhead has since been armed to play next.
void SetMuxFlam(byte stp) {

 const unsigned int acc = (bitRead(pattern[ptrnBuffer].inst[TOTAL_ACC], stp)) ? (pattern[ptrnBuffer].totalAcc * 4) : 0;

 //Select First Multiplexer
 for (byte a = 0; a < 5; a++){
    if(bitRead (stepValueFlam, muxInst[a])){
      //DeselectFirstMux();//Unselect Multiplexer
      SetDacA(pattern[ptrnBuffer].velocity[muxInst[a]][LaneIndex(muxInst[a], stp)] + acc);// [3B][F27]
      //Set Multiplexer address
      PORTA = (PORTA & 0b00011111) | muxAddr[a];//Mask to clear last 3 bits of the PORTA
      SelectFirstMux();//Set the value to the multiplexer out
    }
  }
  //DeselectFirstMux();//Unselect Multiplexer

  // [B-31] No second-multiplexer pass here, and that is correct rather than an
  // omission: flam belongs to BD, SD, LT, MT and HT only, and those are exactly
  // the first mux. Adding one would let a voice render a flam it cannot be
  // given, which is audible as a second hit 20-48 ms late.
}


void SetMuxTrig(int data)
{
  //cli();
  for ( byte x = 0; x < NBR_MUX; x++)
  {
    delayMicroseconds(30);//Solve HT velocity response issue
    bitWrite(MUX_INH_PORT, MUX_INH1_BIT, x);//x);
    bitWrite(MUX_INH_PORT, MUX_INH2_BIT, !x);  
    for ( byte a = 0; a < 5; a++){
      SetDacA(data);
      PORTA = dinStartState | dinClkState << 1 | 1 << 2 | (muxAddr[a] << 5);
    }
  }
  //sei();
}

void SetMuxTrigMidi(byte inst, byte velocity)
{
  //Remember velocity of each MIDI trigged instrument
  midiVelocity[inst] = velocity;

  switch (inst){
  case BD:
    //DeselectFirstMux();//Unselect Multiplexer                                                     // [1.028] this caused cuts in the digital section when a first mux instrument was played shortly after. (the other mux stays selected!) 
    SetDacA(midiVelocity[inst]);//Set DAC Value 
    //Set Multiplexer address
    PORTA = (PORTA & 0b00011111) | muxAddr[2];//Mask to clear last 3 bits of the PORTA
    SelectFirstMux();//Set the value to the multiplexer out
    break;
  case SD:
    //DeselectFirstMux();//Unselect Multiplexer
    SetDacA(midiVelocity[inst]);//Set DAC Value 
    //Set Multiplexer address
    PORTA = (PORTA & 0b00011111) | muxAddr[1];//Mask to clear last 3 bits of the PORTA
    SelectFirstMux();//Set the value to the multiplexer out
    break;
  case LT:
    //DeselectFirstMux();//Unselect Multiplexer
    SetDacA(midiVelocity[inst]);//Set DAC Value 
    //Set Multiplexer address
    PORTA = (PORTA & 0b00011111) | muxAddr[0];//Mask to clear last 3 bits of the PORTA
    SelectFirstMux();//Set the value to the multiplexer out
    break;
  case MT:
    //DeselectFirstMux();//Unselect Multiplexer
    SetDacA(midiVelocity[inst]);//Set DAC Value 
    //Set Multiplexer address
    PORTA = (PORTA & 0b00011111) | muxAddr[3];//Mask to clear last 3 bits of the PORTA
    SelectFirstMux();//Set the value to the multiplexer out
    break;
  case HT:
    //DeselectFirstMux();//Unselect Multiplexer
    SetDacA(midiVelocity[inst]);//Set DAC Value 
    //Set Multiplexer address
    PORTA = (PORTA & 0b00011111) | muxAddr[4];//Mask to clear last 3 bits of the PORTA
    SelectFirstMux();//Set the value to the multiplexer out
    break;
  case RM:
    //DeselectSecondMux();//Unselect Multiplexer
    SetDacA(midiVelocity[inst]);//Set DAC Value 
    //Set Multiplexer address
    PORTA = (PORTA & 0b00011111) | muxAddr[1];//Mask to clear last 3 bits of the PORTA
    SelectSecondMux();//Set the value to the multiplexer out
    break;
  case HC:
    //DeselectSecondMux();//Unselect Multiplexer
    SetDacA(midiVelocity[inst]);//Set DAC Value 
    //Set Multiplexer address
    PORTA = (PORTA & 0b00011111) | muxAddr[0];//Mask to clear last 3 bits of the PORTA
    SelectSecondMux();//Set the value to the multiplexer out
    break;
  case OH:
  case CH:
    //DeselectSecondMux();//Unselect Multiplexer
    // [B-37] A tapped or MIDI-played hat sets the same shared cap, so record it
    // as what the cap now holds. Otherwise the next step would "refresh" it back
    // to the sequencer's last level while the tapped hat was still ringing -
    // the same mid-decay step this fix exists to remove.
    hatCapLevel = midiVelocity[inst];
    SetDacA(midiVelocity[inst]);//Set DAC Value
    //Set Multiplexer address
    PORTA = (PORTA & 0b00011111) | muxAddr[2];//Mask to clear last 3 bits of the PORTA
    SelectSecondMux();//Set the value to the multiplexer out
    break;
  // [7A] Same bookkeeping the hat needs above: a tapped or MIDI-played cymbal is
  // what its cap now holds, so the next step's refresh leaves it there instead of
  // stepping it back to the sequencer's last level while the note is still ringing.
  case RIDE:
    //DeselectSecondMux();//Unselect Multiplexer
    cymCapLevel[1] = midiVelocity[inst];
    SetDacA(midiVelocity[inst]);//Set DAC Value
    //Set Multiplexer address
    PORTA = (PORTA & 0b00011111) | muxAddr[4];//Mask to clear last 3 bits of the PORTA
    SelectSecondMux();//Set the value to the multiplexer out
    break;
  case CRASH:
    //DeselectSecondMux();//Unselect Multiplexer
    cymCapLevel[0] = midiVelocity[inst];
    SetDacA(midiVelocity[inst]);//Set DAC Value
    //Set Multiplexer address
    PORTA = (PORTA & 0b00011111) | muxAddr[3];//Mask to clear last 3 bits of the PORTA
    SelectSecondMux();//Set the value to the multiplexer out
    break;
  }
}

//Select IC114
void SelectFirstMux(){
  delayMicroseconds(15);                                          //Wait stabilisation  [zabox] moved for better visibility, 15us are enough (4,1v/(0.55v/us) slew rate + 4us settling time = max 12us, verified with scope)
  MUX_INH1_LOW;
  delayMicroseconds(5);                                           // [zabox] charge s/h caps
  MUX_INH1_HIGH;                                                  // [1.028] moved from deselect function
}


//Select IC111
void SelectSecondMux(){
  delayMicroseconds(15);                                          //Wait stabilisation  [zabox] moved for better visibility, 15us are enough
  MUX_INH2_LOW;
  delayMicroseconds(5);                                           // [zabox] charge s/h caps
  MUX_INH2_HIGH;                                                  // [1.028] moved from deselect function
}


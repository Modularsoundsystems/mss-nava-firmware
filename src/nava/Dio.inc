//-------------------------------------------------
//                  NAVA v1.x
//                  Dio utility
//-------------------------------------------------

// [B-11] SPI.usingInterrupt(255) passed an invalid interrupt number, so the
// library fell through to "interruptMode = 2" and every single SPI transaction
// executed a global cli()/SREG-restore - including transactions already running
// inside an ISR, where it is pure overhead.
//
// It could not simply be deleted: it was also, by accident, the only thing
// making SetDoutTrig()/SetDacA() safe to call from both the main loop and the
// three timer ISRs. These macros replace it with an explicit critical section
// scoped to the actual transfer. Inside an ISR the cli() is a no-op and the
// SREG restore puts back I=0, so the same call sites work from either context.
#define SPI_ATOMIC_BEGIN  { uint8_t _sreg_spi = SREG; cli()
#define SPI_ATOMIC_END    SREG = _sreg_spi; }

/////////////////////Function//////////////////////
//Initialise IO PORT and libraries
void InitIO()
{
  DDRB = DDRB | B11011000;
  DDRD = DDRD | B11110000;
  DDRA = DDRA | B11101100;
  TRIG_HIGH;
  pinMode(DIN_START_PIN, OUTPUT);
  pinMode(DIN_CLK_PIN, OUTPUT);
  pinMode(TRIG1_PIN, OUTPUT);
  pinMode(TRIG2_PIN, OUTPUT);
  pinMode(ENC_PINA, INPUT);
  pinMode(ENC_PINB, INPUT);
  SPI.begin();
  // [B-11] SPI.usingInterrupt(255) removed - see SPI_ATOMIC_BEGIN above.
  Wire.begin();
}

//Scan DIN shift register with debounce time-------------------------------
void ScanDin()
{
  if ((millis() - debounceTimer) > DEBOUNCE_TIME)
  {
    debounceTimer = millis();
    //shiftregisted switches
    SPI_ATOMIC_BEGIN;                     // [B-11] whole 5-byte chain must not interleave
    SPI.beginTransaction(SPIset_f);
    SW_CS_HIGH;
    tempDin[0][0] = SPI.transfer(0);
    tempDin[1][0] = SPI.transfer(0);
    tempDin[2][0] = SPI.transfer(0);
    tempDin[3][0] = SPI.transfer(0);
    tempDin[4][0] = SPI.transfer(0);
    SW_CS_LOW;
    SPI.endTransaction();
    SPI_ATOMIC_END;

    //µC direct pin switches
    encSwValue[0] = ENC_SW_GET;

    //Compare
    for (int a = 0; a < 5; a++){
      if (tempDin[a][0] == tempDin[a][1]) dinSr[a] = tempDin[a][0];
    }
    if (encSwValue[0] == encSwValue[1]) encSwState = encSwValue[0];
  }

  //shiftregisted switches
  SPI_ATOMIC_BEGIN;                       // [B-11]
  SPI.beginTransaction(SPIset_f);
  SW_CS_HIGH;
  tempDin[0][1] = SPI.transfer(0);
  tempDin[1][1] = SPI.transfer(0);
  tempDin[2][1] = SPI.transfer(0);
  tempDin[3][1] = SPI.transfer(0);
  tempDin[4][1] = SPI.transfer(0);
  SW_CS_LOW;
  SPI.endTransaction();
  SPI_ATOMIC_END;

  //µC direct pin switches
  encSwValue[1] = ENC_SW_GET;
}

//Set Dout LED------------------------------------------------------------
void SetDoutLed(unsigned int stepLed, unsigned int configLed, byte menuLed)
{
  SPI_ATOMIC_BEGIN;                       // [B-11]
  SPI.beginTransaction(SPIset_f);
  LED_CS_LOW;
  SPI.transfer(byte(stepLed >> 8));
  SPI.transfer(byte(stepLed));
  SPI.transfer(byte(configLed >>8));
  SPI.transfer(byte(configLed));
  SPI.transfer(byte(menuLed));
  LED_CS_HIGH;
  SPI.endTransaction();
  SPI_ATOMIC_END;
}

// [4C] The trigger pulse is a fixed length here: nothing shortens it, because
// nothing fires a step more than once.
byte TrigLenNow()
{
  return TRIG_LENGHT;
}

//Set Dout Trig------------------------------------------------------------
void SetDoutTrig(unsigned int value)
{
  SPI_ATOMIC_BEGIN;                       // [B-11] called from the loop AND three ISRs
  SPI.beginTransaction(SPIset_f);
  TRIG_CS_LOW;
  SPI.transfer(byte(value >> 8));
  SPI.transfer(byte(value));
  TRIG_CS_HIGH;
  SPI.endTransaction();
  lastDoutTrig = value;
  // [6G] Everything that fires a trigger comes through here - the sequencer's
  // step, each swing group, a roll, a repeat, a hand-played pad, a note off the
  // MIDI input - so accumulating the bits here is the one place that sees all of
  // them. A pulse is about 2 ms and SetLeds() runs at roughly 80 Hz, so sampling
  // this word directly would miss most hits; OR-ing instead means nothing is
  // missed however brief it was, and SetLeds() empties it when it has shown it.
  //
  // Already past the mute mask - EmitGroup() applies that before calling - so a
  // muted voice does not light, which is what "the triggers" means.
  //
  // Inside the critical section with the rest: this is a read-modify-write on a
  // 16-bit word from three ISRs and the main loop.
  trigMonitor |= value;
  SPI_ATOMIC_END;                                                         // [zabox] [1.027] save last trigger value
}

//Reset Dout Trig---------------------------------------------------------
void ResetDoutTrig()
{
  SPI_ATOMIC_BEGIN;                       // [B-11]
  SPI.beginTransaction(SPIset_f);
  TRIG_CS_LOW;
  SPI.transfer(0);
  SPI.transfer(0);
  TRIG_CS_HIGH;
  SPI.endTransaction();
  SPI_ATOMIC_END;
}

//Set Dac voltage---------------------------------------------------------
// [B-30] Was SetDacA(byte). Every caller passes velocity + acc, where acc is
// totalAcc*4 - an int expression that was truncated to 8 bits at the call
// boundary and only then clamped, so any sum over 255 wrapped to a small
// number and the note came out near-silent instead of loud. Widen the
// parameter so the clamp happens before anything is thrown away.
void SetDacA(int velocity)
{

  //Serial.print(velocity);                                                      // test
  if (velocity < 0) velocity = 0;
  if (velocity > MAX_VEL) velocity = MAX_VEL;
  //unsigned int milliVolt = map(velocity, 0, 127, 0, 4090);                   // [zabox] very slow
  unsigned int milliVolt = velocity << 5;                                  // [zabox] [1.028] fast. halves the mux execution time, but needs a 5v readjustment. (26mv/32mv offset at full velocity)
  unsigned int command = 0;
  command |= 0x1000;// Active DAC out A
  command |= (milliVolt & 0x0FFF);
  SPI_ATOMIC_BEGIN;                       // [B-11] called from the loop AND two ISRs
  SPI.beginTransaction(SPIset_f);
  DAC_CS_LOW;
  SPI.transfer(command >> 8);
  SPI.transfer(command & 0xFF);
  DAC_CS_HIGH;
  SPI.endTransaction();
  SPI_ATOMIC_END;
}

//Scan Din bootloader
void ScanDinBoot()
{
  SPI_ATOMIC_BEGIN;                       // [B-11]
  SPI.beginTransaction(SPIset);
  SW_CS_HIGH;
  SPI.transfer(0);
  SPI.transfer(0);
  btnPlayStopByte = SPI.transfer(0);
  SPI.transfer(0);
  btnEnterByte = SPI.transfer(0);
  SW_CS_LOW;
  SPI.endTransaction();
  SPI_ATOMIC_END;
}


//Set the trig lenght value before reset trig out
void SetTrigPeriod(byte ms)
{ 
 
  static unsigned long trigPeriod;

  if (trigCounterStart){
    trigPeriod = millis();
    trigCounterStart = FALSE;
  }
  
  // [zabox] if the isr starts here in between, trigPeriod hasn't been updated and millis()-trigPeriod is still true < 10ms.  
  
  if ((millis() - trigPeriod >= ms) && !(PORTA & (1 << 2)) && trigCounterStart == FALSE) {              // [zabox] checking trigCounterStart == false fixes short trig out bug. 
    TRIG_HIGH;// Reset trigOut
  }

}


//-------------------------------------------------
//                  NAVA v1.x
//             Timer Configuration
//-------------------------------------------------

void TimerStart()
{
  TCCR1A = TCCR1B = 0;
  //prescale 8 => 16000000/8 = 2000000 Hz by tick
  TCCR1B |= _BV (CS11) | _BV (WGM12);
  TimerSetFrequency();
  TIMSK1 |= _BV(OCIE1A);

}

void TimerStop(void)
{
  bitWrite(TIMSK1, OCIE1A, 0);
  TCCR1A = TCCR1B = OCR1A = 0;
}

void TimerSetFrequency()
{
  // Timer1 top for a tick rate of (BPM*PPQN)/60.
  //
  // [F30] Computed from tenths of a BPM rather than whole ones, which is why
  // this went from one 16-bit divide to a 32-bit one: (F_CPU/8)*600 is 1.2e9 and
  // needs the width. It runs once per encoder detent, not per tick.
  const unsigned int bpm10 = seq.bpm * 10 + seq.bpmFine;
  const unsigned int newTop = (unsigned int)(((F_CPU / 8UL) * 600UL)
                                             / ((unsigned long)bpm10 * PPQN));

  // [B-08] In CTC mode, storing an OCR1A that is already below the live TCNT1
  // means this period's compare has been missed: the counter runs on to 0xFFFF
  // and wraps, swallowing one tick (up to ~32 ms at prescaler 8). Turning the
  // tempo encoder during playback hit this on every detent - the audible
  // hiccup. Pull TCNT1 back inside the new period so the compare still fires.
  // The OCR1A write and the TCNT1 test must not be split by the Timer1 ISR.
  uint8_t sreg = SREG;
  cli();
  OCR1A = newTop;
  if (TCNT1 >= newTop) TCNT1 = newTop - 1;   // tick is already due: fire next cycle
  SREG = sreg;
}




// [B-09] Set Timer1's period from a measured interval in microseconds. Prescaler
// 8 on a 16 MHz part is 2 counts per microsecond, so the top is us*2 - and the
// same [B-08] clamp applies: an OCR1A already below the live TCNT1 means this
// period's compare has been missed and the counter would run to 0xFFFF.
void TimerSetPeriodUs(unsigned int us)
{
  unsigned long top = (unsigned long)us * 2UL;
  if (top < 1500UL)  top = 1500UL;      // ~750 us/tick, far above any real tempo
  if (top > 60000UL) top = 60000UL;     // ~30 ms/tick, far below one

  uint8_t sreg = SREG;
  cli();
  OCR1A = (unsigned int)top;
  if (TCNT1 >= (unsigned int)top) TCNT1 = (unsigned int)top - 1;
  SREG = sreg;
}

void initTrigTimer() {                         // [zabox] [v1.028] timer 2 setup for 2ms interrupt

  TCCR2A = _BV(WGM21);                           // Set timer to ctc mode
  //TCCR2B = _BV(CS22) | (_BV(CS20);             // Set prescaler to 128
  TRIG_TIMER_STOP;
  TRIG_TIMER_ZERO;
  OCR2A = 249;                                   // Set output compare register to 2ms
  TIMSK2 = _BV(OCIE2A);                          // Enable OC interrupt
 
}


void initExpTimer() {                         // [zabox] [v1.028] timer 2 setup for 2ms interrupt

  TCCR2A = 0;                                    // Set timer to normal mode
  TCCR2B = _BV(CS22) | _BV(CS21) | _BV(CS20);    // Set prescaler to 128
  TIMSK2 = 0;                                    // Enable OC interrupt
 
}


void initFlamTimer() {                        // [zabox] [v1.028] timer 3 setup for 2ms interrupt

 TCCR3A = 0;                                    // initialize
 TCCR3C = 0;                                    // initialize
 FLAM_TIMER_STOP;
 cli();
 FLAM_TIMER_ZERO;
 OCR3A = flam[1];                               // Set output compare register to 2ms
 sei();
 TIMSK3 = _BV(OCIE3A);                          // Enable OC interrupt
 
}



void setFlam() {
  if (OCR3A != (unsigned int)flam[pattern[ptrnBuffer].flam]) {   // OCR3A is unsigned, flam[] is int
    OCR3A = flam[pattern[ptrnBuffer].flam];                              // Set output compare register to 2m
  }
}
  
  

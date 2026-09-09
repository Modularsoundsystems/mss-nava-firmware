//-------------------------------------------------
//                  NAVA v1.x
//                  main program
//-------------------------------------------------

/////////////////////Include/////////////////////
#include <Arduino.h>
#include <SPI.h>
#include <LiquidCrystal.h>     // [zabox] [1.028] (still working)
#include <WireN.h>             // [zabox] [1.028]  vendored - BUFFER_LENGTH 66
#include "nava/define.h"
#include "nava/string.h"
#include <MIDI.h>              // vendored v3.2 - DO NOT replace with 4.x/5.x

#if DEBUG
  #include <MemoryFree.h>      // only referenced inside #if DEBUG blocks
#endif

// [3A] lcdHw is the panel; `lcd` is a framebuffer in front of it. The layout
// code in LCD.inc is unchanged - it writes to the frame, LcdFlush() sends only
// the cells that differ. See nava/lcdbuf.h.
LiquidCrystal lcdHw(18, 19, 20, 21, 22, 23);

#include "nava/lcdbuf.h"
#include "nava/prototypes.h"

LcdBuffered lcd;

////////////////////////Setup//////////////////////
void setup()
{
  InitIO();//cf Dio
  InitButtonCounter();//cf Button
  
  SetDoutTrig(0);                                           // [zabox] [1.028] no random trigger pin states at startup (bd can oscillate with open trigger)
  SetDoutLed(0, 0, 0);                                      //                 no random leds at startup
  
  initTrigTimer();                                          // [zabox] [1.028]  init 2ms trig timer
  initFlamTimer();                                          // flam
  
  lcd.begin(16, 2);                                         // [zabox] [1.028] must be executed before chreateChar with the new library
  LcdBufInit();                                             // [3A] begin() leaves the panel blank; match it
  lcd.createChar(LCD_BAR_LIGHT,  fontBarLight);             // [3E][3F] the bars
  lcd.createChar(LCD_BAR_FULL,   fontBarFull);
  lcd.createChar(LCD_GLYPH_NOTE, font5);


  ScanDinBoot();
  //Init EEprom-------------------------------------
  if (btnPlayStopByte == (BTN_PLAY | BTN_STOP)){
    LcdPrintEEpromInit();
    LcdFlush();                                             // [3A]
    bootElapseTime = millis();
    while (1){
      ButtonGet();
      if((millis() - bootElapseTime) > BOOTLOADER_TIME) break;
      if (playBtn.pressed && enterBtn.pressed){
        InitEEprom();
        //InitEEpromTrack();//problem with init pattern 0 to 18: to be solved
        //InitSeqSetup();
        break;
      }
    }
  }
  // [3B] EEprom self-test. Checked before the ENTER branch below, since
  // holding STOP+ENTER also satisfies btnEnterByte == BTN_ENTER.
  else if (btnPlayStopByte == BTN_STOP && btnEnterByte == BTN_ENTER){
    EepromSelfTest();
  }
  //TM2 adjustement for velocity
  else if (btnEnterByte == BTN_ENTER){
    LcdPrintTM2Adjust();
    LcdFlush();                                             // [3A]
    while (1){
      SetDacA(MAX_VEL);
    }
  }


  // [3B] Detect the bank format before anything reads a pattern. Offers
  // conversion or a fresh bank if this chip is not already format v2.
  BankCheckBoot();

  InitSeq();// cf Seq
  //Load default track
  LoadTrack(0);
  //Load default pattern
  LoadPattern(0);
  ptrnBuffer = !ptrnBuffer;
  InitPattern();
  SetHHPattern();                                           // [B-21] InstToStepWord() removed
  SetMuxTrigMidi(RM, 0);                                    // [zabox] workaround. without the line, the first played step/instrument after power on had no sound
  SetDoutTrig(0);
  

  MIDI.begin();//Serial1.begin(MIDI_BAUD);
  //MIDI.setHandleNoteOn(HandleNoteOn);                     // [zabox] [1.028] moved bc expander mode
  //MIDI.setHandleNoteOff(HandleNoteOff);                   // 
  MIDI.setInputChannel(seq.RXchannel > MAX_MIDI_CHAN ? 1 : seq.RXchannel);   // [5I] 17 is OFF
  MIDI.turnThruOff();                                       // [zabox] fixes double real time messages on midi out
  // [SysEx] Always connected. SetSeqSync() swaps the realtime and note handlers
  // per sync mode but never touches this one, and system messages bypass the
  // channel filter, so dump/restore works in every mode and on any RX channel.
  MIDI.setHandleSystemExclusive(HandleSystemExclusive);
  // [3I] Program change picks a pattern, in every sync mode. Connected here
  // rather than in SetSeqSync() for the same reason SysEx is: that function
  // tears the note and realtime handlers down as the mode changes, and neither
  // of these has anything to do with where the clock comes from.
  MIDI.setHandleProgramChange(HandleProgramChange);


#if DEBUG
  Serial.begin(115200);
  Serial.print("freeMemory()=");
  Serial.println(freeMemory());

#endif
  sei();

  //-----------------------------------------------

  // Splash. Two stages: the instrument and its origin, then this firmware.
  // A derivative work should still say whose hardware it is.
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("    MSS NAVA    "));
  lcd.setCursor(0, 1);
  lcd.print(F(" e-licktronic   "));
  LcdFlush();
  delay(700);
  lcd.setCursor(0, 0);
  lcd.print(F("firmware " FW_VERSION));
  lcd.setCursor(0, 1);
  // [3B] LoadPattern() replaces any record that fails its magic check with a
  // blank one rather than playing its bytes as if they meant something. Say so:
  // silently substituting an empty pattern for someone's work is worse than
  // the noise it prevents.
  // The build stamp, so "which firmware is on this chip" is answerable from the
  // front panel instead of by inference. A bad bank still outranks it - that is
  // news you need before anything else.
  if (bankFormatBad) lcd.print(F("BAD RECORD FOUND"));
  else {
    lcd.print(F("build "));
    lcd.print(F(FW_BUILD));
    lcd.print(F("   "));
  }
  LcdFlush();
  delay(700);
  LcdUpdate();                                              // [1.028] if started in expader mode

}

////////////////////////Loop///////////////////////
void loop()
{
  
  Expander();                                                             // [1.028] expander
  SetTrigPeriod(TrigLenNow());                                            // [4C] shorter as repeats get denser
  InitMidiRealTime();

  // [3G] Drain the MIDI input, rather than taking one byte per pass.
  //
  // MIDI v3.2's parse() extracts exactly one byte per call, so a single read()
  // here caps MIDI intake at the main loop rate. At 31250 baud the line delivers
  // 3,125 bytes a second, and during a SysEx restore the loop has to beat that
  // while also servicing every 81-byte page message with a ~6 ms blocking EEPROM
  // write. It cannot, so the 64-byte RX buffer overruns and bytes are lost from
  // the middle of a message; the decode then comes up short of 64 and the page is
  // dropped. Silently - a restore has no acknowledgement, so nothing says so.
  //
  // The deficit is structural rather than transient, so once behind it never
  // catches up: the first page or two land while the buffer is still empty and
  // the rest of the transfer is thrown away. That is a restore that writes one
  // partial pattern out of 128 and reports success.
  //
  // Bounded so a flood cannot own the loop. 64 is the whole buffer, so one pass
  // empties it, and at any loop rate above 50 Hz that is twenty times the line
  // rate. Ordinary traffic - clock at 48 bytes a second, the odd note - leaves
  // the loop on its first test, so this costs nothing when nothing is arriving.
  {
    byte guard = 0;
    while (USE_SERIAL_PORT.available() && guard++ < 64) MIDI.read();
  }
  //SetMux();//!!!! if SetMUX() loop there is noise on HT out and a less noise on HH noise too !!!!
  ButtonGet(); 
  EncGet();
  
  if (ledUpdateCounter > 2) {                                             // [zabox] [1.028] smooth leds (combine 3 loop cycles, reduces update rate from ~220hz with running secuencer to 80hz)
    SetLeds();
    ledUpdateCounter = 0;
  } ledUpdateCounter++;
  
  SeqConfiguration();
  SeqParameter();
  KeyboardUpdate();
  LcdUpdate();
  

  

   
  
#if DEBUG
    if (stepValue) {
      if (stepValue != stepValue_old) {
       // Serial.println(stepValue, BIN);
       
       
        stepValue_old = stepValue;
      }
    }
    if (ppqn != ppqn_old) {
      //Serial.println(ppqn);
      ppqn_old = ppqn;
    }
    
    
#endif



}






























//=====================================================================
// [Phase 0] Single translation unit.
//
// The Arduino IDE concatenated the main sketch followed by every other
// .ino in alphabetical order into one file. That is reproduced exactly
// here: same order, same single TU, so every global in define.h stays
// visible everywhere and behaviour is unchanged.
//
// Do NOT rename these to .cpp - PlatformIO would compile each as its own
// translation unit and every global in define.h would multiply-define.
// Splitting properly (extern declarations + a globals.cpp) is a later
// phase, not a migration task.
//=====================================================================
#include "nava/Button.inc"
#include "nava/Clock.inc"
#include "nava/Cond.inc"
#include "nava/Dio.inc"
#include "nava/EEprom.inc"
#include "nava/Browse.inc"    // after EEprom.inc: uses PatternAddr/TrackAddr
#include "nava/Lane.inc"
#include "nava/Enc.inc"
#include "nava/Expander.inc"
#include "nava/Input.inc"
#include "nava/key.inc"
#include "nava/LCD.inc"
#include "nava/Led.inc"
#include "nava/Midi.inc"
#include "nava/Name.inc"
#include "nava/Mux.inc"
#include "nava/Selftest.inc"    // after EEprom.inc: uses its page helpers
#include "nava/Seq.inc"
#include "nava/SeqConf.inc"
#include "nava/SeqFunc.inc"
#include "nava/SysEx.inc"      // after EEprom.inc: uses its page/offset defines
#include "nava/Undo.inc"
#include "nava/Util.inc"
#include "nava/timer.inc"

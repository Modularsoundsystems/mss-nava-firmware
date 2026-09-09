//-------------------------------------------------
//                  NAVA v1.x
//                    define
//-------------------------------------------------

#ifndef define_h
#define define_h

//Firmware identity---------------------------------
// Kept here so the splash, any future SysEx identity reply and the docs all
// read from one place. Bump FW_VERSION with anything that changes the EEPROM
// format, so a mismatched pattern bank can be spotted from the front panel.
#define FW_NAME     "MSS NAVA"
#define FW_VERSION  "1.0"
// Stamped by tools/gitrev.py at build time. A trailing '+' means the tree had
// uncommitted changes. See the splash screen.
#ifndef FW_BUILD
#define FW_BUILD    "nobuild"
#endif
// [4X] The date of that commit, not of the compile. Two builds of the same
// source should say the same thing, and the question the date answers is how old
// the firmware is - which is a fact about the source, not about when someone
// last happened to press build.
#ifndef FW_DATE
#define FW_DATE     "--------"
#endif

//DEBUG
#define DEBUG 0

  #if DEBUG
   unsigned int stepValue_old = 0;
   byte ppqn_old = 14;
  #endif
  

//MIDI
#define NOTE_ON_CMD 0x90
#define NOTE_OFF_CMD 0x80
#define CLOCK_CMD 0xF8
#define START_CMD 0xFA
#define STOP_CMD 0xFC
#define ALL_NOTE_OFF 0x7B
#define CONTINU_CMD 0xFB
#define MIDI_BAUD 31250

//Button
#define BTN_PLAY     B10000000
#define BTN_STOP     B1
#define BTN_LASTSTEP B1000000
#define BTN_SCALE    B10
#define BTN_SHUF     B100000
#define BTN_CLEAR    B100
#define BTN_INST     B10000
#define BTN_SHIFT    B1000
#define BTN_TRK  B1000
#define BTN_BACK     B100
#define BTN_FWD      B10
#define BTN_NUM      B1
#define BTN_PTRN B10000000
#define BTN_TAP      B1000000
#define BTN_DIR      B100000
#define BTN_GUIDE    B10000
#define BTN_BANK     B1
#define BTN_MUTE     B10
#define BTN_TEMPO    B100
#define BTN_ENTER    B1000
#define BTN_ENCODER  B100
#define ENC_SW_GET   PINB & BTN_ENCODER
#define ENC_PINA    0
#define ENC_PINB    1
#define JUSTPRESSED 1
#define JUSTRELEASE 0
#define HOLD_TIME 1000 //1s to trig hold 
#define DOUBLE_TAP_TIME 400 // [F28] window for a double-tap latch
#define NBR_BTN_STEP 16

//Led
#define LED_STOP  B1
/*#define LED_SHIFT B10
 #define LED_INST  B100
 #define LED_CLR   B1000
 #define LED_SHUF  B10000
 #define LED_LAST  B100000
 #define LED_SCALE B1000000*/
#define LED_PLAY  B10000000
#define LED_MASK  0xDD55 //mask to fade selected inst led
#define LED_MASK_OH  0xFD55 

//LCD
#define MAX_CUR_POS 4
#define MAX_CONF_PAGE 5
// [3T] Reordered by how often you reach for them while playing. Paste contents
// changes between one pattern and the next; sync and MIDI channels are set once
// when the rig is wired and then never again, so they no longer sit in front of
// everything else.
// [4E][4X] Grouped by subject rather than by the order they were written, then
// ordered by how often a hand reaches for them mid-set. The hi-hats had three
// settings spread over two pages - priority on one, mute pairing beside it,
// open-hat selection on another - which is three answers to one question filed
// in two places. They are one page now.
#define CONF_PAGE_BEHAV 1     // pattern change timing, single trig, tap over mute
#define CONF_PAGE_HAT   2     // [4X] everything about the two hi-hat lanes
// [5G] The paste page has gone. Its mask is editable from inside the copy and
// paste chords themselves, and those now hold on their second key alone - so the
// page was a second editor for one byte, reachable only by a different route,
// with nothing to say which of the two you had last used. Hold SHIFT+COPY, set
// the mask, let go: a copy overwrites the copy buffer and nothing else, which
// makes it the side-effect-free way in.
#define CONF_PAGE_SYNC  3     // sync source, tempo, MIDI channels
#define CONF_PAGE_SYSEX 4     // [F23] send a dump from the panel
// [4X] Last, and readable while playing: it answers a question you ask rarely
// and urgently - which firmware is actually on this chip - and the answer must
// not require stopping a set to get at it. Nothing on it is editable.
#define CONF_PAGE_FW    5


// Negative is detents per unit, positive is the ceiling for a fast spin, and 1
// is one for one either way. Ordered, so a turn to the left is always slower.

// One knob job's memory: when its last detent arrived, which way it was going,
// and how many clicks it has banked toward the next unit. Per job, so slowing
// the tempo knob cannot leave the pattern scroll half-spent, and a deliberate
// turn of one is not read as fast because the other was spun a moment earlier.

// [4E] Everything except the two that cannot be touched under a running
// sequencer: sync reconfigures the clock, and the dump blocks the panel for the
// best part of a minute. Saving still waits for the stop either way.
static inline boolean ConfPageLive(byte p)
{
  return (p != CONF_PAGE_SYNC && p != CONF_PAGE_SYSEX);
}

// [F23] What the dump page will send. A restore needs nothing here - incoming
// SysEx is handled whenever it arrives, in any mode - so this page only sends.
#define DUMP_ALL   0          // patterns, tracks and setup - a real backup
#define DUMP_PTRN  1          // the pattern currently selected
#define DUMP_TRK   2          // all sixteen tracks
#define DUMP_SETUP 3          // the global setup page
#define DUMP_MAX   3
byte dumpWhat = DUMP_ALL;

//Utility
// [3A] TOGGLE / MOMENTARY removed with StepButtonGet(), their only user.
#define TRUE 1
#define FALSE 0
#define ON 1
#define OFF 0
#define BOOTLOADER_TIME 5000 // time staying in bootloader 

//Sequencer
#define NBR_INST 16
#define NBR_STEP_BTN 16
#define NBR_STEP 16
#define NBR_PATTERN 16 //Number of pattern in a bank
#define NBR_BANK 8
#define MID_VEL 40 //velocity 0 to 127
#define HIGH_VEL 80
#define MAX_VEL 127 
#define MAX_SEQ_DIR 3
const char dirShortName[16][4] PROGMEM = {
  "rev", "rn4", "rn2", "rn1", "rnb", "bea", "lpr", "lpx",
  "rr4", "rrx", "sr2", "sr4", "bl2", "bl3", "bl4", "rsh"};

boolean          backChordUsed = FALSE;      // [F8] a pad joined this << hold
boolean          tapChordUsed  = FALSE;      // [F8] ...or this TAP hold
boolean          dirChordUsed  = FALSE;      // [F8] ...or this DIR hold
boolean          guideChordUsed = FALSE;     // a modifier or a hold spent this press
unsigned long    guidePending  = 0;          // [F17] a GUIDE tap awaiting its double
unsigned long    guideFxSince  = 0;          // [3W] when effects last held the key
// [3U] A paste waiting for its chord to be let go. The mask is editable while
// SHIFT+PASTE is held, so the paste has to happen after you stop editing it.
boolean          pasteArmed    = FALSE;
boolean          fwdChordUsed  = FALSE;      // [F8] ...or this >> hold

// [3C] The encoder edits whatever the held modifier is about. Scrolling a
// pattern or a track has to wait for the key to come up before it commits -
// each step of it is a 30-50 ms EEprom read, and doing that per detent would
// turn the knob into a stutter.
byte    encPtrnScroll = 0;
byte    encTrkScroll  = 0;
boolean encPtrnArmed  = FALSE;
boolean encTrkArmed   = FALSE;
byte    encFxScroll   = 0;


const byte scatLight[8] PROGMEM = {0, 0, 1, 1, 2, 0, 1, 2};
const byte scatHeavy[8] PROGMEM = {0, 1, 2, 3, 4, 5, 6, 2};

#define FORWARD 0
#define BACKWARD 1
#define PING_PONG 2
#define RANDOM 3
#define MAX_BANK 7 //bank A to H
#define MAX_PTRN 128
#define MAX_TRACK 16
#define MAX_PTRN_TRACK 496                                  // [3B] was 1024; see struct Track
// [5I] MIDI channels run 1..16; 17 is OFF, and OFF means "ignore channel
// messages, keep system messages" - no notes and no program change, but clock,
// start, stop, continue and SysEx all still flow. That is the line the MIDI
// library itself already draws in input_filter(), so it is not a new idea, only
// one the panel can now reach.
//
// It is NOT the library's MIDI_CHANNEL_OFF. That is also 17, but read() opens
// with `if (inChannel >= MIDI_CHANNEL_OFF) return false` - it disables the whole
// input, clock and SysEx included, which is the opposite of what this is for.
// MIDI_CHANNEL_OMNI is 0 and means receive everything, so that spelling is wrong
// too. Both obvious answers are traps; the gating is ours.
//
// 17 rather than 0 as the sentinel so a blank EEprom byte still constrains up to
// channel 1. A 0 sentinel would make a fresh chip come up silent.
#define MAX_MIDI_CHAN 16
#define MIDI_CHAN_OFF 17

#define MIN_BPM 30
#define MAX_BPM 250
#define DEFAULT_BPM 120
#define PPQN 96
#define SCALE_16 PPQN/4           //24
#define SCALE_32 PPQN/8           //12
#define SCALE_16t PPQN/6          //16
#define SCALE_8t PPQN/3           //32
#define MAX_SHUF_TYPE 7
#define MAX_FLAM_TYPE 8                                     // [zabox] [1.027] flam
#define DEFAULT_FLAM 0                                      //
#define DEFAULT_SHUF 1              
#define DEFAULT_LEN 16
#define DEFAULT_SCALE 24
#define MASTER 0
#define SLAVE 1
#define EXPANDER 2                                          // [zabox] [1.028]
#define MAX_BLOCK_LEN 16
#define POSITION 0
#define LENGTH 1
#define MAX_TOTAL_ACC 13
#define MIN_TOTAL_ACC 0
#define SYNC 1
#define FREE 0

//Ext inst
#define MAX_OCT 8
#define DEFAULT_OCT 3 //corresponding to +0
#define MAX_EXT_INST_NOTE 99


//trig out  and dinsynchro
#define TRIG_HIGH PORTA |= 1 << 2
#define TRIG_LOW  PORTA &= ~(1 << 2)
#define TRIG1_PIN 26
#define TRIG2_PIN 27
#define DIN_START_PIN 24
#define DIN_CLK_PIN 25
#define TRIG_LENGHT 10 //10ms

//Dincsync out
#define DIN_START_HIGH PORTA |= 1
#define DIN_START_LOW  PORTA &= ~(1)
#define DIN_CLK_HIGH   PORTA |= 1 << 1
#define DIN_CLK_LOW    PORTA &= ~(1 << 1)

//Dio
#define SW_CS_LOW PORTD&=~(1<<4)
#define SW_CS_HIGH PORTD|=(1<<4) 
#define LED_CS_LOW   PORTB&=~(1<<4)
#define LED_CS_HIGH  PORTB|=(1<<4)
#define TRIG_CS_LOW   PORTB&=~(1<<3)
#define TRIG_CS_HIGH  PORTB|=(1<<3)
#define DAC_CS_HIGH  PORTD|=(1<<5)
#define DAC_CS_LOW  PORTD &=~(1<<5)

#define DEBOUNCE_TIME 5

//Mux
#define MUX_ADDR_PORT PORTA
#define NBR_MUX_OUT 5
#define NBR_MUX 2
#define MUX_INH_PORT PORTD 
#define MUX_INH1_BIT 6
#define MUX_INH2_BIT 7
#define MUX_INH1_HIGH PORTD|=(1<<6)
#define MUX_INH1_LOW PORTD &=~(1<<6)
#define MUX_INH2_HIGH PORTD|=(1<<7)
#define MUX_INH2_LOW PORTD &=~(1<<7)
#define MUX_ADDR_0 PORTA &=~(1<<7)&~(1<<6)&~(1<<5)
#define MUX_ADDR_1 PORTA |=(1<<7)&~(1<<6)&~(1<<5)

//Inst button
#define BD_BTN 0
#define BD_LOW_BTN 1
#define SD_BTN 2
#define SD_LOW_BTN 3
#define LT_BTN 4
#define LT_LOW_BTN 5
#define MT_BTN 6
#define MT_LOW_BTN 7
#define HT_BTN 8
#define HT_LOW_BTN 9
#define RM_BTN 10
#define HC_BTN 11
#define CH_BTN 12
#define CH_LOW_BTN 13
#define CRASH_BTN 14
#define RIDE_BTN 15

#define OH_BTN 12288   //bit 13 and bit 12 of a 16bits word
#define BD_F_BTN 3     //bit 0 and bit 1 of a 16bits word                 // [zabox] [1.027] flam buttons
#define SD_F_BTN 12    //bit 2 and bit 3 of a 16bits word                 //
#define LT_F_BTN 48    //bit 4 and bit 5 of a 16bits word                 //
#define MT_F_BTN 192   //bit 6 and bit 7 of a 16bits word                 //
#define HT_F_BTN 768   //bit 8 and bit 9 of a 16bits word                 //


//Inst Match with bit shift register out (cf schematic)
#define BD 8
#define SD 9
#define LT 10
#define MT 11
#define HT 2
#define RM 3
#define HC 4
#define HH 5
#define CRASH 7
#define RIDE 6
#define HH_SLCT 1
#define OH 15 //unused shift OUT
#define CH 14 //unused shift OUT
#define TRIG_OUT 0
#define TOTAL_ACC 12
#define EXT_INST 13

enum SeqMode
{
  TRACK_PLAY, TRACK_WRITE, PTRN_PLAY, PTRN_STEP, PTRN_TAP, MUTE
};

//////////////////////////////variable////////////////////////////////
//Dio---------------------------------------------
byte dinSr[5]={
  0};//Store Din value of the five shift register
byte tempDin[5][2];//Used for the bounce
unsigned long debounceTimer;

//Buttons-----------------------------------------
typedef struct Button Button;
struct Button
{
  boolean curState;//1 is ON, 0 is OFF
  boolean prevState;
  boolean pressed;
  boolean justPressed;
  boolean justRelease;
  boolean hold;// 1 is holding
  boolean latched;// [F28] double-tapped: behaves as held until tapped twice again
  boolean canLatch;// [F28] opt-in. Latching is a mode; most buttons must not have one
  byte counter;//count number of push on the button
  unsigned long curTime;
  unsigned long tapTime;// [F28] previous press, for double-tap detection
};

// [F28] "Is this modifier engaged?" - held, or latched by a double-tap.
// Read this rather than .pressed anywhere a button acts as a modifier, so a
// latched one behaves exactly as a held one and there is no second code path
// to keep in step.
static inline boolean BtnHeld(const Button *b) { return b->pressed || b->latched; }

// [3J] "Has this been down long enough that it is a hold and not a tap?"
//
// HOLD_TIME is a full second, which is right for a confirmation - executing the
// pattern utilities, saving a track - and far too slow for a performance
// gesture. A momentary mute you have to lean on for a second is not momentary,
// and a metronome that waits a second before deciding a press was not a tap
// spends most of its life toggling by accident.
//
// 300 ms is past any tap and under any deliberate hold, which is the whole
// distinction being drawn.
#define MOMENTARY_TIME 300
static inline boolean BtnHeldFor(const Button *b, unsigned int ms)
{
  return b->pressed && (millis() - b->curTime) > ms;
}
// [3A] stepBtn[] is gone. It was shared mutable state between four consumers
// that each kept their own idea of what had changed - see Input.inc for what
// that cost. The step buttons now live in the input layer below.
Button playBtn;
Button stopBtn;
Button encBtn;
Button guideBtn;
Button scaleBtn;
Button tapBtn;
Button dirBtn;
Button tempoBtn;
Button shufBtn;
Button enterBtn, numBtn, lastStepBtn, backBtn, fwdBtn, muteBtn, bankBtn, stepsBtn, ptrnBtn, trkBtn, clearBtn;

// [F28] The latch state for INST and SHIFT. They are read as plain levels all
// over the code, so the Button that tracks their edges lives beside them and
// the level is what carries the answer.
Button instLatchBtn, shiftLatchBtn;
boolean instBtn;
boolean shiftBtn;

boolean doublePush = 0; //flag that CH and CH_LOW button are pressed together                
byte instOut[NBR_INST]=  {
  BD, BD, SD, SD, LT, LT, MT, MT, HT, HT, RM, HC, CH, CH, CRASH, RIDE};

// [3S] The five voices that can flam - the ones with a low/high pad pair, whose
// double-push arms it. The flam timer fires one shared interval, so a voice
// without a pair has nothing to offset against.
static inline boolean InstCanFlam(byte z)
{
  return (z == BD || z == SD || z == LT || z == MT || z == HT);
}

#define NBR_VOICE 11
const byte voiceOrder[NBR_VOICE] = {
  BD, SD, LT, MT, HT, RM, HC, CH, OH, CRASH, RIDE};

#define LANE_SPAN  0
#define LANE_PAD  3

boolean stepLockout = FALSE;

byte          retrigTrig = 0;
boolean       retrigHat  = FALSE;

// [F10][F11] Instrument roll. Hold a pad in tap mode and that instrument repeats
// on a grid derived from the step, so the repeats land where the sequencer's own
// steps would rather than free-running from whenever the pad happened to go
// down - which is what makes a roll recordable straight into the pattern.
//
// Rate is hits per step, so it follows the scale and the shuffle: x4 is four to
// a step whether the step is a 1/16 or a 1/8 triplet, and x3 and x6 are the
// triplet settings against a straight one.
// Rates below x1 are a hit every N steps rather than N hits in a step, so the
// roll reaches from a slow half-time pulse up to a buzz without a second control.
#define ROLL_OFF 0
#define ROLL_MAX 9
const byte rollNum[ROLL_MAX + 1] = {0, 1, 1, 1, 1, 2, 3, 4, 6, 8};
const byte rollDen[ROLL_MAX + 1] = {1, 4, 3, 2, 1, 1, 1, 1, 1, 1};
const char *const rollName[ROLL_MAX + 1] = {
  "--", "/4", "/3", "/2", "x1", "x2", "x3", "x4", "x6", "x8"};

struct SubGrid { unsigned long ticks; byte forN; byte forScale; };
byte                  rollRate = ROLL_OFF;
volatile unsigned int rollMask = 0;      // instruments held down right now
byte                  rollVel[NBR_INST];

// tempInst is written by the tap handler in the loop and by the roll driver in
// the timer ISR. Its 16-bit read-modify-writes in the loop have to be atomic, or
// a roll hit landing inside one is read, overwritten and lost.
#define TEMPINST_ATOMIC_BEGIN  { uint8_t _tiSreg = SREG; cli()
#define TEMPINST_ATOMIC_END    SREG = _tiSreg; }

//Input layer [3A]----------------------------------
// Produced once per scan by InputScan(), read-only to everyone else. Bit n of
// each mask is step button n. Replaces stepBtn[], readButtonState, and the
// muteButtons/gateButtons shadow pairs, which were four separate answers to
// the same question.
unsigned int inStepNow      = 0;   // level now: 1 = down
unsigned int inStepPrev     = 0;   // level on the previous scan
unsigned int inStepPressed  = 0;   // rising edges seen on this scan
unsigned int inStepReleased = 0;   // falling edges seen on this scan
unsigned int inStepHeld     = 0;   // down longer than HOLD_TIME
byte         inStepCount    = 0;   // how many are down right now

// Chord: every step that took part since the first key went down. Stays valid
// for one scan after the last key comes up (inChordDone), so release-time
// handlers see the whole chord and not just the key that happened to be last.
// [4D] Voices tapped since the last bar line. The first tap of a cycle on a
// voice replaces what it played last cycle; the rest of that cycle adds to it.
volatile unsigned int cycTouched = 0;

// [4H] Cyclic record is a performance decision, not a preference: you turn it on
// for the take you are about to play and off again after. That belongs on a key,
// not four pages into a menu. SHIFT+STOP, so it is one deliberate chord rather
// than a timed gesture on the key that also arms recording. Not saved, for the
// same reason it is not in the menu.
boolean cycRec = FALSE;

// [4L] One hi-hat lane's worth of undo, held for a few milliseconds.
//
// The open hat is a two-pad chord and two fingers do not land in the same scan.
// The first of them is indistinguishable from a closed-hat tap - it IS one, as
// far as anything downstream can tell - so the wipe that belongs to OH lands on
// CH a few milliseconds early. Nothing later can tell the difference either,
// which is why marking the derived hi-hat word [4K] did not fix it: by then the
// lane was already gone.
//
// Rather than delay every hi-hat wipe waiting to find out what the hand meant,
// take the copy and give it back if the chord completes. Twenty-five bytes to
// make a destructive guess reversible, which is the right trade every time.
#define CYC_HAT_GRACE 60      // ms - two fingers of one hand, generously
unsigned int  cycHatInst  = 0;
unsigned int  cycHatFlam  = 0;
byte          cycHatVel[NBR_STEP];
unsigned long cycHatWhen  = 0;
boolean       cycHatSaved = FALSE;

unsigned int inChordMask    = 0;
byte         inChordPeak    = 0;   // most simultaneously down during the chord
boolean      inChordDone    = FALSE;

// 16-bit millis. Unsigned subtraction makes the 65.5 s wrap harmless at a
// 1 s hold threshold, and halves the table against unsigned long.
unsigned int inStepTime[NBR_STEP_BTN];

static inline boolean StepDown(byte a)     { return bitRead(inStepNow, a); }
static inline boolean StepPressed(byte a)  { return bitRead(inStepPressed, a); }
static inline boolean StepReleased(byte a) { return bitRead(inStepReleased, a); }
static inline boolean StepHeld(byte a)     { return bitRead(inStepHeld, a); }


//Led-----------------------------------------------
unsigned int stepLeds;
unsigned int stepLedsHigh;
unsigned int stepLedsLow;
byte menuLed;
unsigned int configLed;
volatile boolean blinkTempo = 1;
volatile boolean blinkFast = 1;
volatile boolean blinkVeryFast;
boolean stopLed;
boolean trackLed;
boolean backLed;
boolean fwdLed;
boolean numLed;
boolean ptrnLed;
boolean tapLed;
boolean dirLed;
boolean guideLed;
boolean bankLed;
boolean muteLed;
boolean tempoLed;
boolean instLed;
boolean shiftLed;
boolean clearLed;
boolean shufLed;
boolean scaleLed;
boolean lastStepLed;
/*boolean eighttLed;//tempo scale Leds
 boolean sixteentLed;
 boolean sixteenLed;
 boolean threetwoLed;*/
byte scaleLeds;
boolean enterLed;
unsigned int instSlctLed;//[NBR_INST]={0x00, 0x00, 0x300, 0x400, 0x800, 
byte ledUpdateCounter = 3;                                                                                               // [zabox] [1.028]
byte flagLedIntensity;
unsigned int muteLedsOrder[NBR_STEP_BTN]=  { 
  0x03, 0x03, 0x0C, 0x0C, 0x30, 0x30, 0xC0, 0xC0, 0x300, 0x300, 0x400, 0X800, 0x1000, 0x2000, 0x4000, 0x8000};           // [zabox] update for OH/CH mute
/*unsigned int muteLedsTrig[NBR_STEP_BTN]=  { 
 0, 0, 0x30, 0x400, 0x800, 0x3000, 0x4000, 0x8000, 0x03, 0x0C, 0x30, 0XC0, 0, 0, 0, 0};*/

//Sequencer-----------------------------------------
struct SeqConfig {
  boolean ptrnChangeSync;// 0 = free pattern change  1 = sync end measure pattern change
  byte sync;//0 = MASTER  1 = SLAVE   2 = EXPANDER                                                                       // [zabox] [1.028]
  boolean syncChanged;
  byte TXchannel;//MIDI transmit channel
  byte RXchannel;
  unsigned int bpm;
  unsigned int defaultBpm;// stored in the eeprom
  byte dir;
  byte configPage;
  boolean configMode;
  boolean setupNeedSaved;
  boolean muteModeHH;                 // [zabox]  HH mute mode
  boolean hhPriority;                 // [3B] which hi-hat lane wins a shared step
  byte    pasteMask;                  // [F29] which field groups a paste transfers
  boolean singleTrig;                 // [F7] step edit is on/off, no weak/strong
  byte    bpmFine;                    // [F30] tenths of a BPM, 0..9
  boolean tapOverMute;                // a tapped voice sounds even when muted
  byte    ohSelect;                   // [4W] how a page is told you mean the open hat
  // [6Z] Weak and strong on the two cymbals. Off by default: the stock machine
  // has never had it, and a setting that changed how an existing pattern sounds
  // the moment you flashed new firmware would be the wrong kind of surprise.
  boolean cymAccent;
}
seq;


// [3B] The hat is one physical voice - HH fires it, HH_SLCT picks the decay -
// so at a step where both lanes carry a trig only one of them can sound.
// v1 enforced that by *deleting* the loser's bit from the pattern, which is
// why editing one hi-hat lane destroyed work in the other. Now both bits
// survive and this decides which one is heard, so flipping the setting brings
// the other lane straight back.
// [F29] What a paste actually transfers. The mask is applied at paste time,
// not at copy time, so one copy can be pasted as several different subsets -
// take a pattern's feel without its notes, or its notes without its name.
#define PASTE_TRIG  0x01
#define PASTE_VEL   0x02
#define PASTE_FLAM  0x04
#define PASTE_SETUP 0x08
#define PASTE_NAME  0x10
#define PASTE_EXT   0x20
#define PASTE_EUCL  0x40
#define PASTE_COND  0x80
// Everything but the name by default: pasting content into a pattern should
// not rename it unless that was asked for.
#define PASTE_DEFAULT ((byte)(0xFF & ~PASTE_NAME))

#define HH_PRIO_OH 0
#define HH_PRIO_CH 1

// [B-32] Flam exists only on the five voices that have a low/high button pair
// whose double-push arms it: BD, SD, LT, MT, HT. Those are exactly the first
// multiplexer's instruments, which is why every flam path is first-mux-only -
// by design, not by oversight. Bit 7 of velocity is the flam flag, so on any
// other voice it is meaningless and must never be written or acted on.
static inline boolean InstHasFlam(byte inst)
{
  return (inst == BD || inst == SD || inst == LT || inst == MT || inst == HT);
}

#define SLAVE_TICK_MAX 8
volatile byte          slaveTicks = 0;
volatile unsigned long slaveLastClock = 0;

// [3H] The tempo of the clock we are following, in tenths of a BPM, and when a
// clock byte last arrived. In slave mode the panel had nothing to say about
// tempo at all - it printed "ext" - so the one number you most want when
// checking a sync connection was the one number the machine would not tell you,
// even though it is measuring it accurately enough to run the sequencer off it.
//
// 0 means "not established yet". Stale means the clock stopped: it is worth
// distinguishing, because a frozen reading and a live one look identical.
#define SLAVE_BPM_STALE 1000              // ms without a clock before it is stale
unsigned int  slaveBpm10   = 0;
unsigned long slaveClockMs = 0;

volatile byte ppqn = 0;
volatile byte curStep = 0;
volatile int stepCount = -1;
volatile byte tapStepCount;//this counter is used to get a better tap response
volatile boolean stepChanged = FALSE;
volatile byte noteIndexCpt = 0;
// [5R] The note actually sounding, rather than one worked out backwards from the
// index afterwards. The old form derived it as extNote[noteIndexCpt - 1], with a
// special case for the wrap - so the note that went off was whatever that
// arithmetic produced against the stack as it stood at that later moment, not the
// note that had gone on. Any edit, wrap or skipped send in between and the pair
// came apart, leaving one note hanging and taking the next one off instead.
volatile byte midiNoteOnValue = 0;
// [5T] The same pairing for the keyboard page's own auditioning.
byte    keybNoteSounding = 0;
boolean keybNoteActive   = FALSE;


boolean isRunning = FALSE;
boolean isStop = TRUE;
SeqMode curSeqMode = PTRN_STEP;
SeqMode prevSeqMode;
byte curInst = BD;//8 is BD trig out shift register
byte curFlam = 0;                                                      // [zabox] [1.028] flam
byte curBank = 0;//0 to 7 banks
byte curPattern = 0;// 0 to 255
byte nextPattern;
volatile unsigned int stepValue = 0;
volatile unsigned int stepValueFlam = 0;                               // [zabox] [1.028] flam
//byte seqDirMode;
boolean changeDir; //use to PING PONG change dir

volatile byte dirRepeatBase  = 0;
volatile boolean endMeasure;
/*byte seqDir[MAX_SEQ_DIR][NBR_STEP]={//To do
 {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}
 {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0}
 {0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8, 7, 9, 8, 10, 9, 11, 10, 12, 11, 13, 12, 14, 13, 15, 14}*/
unsigned int metronome;//value of metronome trig
int shuffle[MAX_SHUF_TYPE][2]={
  {
    0            }
  ,{
    0,-1          }
  ,{
    0,-2          }
  ,{
    0,-3          }
  ,{
    0,-4          }
  ,{
    0,-5          }
  ,{
    0,-6          }
};
volatile boolean shufPolarity;

// [F19] Per-instrument swing. The step still starts on the grid; each
// instrument's trigger is held back by its own number of ticks, so a swung hat
// can sit over a straight kick. pattern.instSwing[z] of 0 means "follow the
// pattern's shuffle", so a pattern that sets none behaves exactly as it always
// did - and the whole-pattern shuffle is now simply the default every
// instrument inherits rather than an offset on the boundary itself.
//
// Seven slots because the shuffle table's deepest offset is six ticks. The word
// for each slot is captured at the step boundary and drained over the ticks that
// follow. Delay is clamped to scale/2 - 1, which keeps the last group clear of
// the half-step precompute - a group draining after that would fire from caps
// already charged for the next step. That clamp also retires a stock bug: at
// 1/32 the deepest shuffle put the swung step on the precompute tick exactly.
#define SWING_SLOTS 7
unsigned int swingWord[SWING_SLOTS];
byte         swingPos = SWING_SLOTS;   // >= SWING_SLOTS: nothing left to drain

// [B-07] Precomputed step event -------------------------------------------
// The stock ISR ran SetMux() - ~35 us of DAC write + S/H settling per sounding
// instrument - BEFORE latching the trigger word, so a kick-only step fired
// ~35 us after the tick and a ten-instrument step ~350 us after it. That is
// deterministic jitter correlated with pattern density: the "loose" feel.
//
// Now the whole slow half is done half a step EARLY, at the ppqn midpoint of
// the preceding step, into these variables. The trigger path itself becomes a
// mask and one SPI write - constant time regardless of how many instruments
// sound. Half a step (~60 ms at 120 BPM 1/16) also gives the S/H caps the same
// settle-and-decay profile they had before, so nothing changes acoustically.
volatile unsigned int evtStepValue = 0;   // instrument word for the step about to fire
volatile unsigned int evtFlamWord  = 0;   // instruments whose velocity carries the flam bit
volatile byte    nextStepIndex = 0;       // resolved index of the step about to fire
volatile boolean evtChOn    = FALSE;      // inst[CH] set on that step
volatile boolean evtOhOn    = FALSE;      // inst[OH] set on that step
volatile boolean evtTrigOut = FALSE;
volatile boolean evtExtInst = FALSE;
volatile boolean muxArmedForNext = FALSE; // S/H caps hold nextStepIndex's velocities
volatile byte    flamStep = 0;            // step the pending flam hit belongs to
// [5A] The pattern the armed swap will start playing, recorded when it is armed.
//
// patternSwapped used to be finalised as `curPattern = nextPattern`, which asks
// a moving variable what happened in the past. Both flags are raised by the ISR
// on the same bar line, and the loop reads them at opposite ends of
// SeqParameter() - so an interrupt landing between the two made the chain
// advance nextPattern before the swap had been finalised, and curPattern then
// took the pattern due NEXT as the one already playing.
//
// The pattern that was armed is a fact at arm time. Recording it there costs a
// byte and removes the question.
//
// Two bytes rather than one, because recording it at arm time is not enough on
// its own: if the bar line falls mid-pass, the chain advances and re-arms before
// the loop has finalised the swap that already happened, so a single byte would
// be overwritten by the next arming and curPattern would skip a pattern anyway.
// The swap itself captures the value - `swappedPattern` is written only where
// `patternSwapped` is raised, so what the loop reads is what actually started
// playing, whatever has been armed since.
// [5E] What is playing RIGHT NOW, including a swap the ISR raised after the top
// of this pass had already finalised curPattern.
//
// curPattern is settled once per pass, at the top of SeqParameter(), so that
// everything below reads one truth [3F]. An interrupt landing after that point
// leaves curPattern naming the pattern before the one now sounding, for the rest
// of the pass - and the bar-end commit, which lives at the bottom, then asks
// `is the queued pattern different from what is playing` and gets the answer for
// last bar. When they happen to match it queues nothing, and a bar line with
// nothing queued is a bar of whatever is already playing.
//
// [5A] fixed the same stale read where curPattern is assigned. This is the other
// side of it: every consumer that runs below the top of the pass has to ask this
// rather than read curPattern directly.

byte armedPattern = 0;                  // loop: what the pending arm will start
volatile byte swappedPattern = 0;       // swap: what it did start
volatile boolean patternSwapped = FALSE;  // [B-05] ISR flipped the buffer; loop must finalise

static inline byte PlayingPattern()
{
  return patternSwapped ? swappedPattern : curPattern;
}

volatile boolean seqNeedRephase = FALSE;  // [B-34] scale or length moved; re-align the counters
int flam[MAX_FLAM_TYPE] =                                               // [zabox] [1.028] flam
{ 4999, 5999, 6999, 7999, 8999, 9999, 10999, 11999 };                // flam timings from EchoBoy (in ms: 20, 24, 28, 32, 36, 40, 44, 48)

//Pattern-------------------------------------------
// [3B] Pattern record v2 - 896 bytes, 14 pages of 64.
//
// The struct IS the record. Members are declared in exactly the order and at
// exactly the offsets the EEPROM holds, so a load or save is a page copy rather
// than a field-by-field serialiser - which is both faster and impossible to get
// out of step with itself. AVR has no alignment requirement, so the compiler
// inserts no padding, and it is little-endian, which is the byte order v1 wrote
// its 16-bit words in anyway. The static_assert below is what keeps this
// honest: change a field and the build fails rather than the format drifting.
//
// Fields marked "reserved" are stored and zeroed but nothing reads them yet -
// they belong to features in later stages. The point of writing them now is
// that the format does not have to move again when those arrive.
#define PTRN_MAGIC0   'N'
#define PTRN_MAGIC1   'V'
#define PTRN_FORMAT   2
#define NAME_LEN      11        // [F26] 11 shown + 1 spare, in a 12-byte field
#define NAME_CHARSET_LEN 42

typedef struct Pattern Pattern;
struct Pattern
{
  //---- header, 0..15
  byte magic0;                          //   0
  byte magic1;                          //   1
  byte formatVer;                       //   2
  byte flags;                           //   3
  char name[12];                        //   4  11 used, space padded

  //---- setup, 16..31
  byte length;//0 to 15 steps           //  16
  byte scale;                           //  17
  byte dir;//0=>fwd 1=>bwd 2=>pp 3=>rnd //  18
  byte dirMode;                         //  19  reserved, F17
  byte shuffle;                         //  20
  byte flam;                            //  21
  byte extLength;                       //  22
  byte groupPos;                        //  23
  byte groupLength;                     //  24
  byte totalAcc;                        //  25
  byte fillLength;                      //  26  reserved, F18 FILL condition
  byte setupSpare0;                     //  27  reserved
  byte setupSpare[4];                   //  28

  //---- the pattern itself
  unsigned int inst[NBR_INST];              //  32  trig bitmask
  unsigned int flamMask[NBR_INST];          //  64  retires velocity bit 7
  byte velocity[NBR_INST][NBR_STEP];        //  96  clean 0..127 now
  byte stepCond[NBR_INST][NBR_STEP];        // 352  reserved, F18
  byte instCondProb[NBR_INST];              // 608  reserved, F18
  byte instSwing[NBR_INST];                 // 624  reserved, F19
  byte instLink[NBR_INST];                  // 640  reserved, F21/F22
  byte lane[NBR_INST][4];                   // 656  per-lane loop: span, -, -, padding
  unsigned int choke[4];                    // 720  reserved, F22
  byte reserved[40];                        // 728  F24
  byte extNote[128];                        // 768  DUE to EEPROM 64 bytes PAGE WRITE
};                                          // 896
// The format only stays a format if a stray field breaks the build rather
// than the bank. These are the guard on "the struct is the record".
static_assert(sizeof(Pattern) == 896, "Pattern must be exactly the 896-byte v2 record");

Pattern pattern[2];//current pattern and next pattern in the buffer
Pattern bufferedPattern;//to copy paste pattern
// [4T] A never-copied buffer is all zeroes, and a group paste of it would write
// that over a range of real patterns with no way back. A single paste has the
// same hole but only costs you the pattern in front of you; across a range it is
// the sort of mistake people stop using a feature over.
boolean bufferHasPattern = FALSE;

// [4T] "Is this record one we recognise", named. LoadPattern() spelled the same
// three comparisons out inline; the group paste needs the same question asked of
// a record it has just read, and two spellings of it would be one too many.
static inline boolean PatternRecordValid(boolean buf)
{
  return pattern[buf].magic0    == PTRN_MAGIC0
      && pattern[buf].magic1    == PTRN_MAGIC1
      && pattern[buf].formatVer == PTRN_FORMAT;
}

byte    bufferSource     = 0;      // which pattern it came from, for the screen

//Undo [F4]-----------------------------------------
// A snapshot of everything an edit can reach. See Undo.inc for why this is a
// snapshot rather than the smaller variable-size record the plan proposed.
typedef struct UndoSnap UndoSnap;
struct UndoSnap
{
  byte length;
  byte scale;
  byte dir;
  byte shuffle;
  byte flam;
  byte extLength;
  byte totalAcc;
  byte spare;
  unsigned int inst[NBR_INST];
  unsigned int flamMask[NBR_INST];
  byte velocity[NBR_INST][NBR_STEP];
  byte lane[NBR_INST][4];
  byte instCondProb[NBR_INST];
  byte stepCond[NBR_INST][NBR_STEP];
};                                        // 664 bytes
#define UNDO_LEVELS 4      // [3D] here, not Undo.inc: LCD.inc is included first
UndoSnap undoStack[UNDO_LEVELS];
#define TAP_UNDO_GAP 600      // [F4] ms of quiet that starts a new tap undo group
unsigned long tapUndoTime = 0;
byte undoDepth = 0;   // levels currently held
byte undoTop   = 0;   // next slot to write, wraps
// [6S] Levels available to redo. undoDepth + redoDepth never exceeds
// UNDO_LEVELS: the two share one ring, and undoTop is the cursor between them.
byte redoDepth = 0;
// [6U] One undo level per LASTSTEP+encoder hold rather than per detent.
boolean lastStepEncDirty = FALSE;

boolean ptrnBuffer = 0;

byte    lanePhase[NBR_INST];
boolean lanePhaseFresh = TRUE;   // first step after PLAY uses phase 0, not 1
// [6A] Whether the phases have already been advanced for the step now pending.
// Set by PrepareNextStep(), cleared by FireStep(), so its life is exactly one
// step. See the note in PrepareNextStep() for what breaks without it.
boolean lanePhaseStepDone = FALSE;

// The lane's own loop, or 0 when it simply follows the pattern.
static inline byte LaneLoop(byte z)
{
  const byte span = pattern[ptrnBuffer].lane[z][0];
  return span ? (byte)(span + pattern[ptrnBuffer].lane[z][3]) : 0;
}

// TRUE when a lane with its own length is inside its padding - past the span
// that holds its trigs, and therefore silent until it wraps.
static inline boolean LaneSilent(byte z)
{
  const byte span = pattern[ptrnBuffer].lane[z][0];
  return (span && lanePhase[z] >= span);
}

// Which step of this instrument to read for a given master step.
static inline byte LaneIndex(byte z, byte s)
{
  const byte span = pattern[ptrnBuffer].lane[z][0];
  if (!span) return s;
  byte p = lanePhase[z];
  if (p >= NBR_STEP) p = 0;             // callers gate on LaneSilent(); belt and braces

  return p;
}

// [3R] Where a tap or a roll for this instrument is written.
//
// A lane with its own length plays from its own phase, so a note played into it
// has to be recorded there too. Writing at the master step and reading at the
// lane index is what put taps where nothing would ever read them - and what made
// the lane stretch to reach them, which is a polyrhythm quietly turning back
// into a bar-length loop while you play into it.
//
// Inside its padding a lane has no step to be at, and the phase is past the span.
// Clamp to the last step it owns rather than dropping the note: the tap happened,
// and the end of the loop is the most recent position it could have belonged to.
static inline byte TapIndex(byte z)
{
  const byte span = pattern[ptrnBuffer].lane[z][0];
  if (!span) return tapStepCount;
  const byte p = lanePhase[z];
  if (p < span)      return p;
  return (byte)(span - 1);
}

// [3B] Trig and flam are two bitmasks describing the same step, so anything
// that removes or creates a trig has to say what happens to its flam bit.
// Routing every such edit through these is what stops the two drifting apart -
// which is exactly what went wrong when flam moved out of velocity bit 7 and
// six separate places carried on clearing only the trig.
static inline void StepClear(byte inst, byte stp)
{
  bitClear(pattern[ptrnBuffer].inst[inst], stp);
  bitClear(pattern[ptrnBuffer].flamMask[inst], stp);
}

// [5V] An empty stack had no way to say so.
//
// extLength is the INDEX of the last note, so 0 means "one note in slot 0" - and
// an emptied stack is also extLength 0 with slot 0 holding nothing. The two states
// are written identically, so an empty stack played its slot 0 as a note, and slot
// 0 holds zero, and MidiSendNoteOn() adds the octave offset: note 12, over and
// over, which is the first message in every dump of this fault.
//
// The same ambiguity put the append cursor one slot too far after a wipe, so the
// first note typed landed in slot 1 and left slot 0 as that phantom note 12 in
// front of the part. Reading zero in slot 0 as "nothing here" resolves both, and
// costs no change to the stored format - which matters, because these records have
// to stay readable by stock firmware.
static inline boolean ExtStackEmpty()
{
  return (pattern[ptrnBuffer].extLength == 0 && pattern[ptrnBuffer].extNote[0] == 0);
}

static inline void InstClear(byte inst)
{
  pattern[ptrnBuffer].inst[inst] = 0;
  pattern[ptrnBuffer].flamMask[inst] = 0;
}

// Add trigs that are explicitly not flams - a tapped step is a plain step, and
// must not inherit a flam left behind by whatever occupied that position before.
static inline void StepAddPlain(byte inst, unsigned int mask)
{
  pattern[ptrnBuffer].inst[inst]     |= mask;
  pattern[ptrnBuffer].flamMask[inst] &= ~mask;
}
boolean patternWasEdited = FALSE;
boolean selectedPatternChanged = FALSE;
boolean nextPatternReady = FALSE;
boolean patternNeedSaved = FALSE;
byte prevShuf;                                                          // [zabox] [1.028] flam
byte prevFlam;                                                          // [zabox] [1.028] flam

//volatile boolean patternNeedSwitchBuffer = FALSE;

struct GroupPattern
{
  byte length;//length of the pattern bloc
  byte firstPattern;//first pattern of the bloc
  byte pos;
}
group;

unsigned int tempInst[NBR_INST]={
  0};
// [B-02] muteInst = latched mutes (MUTE mode). tempMuteInst = transient mutes
// held only while CLEAR is down. Consumers must use ActiveMute(), never
// muteInst directly. prev_muteInst is gone: snapshot/restore was silently
// discarding mute changes made while CLEAR was held.
// volatile: both are written in the main loop and read by three ISRs. This is
// still a non-atomic 16-bit access — a proper critical section arrives with
// B-11 in Phase 2. volatile at least stops the compiler caching them.
// [F16] Solo. Non-zero means "only these sound"; ActiveMute() inverts it.
volatile unsigned int soloInst = 0;

// [F13] Instruments selected for a grouped mute change, applied together.
unsigned int mutePending = 0;

// [F9] Tap record arm. Default on, which is the behaviour that existed before
// there was a switch.
boolean tapRecordArm = TRUE;

// [F12] Tap tempo. 240 ms is 250 BPM and 2000 ms is 30 BPM, so anything
// outside that is not someone keeping time - it is a first tap, or a stray one.
// [4Q] 240 ms was a 250 BPM ceiling on the input, and a hard one: a gap under
// it is discarded silently, so tapping faster than the limit does not clamp -
// it does nothing at all, which reads as a tap tempo that has stopped working.
// 150 ms puts the input ceiling at 400, well clear of MAX_BPM, so the clamp on
// the result is the only limit and it is one the panel can show you.
#define TAP_TEMPO_MIN 150
#define TAP_TEMPO_MAX 2000
// A gap this far from the running average is a new tempo rather than a sloppy
// tap. Averaging the two drags the answer between them and can take a dozen
// taps to arrive - which is the other half of "it will not go faster".
// Expressed as eighths so the test is a shift, not a divide.
#define TAP_TEMPO_JUMP 3      // 3/8 of the average, either side
unsigned long tapTempoLast = 0;
unsigned long tapTempoSum = 0;
byte          tapTempoCount = 0;

volatile unsigned int muteInst = 0;
volatile unsigned int tempMuteInst = 0;
// [F16] Solo overrides the mutes rather than merging with them, so leaving solo
// restores exactly the mute state you had before entering it - no bookkeeping,
// and nothing to get out of step.
static inline unsigned int ActiveMute()
{
  if (soloInst){
    unsigned int keep = soloInst;
    // The hat is one voice reached through HH. Soloing CH or OH has to keep HH
    // audible or the soloed lane cannot sound at all; soloing HH keeps both
    // lanes, since either may be what fires it.
    if (keep & ((1U << CH) | (1U << OH))) keep |= (1U << HH);
    if (keep & (1U << HH))                keep |= (1U << CH) | (1U << OH);
    return (unsigned int)(~keep);
  }
  return muteInst | tempMuteInst;
}
byte muteOut[NBR_STEP_BTN]=  { 
  BD, BD, SD, SD, LT, LT, MT, MT, HT, HT, RM, HC, CH, OH, CRASH, RIDE};                                        // [zabox] added CH/OH mute
volatile unsigned int tempDoutTrig;// used to know what OH or CH is trigged
volatile unsigned int lastDoutTrig;// used to know what OH or CH is trigged                                    // [zabox] [1.028] test
// [6G] Every trigger bit asserted since SetLeds() last looked. Accumulated in
// SetDoutTrig() and emptied by the monitor, so a 2 ms pulse cannot fall between
// two LED passes. See Dio.inc.
volatile unsigned int trigMonitor = 0;
// SetLeds() passes a sample is held for. At roughly 80 Hz that is about 50 ms.
#define TRIG_MON_HOLD 4
byte lastHHtrigged;// remmeber last OH or CH was trigged to prevent OH noise when trigget other instruments
//Velocity table match with original TR909
//0, 0, HT, RM, HC, HH, RIDE, CRASH, BD, SD, LT, MT, TA, EXT, CH, OH
// Indexed by instrument number, so the order is TRIG_OUT, HH_SLCT, HT, RM, HC,
// HH, RIDE, CRASH, BD, SD, LT, MT, TOTAL_ACC, EXT_INST, CH, OH.
//
// [6Z] RIDE and CRASH had their low value one count under their high, which is
// the same as not having one - and it is why they were two-state in step edit.
// They do have a velocity CV: both sit in muxInst[], so the DAC and the
// sample-and-hold reach them exactly as they reach the drums. Only the pair of
// numbers was missing.
//
// The weak values are about 78 % of strong, which is where CH sits (80 against
// 111) - the cymbal family wants a smaller step than the drums, whose 25/50 is a
// halving. **These two are the tuning point**: if the accent comes out too broad
// or too subtle on a given unit, this line is the only thing to change.
byte instVelHigh[NBR_INST]={
  1, 1, 50, 50, 50, 108, 112, 107, 50, 50, 50, 50, 1, 1, 111, 109};
byte instVelLow[NBR_INST]={
  0, 0, 25, 25, 25, 50,  88,  84, 25, 25, 25, 25, 0, 0,  80, 108};

// [4D] The first tap of a cycle on this voice throws away what it played last
// cycle, so playing over a part replaces it rather than piling onto it. Later
// taps in the same cycle add, which is what makes it one take rather than one
// note. Stop playing and what you last recorded stays: nothing clears a lane
// except a tap that is about to write to it - so the take you stopped on is
// the take you keep, with no commit gesture to remember.
//
// Defined here rather than up with InstClear() because it reads two globals
// this file has not reached yet, and one function moved is better than two
// forward declarations in a header whose whole job is to define things once.
static inline void CycRecTouch(byte z)
{
  if (!cycRec || !tapRecordArm || !isRunning) return;

  // [4L] The open hat arriving just behind a closed hat is the chord completing,
  // not a second voice: give the closed hat back the lane the leading pad took
  // off it. cycTouched is cleared with it, because CH has not in fact been taken
  // over this cycle and a real closed-hat tap later in the bar must still get to
  // replace it.
  if (z == OH && cycHatSaved && (millis() - cycHatWhen) < CYC_HAT_GRACE){
    pattern[ptrnBuffer].inst[CH]     = cycHatInst;
    pattern[ptrnBuffer].flamMask[CH] = cycHatFlam;
    for (byte s = 0; s < NBR_STEP; s++) pattern[ptrnBuffer].velocity[CH][s] = cycHatVel[s];
    bitClear(cycTouched, CH);
    cycHatSaved = FALSE;
  }

  if (bitRead(cycTouched, z)) return;
  bitSet(cycTouched, z);

  // Copy before the wipe, not after - this is the only moment the lane still
  // exists. Only the closed hat needs it; nothing arrives behind an open hat
  // that could turn out to have meant something else.
  if (z == CH){
    cycHatInst  = pattern[ptrnBuffer].inst[CH];
    cycHatFlam  = pattern[ptrnBuffer].flamMask[CH];
    for (byte s = 0; s < NBR_STEP; s++) cycHatVel[s] = pattern[ptrnBuffer].velocity[CH][s];
    cycHatWhen  = millis();
    cycHatSaved = TRUE;
  }

  InstClear(z);
  for (byte s = 0; s < NBR_STEP; s++) pattern[ptrnBuffer].velocity[z][s] = instVelLow[z];
  // [4K] inst[HH] is derived - the OR of the two hi-hat lanes - and nothing
  // else recomputes it inside a bar. Left stale, a wipe of one hat leaves the
  // other's steps described by a word that still remembers both, which is how
  // clearing the open hat takes a closed-hat step with it. The lanes themselves
  // are already independent; this is the derived copy catching up.
  if (z == CH || z == OH)
    pattern[ptrnBuffer].inst[HH] = pattern[ptrnBuffer].inst[CH] | pattern[ptrnBuffer].inst[OH];
}

//Track----------------------------------------------
// [3B] Track record v2 - 512 bytes, 8 pages. Same struct-is-the-record rule.
//
// Halved, and the reason is not only space. v1 kept the track length in bytes
// 1022-1023, so *finding out how long a track is* meant reading all sixteen
// pages: that is the entire reason a track load cost 1 KB of I2C. With the
// length in the header a load reads page 0, then only as many more pages as the
// track actually uses - one page for any song under 48 patterns, against
// sixteen before. Capacity falls from 1022 chained patterns to 496, still a
// ~33 minute song at two bars each.
typedef struct Track Track;
struct Track
{
  char name[12];                    //  0  11 used
  unsigned int length;//lenght before loop
  byte flags;                       // 14
  byte formatVer;                   // 15
  byte patternNbr[MAX_PTRN_TRACK];  // 16
};                                  // 512
static_assert(sizeof(Track) == 512, "Track must be exactly the 512-byte v2 record");

Track track[2];
boolean bankFormatBad = FALSE;      // [3B] a record failed its magic check

typedef struct TrackSetup TrackSetup;
struct TrackSetup
{
  byte next;
  byte current;//number of the track 0 to 15
  unsigned int pos;//track position 0 to 999
  volatile byte indexCpt;//incrementer each pattern end
  boolean wasEdited;
};
TrackSetup trk;
boolean selectedTrackChanged = FALSE;
// [3C] A track chosen by hand should land on the bar line like any other pattern
// change. The track's *own* advance must not - it already happens at the bar,
// and queuing it would hold every pattern for a second bar.
boolean trackChangeManual = FALSE;
// [6B] ...but it still happens ON the bar line, and the lanes need telling.
//
// A free pattern change normally lands mid-bar, so LaneSyncAll() folds the
// running phases rather than restarting them - [3X], and it is right, because a
// lane restarted on the downbeat is not a polyrhythm. A track's own advance and a
// chain's own advance are the two free changes that are NOT mid-bar: the bar has
// just restarted underneath them, exactly as it has for the synced swap that
// Clock.inc handles, where the lanes ARE restarted. Both are queued out of
// trackPosNeedIncremante, so that is where this is set.
boolean barLineChange = FALSE;
boolean trkBuffer = 0;
boolean trackNeedSaved = FALSE;
boolean trackJustSaved = FALSE;
volatile boolean trackPosNeedIncremante = FALSE;
unsigned long timeSinceSaved;

//Browser [F26]-------------------------------------
// The list is not cached: browseName holds the one entry under the cursor, read
// from the bank when the cursor moves. See Browse.inc.
boolean browseMode    = FALSE;
boolean browseIsTrack = FALSE;
byte    browseSel     = 0;
char    browseName[NAME_LEN + 1];
unsigned int browseLen = 0;    // [3C] steps in a pattern, or positions in a track

//Naming [F26]--------------------------------------
boolean nameEditMode = FALSE;
byte    nameCursor = 0;
byte    nameLastIdx = 0;    // charset index last chosen, so a blank cell starts near it

//Pattern utilities [F25]---------------------------
#define UTIL_OP_CLR 0
#define UTIL_OP_MOV 1
// [4T] Paste the copy buffer into every pattern of a range, through the same
// mask a single paste uses. Here rather than on a performance chord because it
// is preparation, not playing: "give these thirty-two patterns the same choke
// rules" is something you do once, sitting down, before a set. It inherits the
// range fields, the one-second ENTER hold and the stopped-only rule from the
// two operations already on this page, which is most of what made it worth
// putting here.
#define UTIL_OP_PST 2
#define UTIL_OP_MAX 2
//Trig conditions [F18]-----------------------------
boolean condMode = FALSE;
byte    condPage = 0;         // 0 = conditions, 1 = links [F21]

// [F21] instLink[z]: high nibble is the target as a voiceOrder index (15 = none),
// low nibble is the rule. Selective choke is LINK_NOT - "play only when that
// one did not" - which is the only kind of choke separate analog voices can
// actually do: you cannot cut a note that has already started, so the answer is
// not to start it.
#define LINK_NONE 0
#define LINK_AND  1
#define LINK_NOT  2
#define LINK_PRE  3
#define LINK_NPR  4
#define LINK_MAX  4
#define LINK_NO_TARGET 15
volatile unsigned int condPrevSounded = 0;   // what actually sounded last step
byte    condStep = 0;         // 0 = the instrument as a whole, 1..16 = its steps
volatile unsigned long barCount = 0;          // pattern cycles, for "X of every Y"
// [3X] Steps since the transport started, wrapped at 12 rather than at the bar.
// The slow roll rates fire every `den` steps and counted stepCount, which resets
// at the bar line - so /2 and /4 were invisible on a sixteen-step bar and **/3
// restarted every bar**, which is a triplet roll that is not a triplet.
//
// 12 is divisible by 2, 3 and 4, so every rate divides the wrap and the phase
// never jumps at it - which a free-running counter would do each time it
// overflowed.
volatile byte rollPhase = 0;
volatile unsigned int  evtCondMask = 0xFFFF;  // who may sound on the pending step

boolean instShiftPrev = FALSE;// [F14] SHIFT+INST edge, since instBtn is a level

boolean utilMode  = FALSE;
byte    utilOp    = 0;
byte    utilFirst = 0;      // 0-based; shown 1-based
byte    utilCount = 1;
byte    utilDest  = 0;

static inline void StepInsertLane(byte z, byte at)
{
  const byte loop = LaneLoop(z);
  const byte last = loop ? (byte)(loop - 1) : pattern[ptrnBuffer].length;
  if (at > last) return;
  for (byte s = last; s > at; s--){
    bitWrite(pattern[ptrnBuffer].inst[z],     s, bitRead(pattern[ptrnBuffer].inst[z],     (byte)(s - 1)));
    bitWrite(pattern[ptrnBuffer].flamMask[z], s, bitRead(pattern[ptrnBuffer].flamMask[z], (byte)(s - 1)));
    pattern[ptrnBuffer].velocity[z][s] = pattern[ptrnBuffer].velocity[z][s - 1];
    pattern[ptrnBuffer].stepCond[z][s] = pattern[ptrnBuffer].stepCond[z][s - 1];
  }
  bitClear(pattern[ptrnBuffer].inst[z], at);
  bitClear(pattern[ptrnBuffer].flamMask[z], at);
  pattern[ptrnBuffer].velocity[z][at] = instVelLow[z];
  pattern[ptrnBuffer].stepCond[z][at] = 0;
}

static inline void StepDeleteLane(byte z, byte at)
{
  const byte loop = LaneLoop(z);
  const byte last = loop ? (byte)(loop - 1) : pattern[ptrnBuffer].length;
  if (at > last) return;
  for (byte s = at; s < last; s++){
    bitWrite(pattern[ptrnBuffer].inst[z],     s, bitRead(pattern[ptrnBuffer].inst[z],     (byte)(s + 1)));
    bitWrite(pattern[ptrnBuffer].flamMask[z], s, bitRead(pattern[ptrnBuffer].flamMask[z], (byte)(s + 1)));
    pattern[ptrnBuffer].velocity[z][s] = pattern[ptrnBuffer].velocity[z][s + 1];
    pattern[ptrnBuffer].stepCond[z][s] = pattern[ptrnBuffer].stepCond[z][s + 1];
  }
  bitClear(pattern[ptrnBuffer].inst[z], last);
  bitClear(pattern[ptrnBuffer].flamMask[z], last);
  pattern[ptrnBuffer].velocity[z][last] = instVelLow[z];
  pattern[ptrnBuffer].stepCond[z][last] = 0;
}

static inline boolean CutKeyHeld()
{
  return backBtn.pressed || fwdBtn.pressed || tapBtn.pressed || dirBtn.pressed;
}


// Defined in SeqFunc.inc, which this header is included ahead of - and
// prototypes.h comes after it too, so the declaration has to be here.
unsigned int InstMaskFromButtons(unsigned int btns);

#define OH_SEL_BOTH 0        // both hat pads together
#define OH_SEL_2ND  1        // the second hat pad on its own

static inline boolean InstHasAccent(byte z)
{
  if (z == RIDE || z == CRASH) return seq.cymAccent;
  return TRUE;
}

// [4V][4W] Which voice a press names. `pressed` is the edge, `now` the level -
// the chord form needs the level, because the second pad of a chord has to be
// able to change the answer the first one gave.
static inline byte InstSelectFromButtons(unsigned int pressed, unsigned int now)
{
  if (seq.ohSelect == OH_SEL_BOTH){
    const unsigned int m = InstMaskFromButtons(now);
    for (byte i = 0; i < NBR_INST; i++) if (bitRead(m, i)) return i;
    return 0xFF;
  }
  for (byte a = 0; a < NBR_STEP_BTN; a++)
    if (bitRead(pressed, a)) return muteOut[a];
  return 0xFF;
}

// [5H] The same question asked of a whole chord of pads, which is what the two
// CLEAR gestures take - they act on every voice being held, not on one. Under BTH
// this is InstMaskFromButtons() unchanged; under 2nd the hat pads are simply their
// own lanes, and the leading-pad problem that [3K], [3L], [3M] and [4L] all came
// from cannot arise, because there is no leading pad.
static inline unsigned int InstSelectMask(unsigned int pressed, unsigned int now)
{
  if (seq.ohSelect == OH_SEL_BOTH) return InstMaskFromButtons(now);
  unsigned int m = 0;
  for (byte a = 0; a < NBR_STEP_BTN; a++)
    if (bitRead(pressed, a)) m |= (1U << muteOut[a]);
  return m;
}


static inline unsigned int MuteMaskFromButtons(unsigned int btns, boolean pair)
{
  unsigned int m = 0;
  for (byte a = 0; a < NBR_STEP_BTN; a++){
    if (!bitRead(btns, a)) continue;
    m |= (1U << muteOut[a]);
    if (pair){
      if      (a == 12) m |= (1U << muteOut[13]);
      else if (a == 13) m |= (1U << muteOut[12]);
    }
  }
  return m;
}


// One letter per voice for the map line, in voiceOrder. Distinct within the
// eleven, which is why the hand clap is P and the crash is A - C and O were
// already spoken for by the hi-hats, and case is carrying the state.


// [5G] Copy and paste, held by their second key alone.
//
// Same idea as fillHold below and for the same reason: the two keys of the chord
// are far enough apart to need both hands, and the pads that edit the paste mask
// need one of them back. SHIFT+BANK or SHIFT+MUTE opens it; releasing SHIFT
// leaves BANK or MUTE holding it, and releasing that key ends it.
//
// This changes when a paste fires. It used to go on whichever key came up first
// [3U]; it now waits for MUTE, because releasing SHIFT is no longer the end of
// the gesture - it is the middle of it.
boolean copyHold  = FALSE;
boolean pasteHold = FALSE;


static inline boolean StepModifierHeld()
{
  return BtnHeld(&guideBtn) || CutKeyHeld()
      || (BtnHeld(&clearBtn) && instBtn) || stepLockout
      // [3T] SHIFT+COPY and SHIFT+PASTE held: the pads belong to the paste
      // mask while either is down, so they must not also edit steps.
      // [5G] Through the hold flags now, which also covers the stretch after
      // SHIFT is released - and, because BANK+pad selects a bank from inside two
      // blocks that already sit behind this function, it is the whole of that
      // collision's fix as well.
      || copyHold || pasteHold;
}

// [6N] Every key that can be held while a pad is pressed, for the pattern-select
// gate in PTRN_PLAY. StepModifierHeld() is about the STEP EDITOR - it lists the
// keys that borrow the pads for something else - and it deliberately leaves SHIFT
// and INST out, because in step mode those have their own pad meanings.
//
// Play mode has no pad meanings of its own beyond choosing a pattern, so the
// question there is the opposite one: is ANYTHING held? A chord half-made, or a
// modifier whose other half you never pressed, must not fall through to a pattern
// change - that is the one action in play mode you cannot take back, because the
// pattern you were playing is gone from the buffer by the time you notice.
//
// BANK is excluded: BANK+pad selects a bank and is handled inside the same block.
static inline boolean AnyModifierHeld()
{
  return StepModifierHeld() || shiftBtn || instBtn
      || lastStepBtn.pressed || BtnHeld(&shufBtn) || scaleBtn.pressed
      || numBtn.pressed || tempoBtn.pressed || trkBtn.pressed
      || ptrnBtn.pressed || enterBtn.pressed;
}


static inline boolean PanelBusy()
{
  // [F29][5G] The paste mask uses the step buttons as toggles, so it keeps them
  // away from step editing the same way the other overlays do. It moved from a
  // config page to the copy and paste chords; the reason it is here did not move.
  return nameEditMode || utilMode || condMode ||
         browseMode ||
         (copyHold || pasteHold);                       // [5G] was the paste page
}

//Ext inst-------------------------------------------
boolean keyboardMode;
byte keybOct = DEFAULT_OCT;
byte noteIndex = 0;//external inst note index

//SPI------------------------------------------------
SPISettings SPIset(4000000, MSBFIRST, SPI_MODE0);
SPISettings SPIset_f(4000000, MSBFIRST, SPI_MODE0);                        // [zabox] ready for faster write spi (8mhz), signal integrity looks good and i haven't had any issues, but i'll keep it a 4mhz until the next update and further investigation. 


//Encoder--------------------------------------------
byte encoder_A;
byte encoder_B;
byte encoder_A_prev=0;
// [3E] The detent for this pass: -1, 0 or +1. Read once at the top of EncGet(),
// so EncGet(value, dif) can be asked the same question twice and answer the same
// way both times. See the note on the function.
signed char encStep = 0;
boolean encSwValue[2];
boolean encSwState;

//Mux------------------------------------------------
byte muxAddr[5]={
  B0, B10000000, B1000000, B11000000, B100000};// Due to the  A, B, C pin on muxiPORTA sequence    
byte muxInst[10]={
  LT, SD, BD, MT, HT, HC, RM, CH, CRASH, RIDE};//To match with DAC mux OUT

//LCD------------------------------------------------
boolean needLcdUpdate = TRUE;

// [3D] Transient messages.
//
// The old ones were written straight into the frame from SeqParameter(), which
// runs *before* LcdUpdate() - so any redraw in the same pass painted over them
// before they reached the panel. [UNDO n] was never once visible: undoing
// changes the pattern, which asks for a redraw, which wiped it. [NOTHING]
// survived only because a failed undo changes nothing and nothing asked.
//
// A message is a display *state* now, drawn at the head of the overlay chain and
// owning the screen while it lives. Nothing can overwrite it because nothing
// else is drawn.
#define MSG_NONE   0
#define MSG_SAVED  1
#define MSG_SETUP  2
#define MSG_TRACK  3
#define MSG_REVERT 4
#define MSG_UNDO   5
#define MSG_NOUNDO 6
#define MSG_COPY   7
#define MSG_PASTE  8
#define MSG_CLEARED 9      // [5Y] the external note stack was emptied
#define MSG_REDO   10      // [6S]
#define MSG_NOREDO 11
#define MSG_TIME   1000     // timed messages
#define MSG_TAIL   400      // ...and the tail after a held one is let go
byte          msgKind  = MSG_NONE;
byte          msgArg   = 0;
unsigned long msgSince = 0;

// Copy and paste stay up while their own keys are held - what a paste carries is
// set on a config page and then forgotten, so the answer should be readable for
// as long as you care to look rather than for a second and gone.
static inline void MsgShow(byte kind, byte arg)
{
  msgKind  = kind;
  msgArg   = arg;
  msgSince = millis();
  needLcdUpdate = TRUE;
}
byte curIndex;//position of the cursor
byte curPage = 0;
byte cursorPos[MAX_CUR_POS]={
  0, 4, 8, 12};
const char *letterUpTrackWrite[MAX_CUR_POS]={
  "P", "P", "L", "N"};
const char *letterUpExtInst[MAX_CUR_POS]={
  "I", "N", "L", "O"};
const char *letterUpConfPage1[MAX_CUR_POS]={
  "S", "B", "M", "M"};
const char *letterUpHat[MAX_CUR_POS]={                                // [4X] hhP mte oh
  "H", "M", "O", " "};
const char *letterUpConfPage2[MAX_CUR_POS]={                          // [4X] pCh sTr mut [6Z] cym
  "P", "S", "M", "C"};
const char *letterUpUtil[MAX_CUR_POS]={                               // [F25]
  "O", "F", "T", "D"};                                               // [3E] count became "to"
// [6W] tgt rule src. The source is on the pads - a pad names the voice the rule
// belongs to - so the encoder opens on the one thing the pads cannot reach. The
// rule is the decision and the target the object of it, which is also the order
// the sentence reads backwards from: "...NOT CH" for the lane you have selected.
// [6R] also fixed the marker itself: it was "M" over the word "rule", so
// selecting that field drew "src tgt Mule".
const char *letterUpLink[MAX_CUR_POS]={                               // [F21][6R][6W]
  "T", "R", "S", " "};
// [6R] prb cnd stp ins. Probability is the field this page is opened for; the
// step and the instrument are addresses rather than values, and both are
// reachable on the pads without the cursor. So the two values lead and the two
// addresses follow.
const char *letterUpCond[MAX_CUR_POS]={                               // [F18][6R]
  "P", "C", "S", "I"};

//MIDI-----------------------------------------------
volatile boolean midiNoteOnActive = FALSE;
boolean midiStart;
boolean midiStop;
boolean midiContinue;
boolean instWasMidiTrigged[NBR_INST] ={
  FALSE};
byte midiVelocity[NBR_INST]={
  100};

//Din synchro----------------------------------------
boolean dinStartState = LOW;
boolean dinClkState = LOW;

//SysEx----------------------------------------------
// A restore writes straight to the EEprom, so the copy in RAM goes stale. These
// track the last write so the loop can reload once the transfer goes quiet,
// rather than reloading after each of the ~900 pages a full restore sends.
boolean patternRestored = FALSE;
boolean trackRestored   = FALSE;    // a track came in too, so reload that as well
unsigned long timeSinceRestore = 0;

//Bootloader-----------------------------------------
unsigned long bootElapseTime = 0;
byte btnPlayStopByte = 0;
byte btnEnterByte = 0;

//TrigOut variable
volatile boolean trigCounterStart = FALSE;


//Timer----------------------------------------------                       // [zabox] [1.028] 
#define TRIG_TIMER_STOP  TCCR2B = 0                                         // Stop timer2 (trigger)
#define TRIG_TIMER_START TCCR2B = _BV(CS22) | _BV(CS20)                     // Set prescaler to 128 and start the timer
#define TRIG_TIMER_ZERO  TCNT2  = 0                                         // reset timer

#define FLAM_TIMER_STOP  TCCR3B = 0                                         // Stop timer3 (flam)
#define FLAM_TIMER_START TCCR3B = _BV(WGM32) | _BV(CS31) | _BV(CS30);       // CTC mode & start timer with prescaler 64
#define FLAM_TIMER_ZERO  TCNT3  = 0                                         // reset timer (must be atomic)


//Expader-------------------------------------------                        // [zabox] [1.028]
byte triggerTime [NBR_INST];                                                
byte cur_triggerTime;
#define PULSE 31;                                                           // # timer ticks for 2ms pulse @1024 prescaler
byte ledMap[NBR_INST] = 
{16, 16, 8, 10, 11, 16, 15, 14, 0, 2, 4, 6, 16, 16, 12, 13};
unsigned int lastStepLeds = 0;
byte showTrigLeds = 0;
unsigned int gateInst = 0;


#endif//end if define_h


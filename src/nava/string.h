//-------------------------------------------------
//                  NAVA v1.x
//                  LCD String
// Strings to be stored as Program (not in RAM, but in Flash)
//-------------------------------------------------

// [Phase 0] prog_char was removed from avr-libc 1.8. Converted to the modern
// 'const char X[] PROGMEM' form, and the pointer tables to
// 'const char * const X[] PROGMEM'. pgm_read_word() call sites are unaffected.
#ifndef string_h
#define string_h


const char txt_INST0[] PROGMEM   ="TRG";
const char txt_INST1[] PROGMEM   ="   ";
const char txt_INST2[] PROGMEM   =" HT";
const char txt_INST3[] PROGMEM   ="RIM";
const char txt_INST4[] PROGMEM   ="HCL";
const char txt_INST5[] PROGMEM   ="   ";
const char txt_INST6[] PROGMEM   ="RID";
const char txt_INST7[] PROGMEM   ="CRH";
const char txt_INST8[] PROGMEM   =" BD";
const char txt_INST9[] PROGMEM   =" SD";
const char txt_INST10[] PROGMEM  =" LT";
const char txt_INST11[] PROGMEM  =" MT";
const char txt_INST12[] PROGMEM  ="ACC";
const char txt_INST13[] PROGMEM  ="EXT";
const char txt_INST14[] PROGMEM  =" CH";
const char txt_INST15[] PROGMEM  =" OH";

const char txt_OCT0[] PROGMEM   ="-3";
const char txt_OCT1[] PROGMEM   ="-2";
const char txt_OCT2[] PROGMEM   ="-1";
const char txt_OCT3[] PROGMEM   ="+0";
const char txt_OCT4[] PROGMEM   ="+1";
const char txt_OCT5[] PROGMEM   ="+2";
const char txt_OCT6[] PROGMEM   ="+3";
const char txt_OCT7[] PROGMEM   ="+4";

const char txt_NOTE0[] PROGMEM   =" C";
const char txt_NOTE1[] PROGMEM   ="C#";
const char txt_NOTE2[] PROGMEM   =" D";
const char txt_NOTE3[] PROGMEM   ="D#";
const char txt_NOTE4[] PROGMEM   =" E";
const char txt_NOTE5[] PROGMEM   =" F";
const char txt_NOTE6[] PROGMEM   ="F#";
const char txt_NOTE7[] PROGMEM   =" G";
const char txt_NOTE8[] PROGMEM   ="G#";
const char txt_NOTE9[] PROGMEM   =" A";
const char txt_NOTE10[] PROGMEM   ="A#";
const char txt_NOTE11[] PROGMEM   =" B";

const char txt_SYNC0[] PROGMEM   ="MST";
const char txt_SYNC1[] PROGMEM   ="SLV";
const char txt_SYNC2[] PROGMEM   ="EXP";                                        // [zabox] [1.028] 

const char txt_PTRNSYNCCHANGE[] PROGMEM   ="SYN";
const char txt_PTRNFREECHANGE[] PROGMEM   ="FRE";

const char txt_MUTE0[] PROGMEM   ="C/O";                                        // [zabox] HH mute mode
const char txt_MUTE1[] PROGMEM   ="HH";                                         //
const char txt_HHPRIO0[] PROGMEM ="OH";                                         // [3B] open hat wins a shared step
const char txt_HHPRIO1[] PROGMEM ="CH";                                         //     closed hat wins

//synchro name----------------------------------------------------
const char * const nameSync[] PROGMEM ={
  txt_SYNC0, txt_SYNC1, txt_SYNC2};                                            // [zabox] [1.028]
  
  //Pattern change sync name--------------------------------------
const char * const namePtrnChange[] PROGMEM ={
  txt_PTRNFREECHANGE, txt_PTRNSYNCCHANGE};
  
//Mute Mode-------------------------------------------------------            // [zabox] HH mute mode
const char * const nameMute[] PROGMEM ={
  txt_MUTE0, txt_MUTE1};

//Hi-hat lane priority--------------------------------------------            // [3B]
const char * const nameHHprio[] PROGMEM ={
  txt_HHPRIO0, txt_HHPRIO1};

//Trig conditions--------------------------------------------------           // [F18]
const char txt_CND0[]  PROGMEM = "---";
const char txt_CND1[]  PROGMEM = "1:2";
const char txt_CND2[]  PROGMEM = "2:2";
const char txt_CND3[]  PROGMEM = "1:3";
const char txt_CND4[]  PROGMEM = "2:3";
const char txt_CND5[]  PROGMEM = "3:3";
const char txt_CND6[]  PROGMEM = "1:4";
const char txt_CND7[]  PROGMEM = "2:4";
const char txt_CND8[]  PROGMEM = "3:4";
const char txt_CND9[]  PROGMEM = "4:4";
const char txt_CND10[] PROGMEM = "1:8";
const char txt_CND11[] PROGMEM = "5:8";
const char * const nameCond[] PROGMEM ={
  txt_CND0, txt_CND1, txt_CND2,  txt_CND3, txt_CND4,  txt_CND5,
  txt_CND6, txt_CND7, txt_CND8,  txt_CND9, txt_CND10, txt_CND11};
  
//instrument name-------------------------------------------------
const char * const selectInstString[] PROGMEM ={
  txt_INST0,txt_INST1,txt_INST2,txt_INST3,
  txt_INST4,txt_INST5,txt_INST6,txt_INST7,
  txt_INST8,txt_INST9,txt_INST10,txt_INST11,
  txt_INST12,txt_INST13,txt_INST14,txt_INST15};

//Octaves names------------------------------------------------------
const char * const nameOct[] PROGMEM = { 
 txt_OCT0,txt_OCT1,txt_OCT2,txt_OCT3,txt_OCT4,txt_OCT5,txt_OCT6,txt_OCT7 };

//Notes names----------------------------------------------------
const char * const nameNote[] PROGMEM = { 
  txt_NOTE0,txt_NOTE1,txt_NOTE2,txt_NOTE3,txt_NOTE4,txt_NOTE5,txt_NOTE6,txt_NOTE7,txt_NOTE8,txt_NOTE9,txt_NOTE10,txt_NOTE11};

//Special character-----------------------------------------------
// [3E] The five small scale glyphs that used to hold CGRAM 0-4 went with
// LcdPrintScale(), which had no caller left once 3B moved the scale onto the
// faceplate LEDs.
#define LCD_BAR_LIGHT  0
#define LCD_BAR_FULL   1
#define LCD_GLYPH_NOTE 5

// [3F] Two weights, not four: filled and not filled. A bar answers one question
// - how much of this is set - and a ramp of shades was three answers to it.
//
// These are ours rather than the character ROM's on purpose. The bars were
// built from characters 161 and 219, which are a track and a solid block only
// on some HD44780 ROM variants - on an A00 part 161 is a small corner dot and
// 219 an outlined square, which is exactly why they drew thin.
byte fontBarLight[8] = {
  0x15, 0x00, 0x0A, 0x00, 0x15, 0x00, 0x0A, 0x00};// light shade - not filled
byte fontBarFull[8] = {
  0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F};// solid block - filled


byte font5[8] = {
  0x01, 0x01, 0x01, 0x01, 0x01, 0x0F, 0x1F, 0x0E};//noir tempo

#endif//end if string_h


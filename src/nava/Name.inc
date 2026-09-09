//-------------------------------------------------
//                  NAVA v2.x
//            Pattern / track naming  [F26]
//
// Eleven characters, stored in the record - see struct Pattern and struct Track.
// They cost no extra EEprom reads: the record is already loaded when its pattern
// is selected, so the name arrives with it. A separate name table would be
// faster to enumerate and would duplicate state, and duplicated state is how a
// record and its label drift apart.
//
// Entry is tactile rather than menu-driven, in the spirit of F24: the step
// buttons are the cursor, the encoder is the character. The cursor position
// shows on the step LEDs rather than as a blinking block on the LCD - the panel
// already has sixteen indicators in a row, and using them means the LCD does not
// have to be redrawn several times a second just to blink.
//-------------------------------------------------

// Space first, so index 0 is "blank" and an unrecognised byte reads as one.
const char nameCharset[] PROGMEM = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-./#+";

////////////////////////Function//////////////////////

// Track modes name the track, pattern modes name the pattern. One function so
// every caller agrees about which is being edited.
char *ActiveName()
{
  if (curSeqMode == TRACK_PLAY || curSeqMode == TRACK_WRITE) return track[trkBuffer].name;
  return pattern[ptrnBuffer].name;
}

static void ActiveNameDirty()
{
  if (curSeqMode == TRACK_PLAY || curSeqMode == TRACK_WRITE) trackNeedSaved = TRUE;
  else                                                       patternNeedSaved = TRUE;
}

char NameCharAt(byte i)
{
  if (i >= NAME_CHARSET_LEN) i = 0;
  return (char)pgm_read_byte(&nameCharset[i]);
}

byte NameCharIndex(char c)
{
  for (byte i = 0; i < NAME_CHARSET_LEN; i++){
    if ((char)pgm_read_byte(&nameCharset[i]) == c) return i;
  }
  return 0;                       // anything the charset cannot express is a space
}

void NameEditEnter()
{
  nameEditMode = TRUE;
  nameCursor = 0;
  needLcdUpdate = TRUE;

  // A record straight from InitPatternRecord() is "P001" padded with NULs, and
  // a migrated one likewise. Normalise to spaces so the editor and the display
  // are working on the same thing.
  char *nm = ActiveName();
  for (byte i = 0; i < NAME_LEN; i++){
    const char c = nm[i];
    if (c < ' ' || c > '~') nm[i] = ' ';
  }
}

void NameEditExit()
{
  nameEditMode = FALSE;
  needLcdUpdate = TRUE;
}

// Step buttons place the cursor, BACK/FWD nudge it, CLEAR blanks the field.
// Called from SeqParameter() while nameEditMode is set, before any of the
// normal step handling gets a look at the same buttons.
// Move the cursor on. Deliberately writes nothing: moving is not editing, and
// an earlier version that seeded the new cell from the last character meant a
// run of encoder presses silently typed a row of letters. "Start from the last
// letter" is now a property of the *encoder*, not of the cell - see
// NameEditEncoder(), which begins from nameLastIdx when the cell is blank.
void NameCursorAdvance()
{
  nameCursor = (byte)((nameCursor + 1) % NAME_LEN);
  needLcdUpdate = TRUE;
}

void NameEditButtons()
{
  char *nm = ActiveName();

  if (inStepPressed){
    for (byte a = 0; a < NAME_LEN; a++){
      if (bitRead(inStepPressed, a)){
        nameCursor = a;
        needLcdUpdate = TRUE;
        break;
      }
    }
  }

  if (backBtn.justPressed){
    nameCursor = (byte)((nameCursor + NAME_LEN - 1) % NAME_LEN);
    needLcdUpdate = TRUE;
  }
  if (fwdBtn.justPressed) NameCursorAdvance();

  // Encoder press advances too, so a name can be typed without moving a hand
  // off the encoder: turn to pick a letter, press to move on.
  if (encBtn.justPressed) NameCursorAdvance();

  if (clearBtn.justPressed){
    for (byte i = 0; i < NAME_LEN; i++) nm[i] = ' ';
    nameCursor = 0;
    ActiveNameDirty();
    needLcdUpdate = TRUE;
  }
}

// The encoder edits the character under the cursor. Wraps at both ends, because
// the charset is a ring and hunting for a character you overshot by one should
// not mean turning it forty times.
void NameEditEncoder()
{
  char *nm = ActiveName();

  // A blank cell starts from the last character actually chosen, so typing a
  // run of similar letters is a small turn rather than a hunt from the top of
  // the charset. Nothing is written until the encoder moves.
  const byte idx = (nm[nameCursor] == ' ') ? nameLastIdx : NameCharIndex(nm[nameCursor]);
  int moved = EncGet((int)idx + 1, 1) - 1;    // +1/-1 so EncGet's floor at 0 does not bite

  if (moved == (int)idx) return;
  if (moved < 0) moved = NAME_CHARSET_LEN - 1;
  if (moved >= NAME_CHARSET_LEN) moved = 0;

  nameLastIdx = (byte)moved;
  nm[nameCursor] = NameCharAt((byte)moved);
  ActiveNameDirty();
  needLcdUpdate = TRUE;
}

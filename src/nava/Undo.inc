

////////////////////////Function//////////////////////

static void UndoCapture(UndoSnap *s)
{
  const Pattern *p = &pattern[ptrnBuffer];
  s->length    = p->length;
  s->scale     = p->scale;
  s->dir       = p->dir;
  s->shuffle   = p->shuffle;
  s->flam      = p->flam;
  s->extLength = p->extLength;
  s->totalAcc  = p->totalAcc;

  for (byte i = 0; i < NBR_INST; i++){
    s->inst[i]     = p->inst[i];
    s->flamMask[i] = p->flamMask[i];
  }
  memcpy(s->velocity, p->velocity, sizeof(s->velocity));
  memcpy(s->lane,   p->lane,   sizeof(s->lane));      // [6D]
  memcpy(s->instCondProb, p->instCondProb, sizeof(s->instCondProb));   // [6L]
  memcpy(s->stepCond,     p->stepCond,     sizeof(s->stepCond));
}

// [6S] Exchange the snapshot with the live pattern instead of overwriting it.
//
// This is the whole of redo. Undo walks the cursor back and swaps, which leaves
// the state you just left sitting in the slot you took the old one from; redo
// swaps the same slot again and walks forward. One ring, one cursor, and no
// second stack - a redo stack would be four more 664-byte records for something
// the undo ring already has room for.
//
// Swapped element by element rather than through a temporary UndoSnap: a 664-byte
// local would go on the stack, inside a critical section, on a part with 7 KB of
// stack to its name.
static void UndoSwapBytes(byte *a, byte *b, unsigned int n)
{
  while (n--){ const byte t = *a; *a++ = *b; *b++ = t; }
}

static void UndoRestore(UndoSnap *s)
{
  Pattern *p = &pattern[ptrnBuffer];

  uint8_t sreg = SREG;
  cli();

  UndoSwapBytes(&p->length,    &s->length,    1);
  UndoSwapBytes(&p->scale,     &s->scale,     1);
  UndoSwapBytes(&p->dir,       &s->dir,       1);
  UndoSwapBytes(&p->shuffle,   &s->shuffle,   1);
  UndoSwapBytes(&p->flam,      &s->flam,      1);
  UndoSwapBytes(&p->extLength, &s->extLength, 1);
  UndoSwapBytes(&p->totalAcc,  &s->totalAcc,  1);

  UndoSwapBytes((byte*)p->inst,     (byte*)s->inst,     sizeof(p->inst));
  UndoSwapBytes((byte*)p->flamMask, (byte*)s->flamMask, sizeof(p->flamMask));
  UndoSwapBytes((byte*)p->velocity, (byte*)s->velocity, sizeof(p->velocity));
  // [6D] Inside the critical section with the rest: LaneLoop() and LaneIndex()
  // read these on the trigger edge, so a torn restore is a lane reading a span
  // it does not have.
  UndoSwapBytes((byte*)p->lane, (byte*)s->lane, sizeof(p->lane));
  UndoSwapBytes(p->instCondProb,     s->instCondProb,     sizeof(p->instCondProb));
  UndoSwapBytes((byte*)p->stepCond, (byte*)s->stepCond,  sizeof(p->stepCond));

  SREG = sreg;
}

// Call immediately BEFORE an edit, not after. Anything that changes pattern
// content through a gesture the user could want back belongs here.
void UndoPush()
{
  UndoCapture(&undoStack[undoTop]);
  undoTop = (byte)((undoTop + 1) % UNDO_LEVELS);
  if (undoDepth < UNDO_LEVELS) undoDepth++;
  // [6S] A new edit ends the redo chain. The states it led to describe a pattern
  // that no longer exists, and offering to walk forward into them would be undo's
  // own failure mode - putting back almost the right thing - wearing a new name.
  redoDepth = 0;
}

// [6S] Everything both directions have in common. The hi-hat trigger lane is
// derived from CH and OH and the scale button's counter from the pattern scale,
// so both are rebuilt from what was just put back rather than left describing the
// state we came from.
static void UndoAfter()
{
  SetHHPattern();
  InitPatternBuf(ptrnBuffer);
  LaneSyncAll();                  // [F27] the restored record may have other lane lengths
  patternWasEdited = TRUE;
  needLcdUpdate = TRUE;
}

// TRUE if something was undone.
boolean UndoPop()
{
  if (!undoDepth) return FALSE;

  undoTop = (byte)((undoTop + UNDO_LEVELS - 1) % UNDO_LEVELS);
  undoDepth--;
  // [6S] The swap leaves the state we are leaving in the slot we took this one
  // from, which is exactly what redo needs and costs nothing to keep.
  UndoRestore(&undoStack[undoTop]);
  if (redoDepth < UNDO_LEVELS) redoDepth++;

  UndoAfter();
  return TRUE;
}

// [6S] TRUE if something was redone. The mirror of UndoPop(): swap the slot the
// cursor is on and walk forward.
boolean RedoPop()
{
  if (!redoDepth) return FALSE;

  UndoRestore(&undoStack[undoTop]);
  undoTop = (byte)((undoTop + 1) % UNDO_LEVELS);
  redoDepth--;
  if (undoDepth < UNDO_LEVELS) undoDepth++;

  UndoAfter();
  return TRUE;
}

// Dropped when the pattern underneath changes: the snapshots describe a
// pattern that is no longer loaded, and restoring one into a different pattern
// would be a corruption dressed up as a feature.
void UndoDiscard()
{
  undoDepth = 0;
  redoDepth = 0;                  // [6S]
  undoTop = 0;
}

//-------------------------------------------------
//                  NAVA v2.x
//        Pattern / track browser  [F26]
//
// The other half of naming. Names were worth adding because a pattern is easier
// to remember as "intro" than as C3, but until now the only way to reach one was
// still to know which pad it was under - so the name was a label you could read
// and never a thing you could look for.
//
// The list is read from the EEprom a name at a time rather than held in RAM.
// 128 patterns at twelve bytes is 1536 bytes, a tenth of the chip, to cache
// something the hand moves through one entry at a time; a twelve-byte read at
// 400 kHz is well under a millisecond and happens only when the encoder moves.
//-------------------------------------------------

////////////////////////Function//////////////////////

// Pull one name out of the bank. The record header puts a pattern's name at
// offset 4 and a track's at offset 0 - see struct Pattern and struct Track.
// [3C] Split out so the BANK and TRACK scroll overlays can name what they are
// about to select without opening the browser. Same twelve bytes, same page.
void NamePeek(byte n, boolean isTrack)
{
  const unsigned long addr = isTrack ? TrackAddr(n) : (PatternAddr(n) + 4);
  EepRead(addr, (byte *)browseName, NAME_LEN + 1);
  browseName[NAME_LEN] = '\0';

  // A record that has never been written reads back as 0xFF, which the display
  // would render as sixteen blocks. Show it as empty instead: "nothing here" is
  // the true answer and it is what the hand is looking for when it scrolls past.
  for (byte i = 0; i < NAME_LEN; i++){
    if (browseName[i] < ' ' || browseName[i] > '~') browseName[i] = ' ';
  }

  // [3C] ...and how long it is. A name tells you which one it is; the length
  // tells you whether it is the one you want, and selecting a pattern you have
  // not heard yet was the browser's whole weakness. Two more bytes off the same
  // page, so it costs nothing the name did not already cost.
  byte b[2];
  if (isTrack){
    EepRead(TrackAddr(n) + 12, b, 2);                       // struct Track.length
    browseLen = (unsigned int)b[0] | ((unsigned int)b[1] << 8);
    if (browseLen > MAX_PTRN_TRACK) browseLen = 0;
  }
  else {
    EepRead(PatternAddr(n) + 16, b, 1);                     // struct Pattern.length
    browseLen = (b[0] < NBR_STEP) ? (unsigned int)(b[0] + 1) : 0;
  }
}

void BrowsePeek() { NamePeek(browseSel, browseIsTrack); }

void BrowseEnter()
{
  browseIsTrack = (curSeqMode == TRACK_PLAY || curSeqMode == TRACK_WRITE);
  browseSel     = browseIsTrack ? trk.current : curPattern;
  browseMode    = TRUE;
  BrowsePeek();
  needLcdUpdate = TRUE;
}

void BrowseExit()
{
  browseMode = FALSE;
  needLcdUpdate = TRUE;
}

void BrowseEncoder()
{
  const byte last = browseIsTrack ? (byte)(MAX_TRACK - 1) : (byte)(MAX_PTRN - 1);
  int v = (int)browseSel;

  // INST steps a bank at a time. 128 patterns is a long way at one detent per
  // entry, and the banks are how the panel already divides them up.
  v = EncGet(v, instBtn ? NBR_PATTERN : 1);

  // Stop at the ends rather than wrapping. A list you can fall off the end of is
  // a list you can lose your place in, and there is no other cue to where you
  // are than the number.
  if (v < 0) v = 0;
  if (v > (int)last) v = last;

  if ((byte)v != browseSel){
    browseSel = (byte)v;
    BrowsePeek();
    needLcdUpdate = TRUE;
  }
}

void BrowseButtons()
{
  // The encoder press is the select, which is what makes this a list rather
  // than a display: scroll to it, push, done. It leaves the page too - the
  // choice has been made and there is nothing else to do here.
  if (encBtn.justPressed){
    if (browseIsTrack){
      trk.next = browseSel;
      selectedTrackChanged = TRUE;
    }
    else {
      nextPattern = browseSel;
      // [5F] And it ends any chain, as a pad press does. Picking one pattern from
      // the list with a chain still running underneath would have played it for a
      // bar and then carried on round the chain.
      group.length = 0;
      group.pos    = 0;
      // Queued exactly as a pad press queues it, so a change made from the list
      // lands on the bar line like any other and the two ways in cannot drift
      // into behaving differently.
      if (PlayingPattern() != nextPattern) selectedPatternChanged = TRUE;
      curBank = (byte)(browseSel / NBR_PATTERN);
    }
    BrowseExit();
    return;
  }

  // A pad jumps within the current bank, so the list and the pads address the
  // same thing rather than being two separate ways to be somewhere.
  if (inStepPressed && !StepModifierHeld()){
    const byte n = FirstBitOn();
    const byte last = browseIsTrack ? (byte)(MAX_TRACK - 1) : (byte)(MAX_PTRN - 1);
    byte v = browseIsTrack ? n : (byte)(curBank * NBR_PATTERN + n);
    if (v > last) v = last;
    if (v != browseSel){
      browseSel = v;
      BrowsePeek();
      needLcdUpdate = TRUE;
    }
  }

  // [3C] NUM alone swaps which list you are in. One browser, two kinds of thing,
  // and the key that opened it is the one that changes its mind. INST+NUM still
  // closes it.
  if (numBtn.justPressed && !instBtn){
    browseIsTrack = !browseIsTrack;
    const byte last = browseIsTrack ? (byte)(MAX_TRACK - 1) : (byte)(MAX_PTRN - 1);
    browseSel = browseIsTrack ? trk.current : curPattern;
    if (browseSel > last) browseSel = last;
    BrowsePeek();
    needLcdUpdate = TRUE;
  }

  // CLEAR goes back to where you actually are. Scrolling away from the playing
  // pattern to look at something and then wanting out is the common case, and
  // hunting back to it by number is not an answer.
  if (clearBtn.justPressed){
    browseSel = browseIsTrack ? trk.current : curPattern;
    BrowsePeek();
    needLcdUpdate = TRUE;
  }
}

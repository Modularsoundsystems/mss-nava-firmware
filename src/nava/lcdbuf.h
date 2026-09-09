//-------------------------------------------------
//                  NAVA v2.x
//        Buffered LCD - incremental update  [3A]
//
// LcdUpdate() redrew both lines in full on every needLcdUpdate. In 4-bit mode
// each character is two nibble writes and LiquidCrystal settles 100 us after
// each one, so a character costs ~206 us and a two-line redraw ~6.6 ms of
// blocked main loop - during which ButtonGet(), EncGet() and MIDI.read() all
// stop. Turning the encoder redraws on every detent, which is exactly when
// responsiveness matters most.
//
// The fix is not to rewrite the layout code. `lcd` is now a framebuffer that
// mimics the small slice of the LiquidCrystal API those 200 lines use, so
// every existing call site is unchanged; LcdFlush() then diffs against a
// shadow of what the panel is actually showing and writes only the runs that
// differ. A typical update touches two or three cells: ~0.4 ms instead of
// ~6.6 ms, and an update that changes nothing costs a 32-byte compare.
//
// Overload set matches Arduino's Print deliberately: print(char) emits a
// character, print(unsigned char) emits a decimal number. Existing code relies
// on both - lcd.print(char(curBank+65)) vs lcd.print(seq.TXchannel).
//-------------------------------------------------

#ifndef NAVA_LCDBUF_H
#define NAVA_LCDBUF_H

#define LCD_COLS 16
#define LCD_ROWS 2

extern LiquidCrystal lcdHw;          // the real panel, defined in main.cpp

uint8_t lcdFb[LCD_ROWS][LCD_COLS];   // what we want shown
uint8_t lcdShadow[LCD_ROWS][LCD_COLS];// what the panel is showing

class LcdBuffered
{
public:
  // Pass-through: these touch the controller, not the frame.
  void begin(uint8_t cols, uint8_t rows) { lcdHw.begin(cols, rows); }
  void createChar(uint8_t n, uint8_t *d) { lcdHw.createChar(n, d); }

  // clear() no longer costs the controller's 2 ms clear-and-home; the diff
  // writes spaces over whatever differs.
  void clear()                           { Fill(' '); col = 0; row = 0; }

  void setCursor(uint8_t c, uint8_t r)   { col = c; row = r; }

  void write(uint8_t c)                  { Put(c); }
  void print(char c)                     { Put((uint8_t)c); }
  void print(const char *s)              { while (*s) Put((uint8_t)*s++); }

  // [3B] Strings wrapped in F() stay in flash. On AVR a plain literal is
  // copied into RAM at startup and never leaves, and this firmware has ~30
  // sixteen-character panel strings - about 500 bytes of the RAM that the
  // 896-byte pattern records need.
  void print(const __FlashStringHelper *s)
  {
    const char *p = (const char *)s;
    char c;
    while ((c = pgm_read_byte(p++)) != 0) Put((uint8_t)c);
  }
  void print(unsigned char n)            { Num((unsigned long)n); }
  void print(int n)                      { Num((long)n); }
  void print(unsigned int n)             { Num((unsigned long)n); }
  void print(long n)                     { Num(n); }
  void print(unsigned long n)            { Num(n); }

private:
  uint8_t col, row;

  // Off-panel writes are dropped. The real controller would push them into
  // invisible DDRAM (row 0 runs to 0x27 before row 1 begins at 0x40), so
  // clipping matches what you could actually see.
  void Put(uint8_t c)
  {
    if (row < LCD_ROWS && col < LCD_COLS) lcdFb[row][col++] = c;
    else if (row < LCD_ROWS) col++;
  }

  void Fill(uint8_t c)
  {
    for (uint8_t r = 0; r < LCD_ROWS; r++)
      for (uint8_t i = 0; i < LCD_COLS; i++) lcdFb[r][i] = c;
  }

  void Num(long v)
  {
    if (v < 0){ Put('-'); v = -v; }
    Num((unsigned long)v);
  }

  void Num(unsigned long v)
  {
    char tmp[10];
    uint8_t n = 0;
    do { tmp[n++] = '0' + (char)(v % 10); v /= 10; } while (v);
    while (n) Put((uint8_t)tmp[--n]);
  }
};

#endif


#define VIDEO_INT           0x10
#define SET_MODE            0x00
#define VGA_256_COLOR_MODE  0x13
#define TEXT_MODE           0x03

#ifndef LINES
#define LINES 25
#endif

unsigned char text[50 * 80 * 2];

void set_mode(unsigned char mode)
{
  union REGS regs;

  regs.h.ah = SET_MODE;
  regs.h.al = mode;
  int386(VIDEO_INT, &regs, &regs);
}

void set_rows(int aa)
{
  union REGS regs;

  regs.h.ah = 0x11;
  regs.h.al = aa ? 0x12 : 0x14;
  regs.h.bl = 0x0;
  int386(VIDEO_INT, &regs, &regs);
}

void set_color(char n, char r, char g, char b)
{
  union REGS regs;

  regs.h.ah = 0x10;
  regs.h.al = 0x10;
  regs.h.bh = 0;
  regs.h.bl = n;
  regs.h.ch = g;
  regs.h.cl = b;
  regs.h.dh = r;

  int386(VIDEO_INT, &regs, &regs);
}

void set_cursor(int x, int y)
{
  union REGS regs;

  regs.h.ah = 0x02;
  regs.h.dh = y;
  regs.h.dl = x;
  regs.h.bh = 0x0;
  int386(VIDEO_INT, &regs, &regs);
}


void set_glyph(int n, const char* glyph, int h)
{
  char* video = (char *)0xA0000L;
  int i;
  outpw(0x3ce, 5);
  outpw(0x3ce, 0x406);
  outpw(0x3c4, 0x402);
  outpw(0x3c4, 0x604);
  for(i = 0; i < h; ++i) video[i + 32 * n] = glyph[i + (h == 16 ? 0 : 5)];
  outpw(0x3c4, 0x302);
  outpw(0x3c4, 0x204);
  outpw(0x3ce, 0x1005);
  outpw(0x3ce, 0xe06);
}

void text_strcpy(int x, int y, const char *src, char color)
{
  char *tgt = text + y * 160 + x * 2;
  while(*src) {
    tgt[0] = *src;
    tgt[1] = color;
    tgt += 2;
    src += 1;
  }
}

void text_strcpy2(int x, int y, const char *src, char color, char spacecolor)
{
  char *tgt = text + y * 160 + x * 2;
  while(*src) {
    tgt[0] = *src;
    if(*src == ' ') tgt[1] = spacecolor; else tgt[1] = color;
    tgt += 2;
    src += 1;
  }
}

void draw_char(int x, int y, char c)
{
  text[y * 160 + x * 2] = c;
}

void draw_color(int x, int y, char c)
{
  text[y * 160 + x * 2 + 1] = c;
}

void draw_char_color(int x, int y, char c, char d)
{
  text[y * 160 + x * 2] = c;
  text[y * 160 + x * 2 + 1] = d;
}

void update_text()
{
  char* video = (char *)0xB8000L;
  int i;
  for(i = 0; i < LINES * 80 * 2; ++i) video[i] = text[i];
}

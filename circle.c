
#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <conio.h>
#include <string.h>
#include <direct.h>
#include <time.h>

#include "lz4.h"

#include "pattern.h"

struct { int lines, port; } config = {25, 0x388};

int master_key_offset = 0;

#define LINES (config.lines)
#define MOUSE_VSHIFT 3
#define SPLIT (LINES - 6)

int debug_vars[8];

int bounds(int aa, int bb)
{
  if (aa < 0) return 0;
  else if (aa > bb) return bb;
  else return aa;
}

int bounds2(int aa, int bb, int cc)
{
  if (aa < bb) return bb;
  else if (aa > cc) return cc;
  else return aa;
}

#include "strings.h"
#include "timer.h"
#include "vga.h"
#include "opl.h"
#include "scancode.h"
#include "keyboard.h"
#include "mouse.h"

#define COLUMNS_PER_CHANNEL 6
#define MELODY_CHANNELS 6
#define DRUM_CHANNELS 3
#define ROW_LCOLUMNS 2
#define MELODY_COLUMNS (COLUMNS_PER_CHANNEL * MELODY_CHANNELS)
#define DRUM_COLUMNS (COLUMNS_PER_CHANNEL * DRUM_CHANNELS)
#define CHANNEL_COLUMNS (MELODY_COLUMNS + DRUM_COLUMNS)
#define DRUM_COLUMN_START (ROW_LCOLUMNS + CHANNEL_COLUMNS)

#define INST_LR_PARAMS 8
#define INST_OPS 4
#define COLUMNS_PER_OP 12
#define OP_COLUMNS COLUMNS_PER_OP * INST_OPS

#define PATTERN_COLUMNS (DRUM_COLUMN_START + 5)
#define INST_COLUMNS (INST_LR_PARAMS + OP_COLUMNS)

#define UNDO_SIZE (256 * 1024)
#define UNDO_STEPS (4 * 1024)

#define PLAY_PLAY 0x1
#define PLAY_ADVANCE 0x2
#define PLAY_LOOP 0x4
#define PLAY_SINGLE_STEP 0x8
#define PLAY_TAP 0x10

struct s_pos {
  int xx, yy;
};

struct s_select {
  struct s_pos cursor, mark, end, max;
  int scroll, rows;
};

struct s_nav {
  struct s_select pattern, inst;
  char filepath[1024];
  //char pattern_mask[PATTERN_COLUMNS];
  //char inst_mask[INST_COLUMNS];
};

struct s_edit {
  struct s_nav nav;
  struct s_pattern pt;
};

struct s_undo_char {
  char *pp, aa;
};

int cursor_mode = 0, show_inst = 1;
int delta_mode = 1;
int record_mode = 0, record_note = 0, record_key_on = 0;
int current_page = 0;
int octave = 4;

int redraw = 1;
int play_mode = 0, ticks = 0, note_tick = 0;
int channel_on = 0xff, drums_on = 0xff;

struct s_edit edit_aa, edit_bb, edit_cc, *edit = &edit_aa;

struct s_undo_char undo_chars[UNDO_SIZE];
int undo_index[UNDO_STEPS];
int undo_start = 0, undo_end = 1, undo_max = 1, undo_change = 0, song_change[2] = {0, 0};

void set_char(char* pp, char aa) {
  if(*pp != aa && pp >= (char*)&edit->pt && pp < (char*)((&edit->pt) + 1)) {
    if(!undo_change) {
      undo_index[undo_end] = undo_index[(undo_end - 1) & (UNDO_SIZE - 1)];    
    }
    undo_index[undo_end]++;
    undo_index[undo_end] &= UNDO_SIZE - 1;
    undo_chars[undo_index[undo_end]].pp = pp;
    undo_chars[undo_index[undo_end]].aa = *pp;
    undo_change = 1; song_change[current_page] = 1;
    if(undo_index[undo_end] == undo_index[undo_start]) {
      undo_start++;
      undo_start &= UNDO_STEPS - 1;
    }
  }
  *pp = aa;
}

void set_short(unsigned short* pp, unsigned short aa) {
  set_char((char*)pp, aa & 0xff);
  set_char(((char*)pp) + 1, aa >> 8);
}


void undo_step() {
  if(!undo_change) return;

  undo_end++;
  undo_end &= UNDO_STEPS - 1;

  undo_max = undo_end;

  if(undo_start == undo_end) {
    undo_start++;
    undo_start &= UNDO_STEPS - 1;
  }

  undo_change = 0;
}

void swap_chars(char *aa, char *bb) 
{
  char cc = *aa;
  *aa = *bb;
  *bb = cc;
}

void undo() {
  int ii, aa, bb;

  if(((undo_end - 1) & (UNDO_STEPS - 1)) == undo_start) return;

  undo_end--;
  undo_end &= UNDO_STEPS - 1;

  aa = undo_index[undo_end];
  bb = undo_index[(undo_end - 1) & (UNDO_STEPS - 1)];

  for(ii = aa; ii != bb; ii = (ii - 1) & (UNDO_SIZE - 1)) swap_chars(undo_chars[ii].pp, &undo_chars[ii].aa);
}

void redo() {
  int ii, aa, bb;

  if((undo_end & (UNDO_STEPS - 1)) == undo_max) return;

  aa = (undo_index[undo_end] + 1) & (UNDO_SIZE - 1);
  bb = (undo_index[(undo_end - 1) & (UNDO_STEPS - 1)] + 1) & (UNDO_SIZE - 1);

  for(ii = bb; ii != aa; ii = (ii + 1) & (UNDO_SIZE - 1)) swap_chars(undo_chars[ii].pp, &undo_chars[ii].aa);

  undo_end++;
  undo_end &= UNDO_STEPS - 1;
}

void undo_init() {
  memset(undo_index, 0, UNDO_STEPS * sizeof(int));
}

void set_palette()
{
  char light[] = {0, 3, 6, 12, 24, 48, 63};
  char dark[] = {0, 2, 4, 8, 16, 32, 63};
  int i;

  if(current_page == 0) {
    for(i = 0; i < 6; ++i) set_color(i, light[i], light[i], dark[i]);
    set_color(20, light[6], light[6], dark[6]);
  } else {
    for(i = 0; i < 6; ++i) set_color(i, dark[i], dark[i], light[i]);
    set_color(20, dark[6], dark[6], light[6]);
  } 

  set_color(57, 63, 0, 0);
}

int column_channel(int aa)
{
  return bounds((aa - ROW_LCOLUMNS) / COLUMNS_PER_CHANNEL, MELODY_CHANNELS + DRUM_CHANNELS - 1);
}

int column_channel_offset(int aa)
{
  return (aa - ROW_LCOLUMNS) % COLUMNS_PER_CHANNEL;
}

int column_inst(int aa)
{
  return bounds((aa - INST_LR_PARAMS) / COLUMNS_PER_OP, INST_OPS - 1);
}

int column_inst_offset(int aa)
{
  return (aa - INST_LR_PARAMS) % COLUMNS_PER_OP;
}

struct s_pos tf_pattern(struct s_pos aa)
{
  if(aa.xx == 0);
  else if (aa.xx == 1) ++aa.xx;
  else if (aa.xx < ROW_LCOLUMNS + CHANNEL_COLUMNS) {
    int j = column_channel_offset(aa.xx);
    if(j >= 3) ++j;
    aa.xx = j + column_channel(aa.xx) * 8 + ROW_LCOLUMNS + 2;
  } else {
    aa.xx += 19;
  }

  aa.yy = bounds(aa.yy - edit->nav.pattern.scroll, edit->nav.pattern.rows - 1);

  return aa;
}

int tf_pattern_reverse(int xx)
{
  if(xx == 0) return 0;
  else if(xx < 4) return 1;
  else if(xx < 75) {
    int j = (xx - 4) % 8;
    if(j >= 4) --j;
    return ((xx - 4) / 8) * 6 + j + 2;
  } else return xx - 19;
}

int tf_inst_reverse(int xx)
{
  const int ofs[] = {0, 1, 2, 3, 3, 4, 5, 6, 6, 6, 7, 8, 9, 10, 10, 11, 11};
  if(xx < 8) return xx;
  else if(xx == 8) return 7;
  else return ((xx - 9) / 17) * 12 + ofs[(xx - 9) % 17] + 8;
}

struct s_pos tf_inst(struct s_pos aa)
{
  const int ofs[] = {0, 1, 2, 3, 5, 6, 8, 10, 11, 12, 13, 15};
  if (aa.xx < INST_LR_PARAMS);
  else {
    aa.xx = ofs[column_inst_offset(aa.xx)] + column_inst(aa.xx) * 17 + INST_LR_PARAMS + 1;
  }

  aa.yy = bounds(aa.yy - edit->nav.inst.scroll, edit->nav.inst.rows - 1);
  aa.yy += edit->nav.pattern.rows + 1;

  return aa;
}

void set_cursor2(struct s_pos aa) {
  text[aa.yy * 160 + aa.xx * 2 + 1] = 0x50;
}

int inside(struct s_pos aa, struct s_pos tl, struct s_pos br)
{
  return aa.xx >= tl.xx && aa.xx <= br.xx && aa.yy >= tl.yy && aa.yy <= br.yy;
}

void draw_cursor()
{
  struct s_pos pattern_tl, pattern_br, inst_tl, inst_br, aa;

  if(cursor_mode == 0) {
    set_cursor2(tf_pattern(edit->nav.pattern.cursor));
  } else {
    set_cursor2(tf_inst(edit->nav.inst.cursor));
  }

  pattern_tl = tf_pattern(edit->nav.pattern.mark);
  pattern_br = tf_pattern(edit->nav.pattern.end);

  inst_tl = tf_inst(edit->nav.inst.mark);
  inst_br = tf_inst(edit->nav.inst.end);

  for(aa.yy = 0; aa.yy < LINES; ++aa.yy)
  for(aa.xx = 0; aa.xx < 80; ++aa.xx) {
    if (inside(aa, pattern_tl, pattern_br) || inside(aa, inst_tl, inst_br))
      text[aa.yy * 160 + aa.xx * 2 + 1] += 0x11;
  }
}

struct s_pos get_mouse_screen_pos()
{
  struct s_pos aa;
  aa.xx = mouse.cx >> 3; aa.yy = mouse.dx >> MOUSE_VSHIFT; 
  return aa;
}

int get_mouse_mode(struct s_pos aa)
{
  return (aa.yy > edit->nav.pattern.rows) ? 1 : 0;
}

struct s_select* get_mode_select(int mode)
{
  return mode ? &edit->nav.inst : &edit->nav.pattern;
}

struct s_pos get_mouse_pos(struct s_pos aa, int mode)
{
  struct s_select* sel = get_mode_select(mode);
  aa.xx = bounds(mode ? tf_inst_reverse(aa.xx) : tf_pattern_reverse(aa.xx), sel->max.xx - 1),
  aa.yy = bounds(aa.yy - (mode ? edit->nav.pattern.rows + 1 : 0), sel->rows - 1);
  aa.yy = bounds(aa.yy + sel->scroll, sel->max.yy - 1);
  return aa;
}

void draw_mouse()
{
  const char *t1 = STRING_EMPTY, *t2 = STRING_EMPTY;
  struct s_pos screen = get_mouse_screen_pos();
  int mode = get_mouse_mode(screen);
  struct s_pos pos = get_mouse_pos(screen, mode);

  int ii, kk, ll, mm;

  draw_color(screen.xx, screen.yy, 0x50);

  if((mouse.bx & 2) && mode == 0) {
    if(pos.xx == 0) t2 = STRING_INFO_LOOP;
    else if(pos.xx == 1) t2 = STRING_INFO_TICKS;
    else if(pos.xx >= ROW_LCOLUMNS && pos.xx < ROW_LCOLUMNS + CHANNEL_COLUMNS) {
      t1 = STRING_INFO_PATTERN[column_channel(pos.xx)];
      t2 = STRING_INFO_CHANNEL[column_channel_offset(pos.xx)];
    }
    else t2 = STRING_INFO_DRUMS[screen.xx - 75];
  } 

  if((mouse.bx & 2) && mode == 1) {
    if(pos.xx <= INST_LR_PARAMS) t2 = STRING_INFO_INST[pos.xx];
    else {
      kk = column_channel(edit->nav.pattern.cursor.xx);
      ll = column_inst(pos.xx);
      mm = column_inst_offset(pos.xx);
      if(kk < MELODY_CHANNELS) t1 = STRING_INFO_OPERATOR_NAME[ll];
      else if(ll < 2) t1 = STRING_INFO_DRUM_CHANNEL[(kk - MELODY_CHANNELS) * 2 + ll];
      if(mm < 11) t2 = STRING_INFO_OPERATOR[mm];
      else t2 = STRING_INFO_WAVE[edit->pt.inst[pos.yy].op[ll].xe0 & 7];
    }
  }

  ll = strlen(t1); 
  mm = strlen(t2);
  kk = (ll ? 1 : 0) + (mm ? 1 : 0);
  if(mm > ll) ll = mm;

  screen.xx = bounds(screen.xx + 1, 80 - ll);
  screen.yy = bounds(screen.yy + 1, 25 - kk);
  for(ii = 0; ii < ll; ++ii) {
    draw_char_color(screen.xx + ii, screen.yy, ' ', 0x03);
    if(t1 != STRING_EMPTY) draw_char_color(screen.xx + ii, screen.yy + 1, ' ', 0x03);
  }

  if(t1 != STRING_EMPTY) {
    text_strcpy(screen.xx, screen.yy, t1, 0x03);
    ++screen.yy;
  }

  text_strcpy(screen.xx, screen.yy, t2, 0x03);
}

void andor(char *aa, char bb, char cc)
{
  *aa = (*aa & bb) | cc;
}

char key_on_char(char k)
{
  return STRING_KEY_ON + (
    ((k & 0x03) ? 0x1 : 0) | ((k & 0x0c) ? 0x2 : 0) | 
    ((k & 0x30) ? 0x4 : 0) | ((k & 0xc0) ? 0x8 : 0));
}

char hex(char k)
{
  return STRING_HEXA + k;
}

void pattern_draw()
{
  char mm[16];

  const struct s_nav *nav = &edit->nav;
  const struct s_pattern *pt = &edit->pt;
  const struct s_channel *ch, *pch;
  const struct s_inst *inst;
  const struct s_op *op;

  int i, j, k, l, m, n, pk, delta;

  for(i = 0; i < nav->pattern.rows; ++i) { 
    j = (i + nav->pattern.scroll) & 3;
    text_strcpy2(0, i, STRING_PATTERN_ROW, j ? 0x10 : 0x20, j ? 0x14 : 0x24);
  }
  text_strcpy(0, nav->pattern.rows, STRING_SEPARATOR, 0x10);

  for(i = 0; i < nav->inst.rows; ++i) {
    text_strcpy2(0, nav->pattern.rows + 1 + i, STRING_INST_ROW, 0x10, 0x14);
  }
  text_strcpy(0, LINES - 1, STRING_STATUS, 0x01);

  sprintf(mm, "%3i", pt->head.tempo); text_strcpy(4, LINES - 1, mm, 0x02);

  sprintf(mm, "%4i", pt->head.length); text_strcpy(10, LINES - 1, mm, 0x02);

  sprintf(mm, "%1i", octave); text_strcpy(19, LINES - 1, mm, 0x02);
  sprintf(mm, "%4i", nav->pattern.cursor.yy); text_strcpy(25, LINES - 1, mm, 0x02);
  sprintf(mm, "%4i", nav->pattern.mark.yy); text_strcpy(30, LINES - 1, mm, 0x02);
  sprintf(mm, "%4i", nav->pattern.end.yy); text_strcpy(35, LINES - 1, mm, 0x02);
  sprintf(mm, "%4i", nav->pattern.end.yy - nav->pattern.mark.yy + 1); text_strcpy(40, LINES - 1, mm, 0x02);

  sprintf(mm, "%s", song_change[current_page] ? "*" : " "); text_strcpy(49, LINES - 1, mm, 0x02);

  draw_color(73, LINES - 1, (play_mode && (nav->pattern.cursor.yy & 3) == 0) ? 0x50 : 0x10);
  draw_color(74, LINES - 1, (play_mode && (nav->pattern.cursor.yy & 3) == 0) ? 0x50 : 0x10);

  draw_color(76, LINES - 1, (play_mode & PLAY_ADVANCE) ? 0x50 : 0x10);
  draw_color(77, LINES - 1, (play_mode & PLAY_ADVANCE) ? 0x50 : 0x10);

  draw_color(78, LINES - 1, (record_mode & 1) ? 0x59 : 0x10);
  draw_color(79, LINES - 1, (record_mode & 1) ? 0x59 : 0x10);

  for(i = 0; i < 6; ++i ) if(channel_on & (1 << i)) draw_char_color(51 + i, LINES - 1, STRING_LIVE, 0x05);
  for(i = 0; i < 5; ++i ) if(drums_on & (2 << i)) draw_char_color(58 + i, LINES - 1, STRING_LIVE, 0x05);
  for(i = 0; i < 5; ++i ) if(record_mode & (2 << i)) draw_char_color(64 + i, LINES - 1, STRING_LIVE, 0x09);
  for(i = 0; i < 2; ++i ) if(record_mode & (0x40 << i)) draw_char_color(70 + i, LINES - 1, STRING_LIVE, 0x09);

  for(i = 0; i < nav->pattern.rows; ++i) {
    k = i + nav->pattern.scroll;
    pk = (k - 1) & (TT_PATTERN_ROWS - 1); delta = delta_mode && k != nav->pattern.cursor.yy && !pt->row[k].loop; 
    draw_char(0, i, STRING_LOOP_FLAG[pt->row[k].loop ? 1 : 0]);
    if(!delta || pt->row[k].ticks != pt->row[pk].ticks) {
      draw_char(1, i, pt->row[k].ticks >= 0x10 ? hex(pt->row[k].ticks >> 4) : ' ');
      draw_char(2, i, hex(pt->row[k].ticks & 0xf));
    } else {
      draw_char(1, i, STRING_DELTA);
      draw_char(2, i, STRING_DELTA);
    }
    for(j = 0; j < 9; ++j) {
      ch = pt->row[k].ch + j;
      pch = pt->row[pk].ch + j;
      if(!delta || ch->note != pch->note) {
        if(ch->note > 0) {
          draw_char(j * 8 + 4, i, STRING_NOTE_NAMES[ch->note % 12]);
          draw_char(j * 8 + 5, i, '0' + ch->note / 12);
	  andor(text + (i * 80 + j * 8 + 4) * 2 + 1, 0xf0, 0x5);
	  andor(text + (i * 80 + j * 8 + 5) * 2 + 1, 0xf0, 0x5);
        } else {
          draw_char(j * 8 + 4, i, STRING_NOTE_NONE);
          draw_char(j * 8 + 5, i, STRING_NOTE_NONE);
        }
      } else {
        draw_char(j * 8 + 4, i, STRING_DELTA);
        draw_char(j * 8 + 5, i, STRING_DELTA);
      }
      
      draw_char(j * 8 + 6, i, key_on_char(ch->key_on));

      if(!delta || ch->inst != pch->inst) {
        draw_char(j * 8 + 7, i, hex(ch->inst >> 4));
        draw_char(j * 8 + 8, i, hex(ch->inst & 0xf));
      } else {
        draw_char(j * 8 + 7, i, STRING_DELTA);
        draw_char(j * 8 + 8, i, STRING_DELTA);
      }
      if(!delta || ch->control != pch->control) {
        draw_char(j * 8 + 9, i, hex(ch->control >> 4));
        draw_char(j * 8 + 10, i, hex(ch->control & 0xf));
      } else {
        draw_char(j * 8 + 9, i, STRING_DELTA);
        draw_char(j * 8 + 10, i, STRING_DELTA);
      }
    }
    for(j = 0; j < 5; ++j) draw_char(j + 75, i, key_on_char(pt->row[k].drums[j]));
  }


  for(i = 0; i < nav->inst.rows; ++i) {
    l = i + nav->pattern.rows + 1;
    k = i + nav->inst.scroll;
    
    inst = pt->inst + k;

    draw_char(0, l, hex(inst->porta[0] & 0xf));
    draw_char(1, l, hex(inst->tune[0] & 0xf));
    draw_char(2, l, hex((inst->xc0[0] >> 1) & 7));
    draw_char(3, l, STRING_SYN_FLAG[inst->xc0[0] & 1]);

    draw_char(4, l, hex(inst->porta[1] & 0xf));
    draw_char(5, l, hex(inst->tune[1] & 0xf));
    draw_char(6, l, hex((inst->xc0[1] >> 1) & 7));
    draw_char(7, l, STRING_SYN_FLAG[inst->xc0[1] & 1]);

    for(m = 0; m < 4; ++m) {
      op = inst->op + m;

      draw_char(m * 17 + 9, l, STRING_FLAG[(op->x20 & 0x80) ? 1 : 0]);
      draw_char(m * 17 + 10, l, STRING_FLAG[(op->x20 & 0x40) ? 1 : 0]);
      draw_char(m * 17 + 11, l, STRING_FLAG[(op->x20 & 0x20) ? 1 : 0]);
      draw_char(m * 17 + 12, l, STRING_FLAG[(op->x20 & 0x10) ? 1 : 0]);

      draw_char(m * 17 + 14, l, hex((op->x20 & 0xf)));
      draw_char(m * 17 + 15, l, hex((op->x40 >> 6)));
      draw_char(m * 17 + 16, l, hex((op->x40 >> 4) & 0x3));
      draw_char(m * 17 + 17, l, hex((op->x40) & 0xf));

      draw_char(m * 17 + 19, l, hex((op->x60 >> 4)));
      draw_char(m * 17 + 20, l, hex((op->x60) & 0xf));
      draw_char(m * 17 + 21, l, hex((op->x80 >> 4)));
      draw_char(m * 17 + 22, l, hex((op->x80) & 0xf));

      draw_char(m * 17 + 24, l, hex((op->xe0) & 0xf));
    }
  }

  draw_cursor(pt);
  
}

char enter_char(int scan_code)
{
  const char scan_code_char[] = 
    "    1!2@3#4$5%6^7&8*9(0)-_=+    "
    "qQwWeErRtTyYuUiIoOpP[{]}    aAsS"
    "dDfFgGhHjJkKlL;:'\"`~  \\|zZxXcCvV"
    "bBnNmM,<.>/?*";
  int shift = key_states[SCAN_LEFT_SHIFT] ? 1 : 0;
  if(scan_code <= 0x37 && scan_code_char[scan_code * 2] != ' ') return scan_code_char[scan_code * 2 + shift];
  if(scan_code == SCAN_SPACE) return ' ';
  return 0;
}

char enter_value(int scan_code, char tt, char max)
{
  switch (scan_code) {
    case SCAN_0:
    case SCAN_1:
    case SCAN_2:
    case SCAN_3:
    case SCAN_4:
    case SCAN_5:
    case SCAN_6:
    case SCAN_7:
    case SCAN_8:
    case SCAN_9:
    case SCAN_A:
    case SCAN_B:
    case SCAN_C:
    case SCAN_D:
    case SCAN_E:
    case SCAN_F:
      if(max > 0x10) {
        tt <<= 4;
        if(tt >= max) tt = max - 1;
      }
      tt &= 0xf0;
  }

  switch (scan_code) {
    case SCAN_0: break;
    case SCAN_1: tt |= 1; break;
    case SCAN_2: tt |= 2; break;
    case SCAN_3: tt |= 3; break;
    case SCAN_4: tt |= 4; break;
    case SCAN_5: tt |= 5; break;
    case SCAN_6: tt |= 6; break;
    case SCAN_7: tt |= 7; break;
    case SCAN_8: tt |= 8; break;
    case SCAN_9: tt |= 9; break;
    case SCAN_A: tt |= 0xa; break;
    case SCAN_B: tt |= 0xb; break;
    case SCAN_C: tt |= 0xc; break;
    case SCAN_D: tt |= 0xd; break;
    case SCAN_E: tt |= 0xe; break;
    case SCAN_F: tt |= 0xf; break;
    case SCAN_KP_PLUS: 
    case SCAN_EQUALS: 
      if (tt < max - 1) tt++; break;
    case SCAN_KP_MINUS: 
    case SCAN_MINUS:
      if(tt > 0) tt--; break;
  }

  if(tt >= max) tt = max - 1;

  return tt;
}

char enter_value_masked(int scan_code, char tt, char max, char mask, char shift)
{
  char aa;
  aa = (tt & mask) >> shift;
  aa = enter_value(scan_code, aa, max);
  aa = (tt & ~mask) | ((aa << shift) & mask);
  return aa;
}

int enter_note(int scan_code, int tt)
{
  switch(scan_code) {
    case SCAN_Z: return 0 + octave * 12;
    case SCAN_S: return 1 + octave * 12;
    case SCAN_X: return 2 + octave * 12;
    case SCAN_D: return 3 + octave * 12;
    case SCAN_C: return 4 + octave * 12;
    case SCAN_V: return 5 + octave * 12;
    case SCAN_G: return 6 + octave * 12;
    case SCAN_B: return 7 + octave * 12;
    case SCAN_H: return 8 + octave * 12;
    case SCAN_N: return 9 + octave * 12;
    case SCAN_J: return 10 + octave * 12;
    case SCAN_M: return 11 + octave * 12;
    case SCAN_COMMA: return 12 + octave * 12;

    case SCAN_Q: return 0 + octave * 12 + 12;
    case SCAN_2: return 1 + octave * 12 + 12;
    case SCAN_W: return 2 + octave * 12 + 12;
    case SCAN_3: return 3 + octave * 12 + 12;
    case SCAN_E: return 4 + octave * 12 + 12;
    case SCAN_R: return 5 + octave * 12 + 12;
    case SCAN_5: return 6 + octave * 12 + 12;
    case SCAN_T: return 7 + octave * 12 + 12;
    case SCAN_6: return 8 + octave * 12 + 12;
    case SCAN_Y: return 9 + octave * 12 + 12;
    case SCAN_7: return 10 + octave * 12 + 12;
    case SCAN_U: return 11 + octave * 12 + 12;
    case SCAN_I: return 12 + octave * 12 + 12;

    case SCAN_KP_PLUS: tt += 1; if (tt == 8 * 12) tt = 0; return tt;
    case SCAN_KP_MINUS: if (tt == 0) tt = 8 * 12; tt -= 1; return tt;
  }
  return 0;
}

char enter_key_on(int scan_code, char tt)
{
  switch(scan_code) {
    case SCAN_Z: return 0x00;
    case SCAN_X: return 0xff;
    case SCAN_1: return tt ^ 0x03;
    case SCAN_2: return tt ^ 0x0c;
    case SCAN_3: return tt ^ 0x30;
    case SCAN_4: return tt ^ 0xc0;
  }
  return tt;
}

char enter_flag(int scan_code, char tt, char flag)
{
  switch(scan_code) {
    case SCAN_1:
    case SCAN_0:
      tt ^= flag; break;
    case SCAN_2:
    case SCAN_EQUALS:
    case SCAN_KP_PLUS: 
      tt |= flag; break;
    case SCAN_3:
    case SCAN_MINUS:
    case SCAN_KP_MINUS: 
      tt &= ~flag; break;
  }
  return tt;
}

void enter_loop(int scan_code)
{
  int tt = edit->nav.pattern.mark.yy, bb = edit->nav.pattern.end.yy;
  int ii;
  for(ii = tt; ii <= bb; ++ii) {
    set_char(&edit->pt.row[ii].loop, enter_flag(scan_code, edit->pt.row[ii].loop, 1));
  }
}

void enter_ticks(int scan_code)
{
  int tt = edit->nav.pattern.mark.yy, bb = edit->nav.pattern.end.yy;
  int ii;
  for(ii = tt; ii <= bb; ++ii) {
    set_char(&edit->pt.row[ii].ticks, enter_value(scan_code, edit->pt.row[ii].ticks, 0xff));
  }
}

#define FOR_PATTERN_MARK(ch) \
  int ll = column_channel(edit->nav.pattern.mark.xx); \
  int rr = column_channel(edit->nav.pattern.end.xx); \
  int tt = edit->nav.pattern.mark.yy, bb = edit->nav.pattern.end.yy; \
  int ii, jj; \
  struct s_channel* ch; \
  for(ii = tt; ii <= bb; ++ii) \
  for(jj = ll, ch = edit->pt.row[ii].ch + jj; jj <= rr; ++jj, ch = edit->pt.row[ii].ch + jj) 
   
#define FOR_INST_MARK(inst, jj) \
  int ll = edit->nav.inst.mark.xx, rr = edit->nav.inst.end.xx; \
  int tt = edit->nav.inst.mark.yy, bb = edit->nav.inst.end.yy; \
  int jj, ii; \
  struct s_inst* inst; \
  for(ii = tt, inst = edit->pt.inst + ii; ii <= bb; ++ii, inst = edit->pt.inst + ii) \
  for(jj = ll; jj <= rr; ++jj) 

#define FOR_OP_MARK(op, mm) \
  int ll = edit->nav.inst.mark.xx, rr = edit->nav.inst.end.xx; \
  int tt = edit->nav.inst.mark.yy, bb = edit->nav.inst.end.yy; \
  int ii, jj, mm; \
  struct s_op* op; \
  for(ii = tt; ii <= bb; ++ii) \
  for(jj = ll, op = edit->pt.inst[ii].op + column_inst(jj), mm = column_inst_offset(jj); jj <= rr; \
  ++jj, op = edit->pt.inst[ii].op + column_inst(jj), mm = column_inst_offset(jj)) 
   
int enter_notes(int scan_code)
{
  char aa;
  int ret = 0;

  FOR_PATTERN_MARK(ch) {
    aa = enter_note(scan_code, ch->note);
    if(aa) { set_char(&ch->note, aa); ret |= 1; }
  }

  return ret;
}

void enter_octaves(int scan_code)
{
  char aa;

  FOR_PATTERN_MARK(ch) {
    aa = ch->note / 12; 
    aa = enter_value(scan_code, aa, 8);
    set_char(&ch->note, (ch->note % 12) + aa * 12);
  }
}

void enter_key_on_values(int scan_code)
{
  FOR_PATTERN_MARK(ch) {
    set_char(&ch->key_on, enter_key_on(scan_code, ch->key_on));
  }
}

void enter_inst(int scan_code)
{
  FOR_PATTERN_MARK(ch) {
    set_char(&ch->inst, enter_value(scan_code, ch->inst, 0xff));
  }
}

void enter_control_hi(int scan_code)
{
  FOR_PATTERN_MARK(ch) {
    set_char(&ch->control, enter_value_masked(scan_code, ch->control, 0x10, 0xf0, 4));
  }
}

void enter_control_lo(int scan_code)
{
  FOR_PATTERN_MARK(ch) {
    set_char(&ch->control, enter_value_masked(scan_code, ch->control, 0x10, 0xf, 0));
  }
}

void enter_inst_values(int scan_code)
{
  FOR_INST_MARK(inst, jj) {
    if(jj < 8) switch(jj & 3) {
      case 0: set_char(inst->porta + (jj >> 2), enter_value(scan_code, inst->porta[jj >> 2], 0x10)); break;
      case 1: set_char(inst->tune + (jj >> 2), enter_value(scan_code, inst->tune[jj >> 2], 0x10)); break;
      case 2: set_char(inst->xc0 + (jj >> 2), enter_value_masked(scan_code, inst->xc0[jj >> 2], 8, 0xe, 1)); break;
    }
  }
}

void enter_inst_flags(int scan_code)
{
  FOR_INST_MARK(inst, jj) {
    if (jj < 8 && (jj & 3) == 3) set_char(inst->xc0 + (jj >> 2), enter_flag(scan_code, inst->xc0[jj >> 2], 1));
  }
}

void enter_drums(int scan_code)
{
  int ll = bounds(edit->nav.pattern.mark.xx - ROW_LCOLUMNS - CHANNEL_COLUMNS, 4);
  int rr = bounds(edit->nav.pattern.end.xx - ROW_LCOLUMNS - CHANNEL_COLUMNS, 4);
  int tt = edit->nav.pattern.mark.yy;
  int bb = edit->nav.pattern.end.yy;
  int ii, jj;
  for(ii = tt; ii <= bb; ++ii) {
    for(jj = ll; jj <= rr; ++jj) {
      set_char(edit->pt.row[ii].drums + jj, enter_key_on(scan_code, edit->pt.row[ii].drums[jj]));
    }
  }
}

void enter_op_flags(int scan_code)
{
  char aa;

  FOR_OP_MARK(op, mm) {
    switch(mm) {
      case 0: aa = enter_flag(scan_code, op->x20, 0x80); break;
      case 1: aa = enter_flag(scan_code, op->x20, 0x40); break;
      case 2: aa = enter_flag(scan_code, op->x20, 0x20); break;
      case 3: aa = enter_flag(scan_code, op->x20, 0x10); break;
    }
    set_char(&op->x20, aa);
  }
}

void enter_op_values(int scan_code)
{
  FOR_OP_MARK(op, mm) {
    switch(mm) {
      case 4: set_char(&op->x20, enter_value_masked(scan_code, op->x20, 0x10, 0xf, 0)); break;
      case 5: set_char(&op->x40, enter_value_masked(scan_code, op->x40, 0x4, 0xc0, 6)); break;
      case 6: set_char(&op->x40, enter_value_masked(scan_code, op->x40, 0x40, 0x3f, 0)); break;
      case 7: set_char(&op->x60, enter_value_masked(scan_code, op->x60, 0x10, 0xf0, 4)); break;
      case 8: set_char(&op->x60, enter_value_masked(scan_code, op->x60, 0x10, 0xf, 0)); break;
      case 9: set_char(&op->x80, enter_value_masked(scan_code, op->x80, 0x10, 0xf0, 4)); break; 
      case 10: set_char(&op->x80, enter_value_masked(scan_code, op->x80, 0x10, 0xf, 0)); break;
      case 11: set_char(&op->xe0, enter_value_masked(scan_code, op->xe0, 0x8, 0x7, 0)); break;
    }
  }
}

void cursor_and_mark_rewind(struct s_select* ss, int mark, int end)
{
  if(ss->cursor.yy) ss->cursor.yy--;;
  if(ss->cursor.yy < ss->scroll) ss->scroll = ss->cursor.yy;
  if(mark && ss->mark.yy > 0) ss->mark.yy--;
  if(end && ss->end.yy > ss->mark.yy) ss->end.yy--;
}

void cursor_and_mark_advance(struct s_select* ss, int mark, int end)
{
  
  if (ss->cursor.yy < ss->max.yy - 1) ss->cursor.yy++;
  if(ss->cursor.yy > ss->scroll + ss->rows - 1) ss->scroll = ss->cursor.yy - ss->rows + 1;
  if (end && ss->end.yy < ss->max.yy - 1) ss->end.yy++;
  if (mark && ss->mark.yy < ss->end.yy) ss->mark.yy++;      
}

void set_play_mode(int aa)
{
  play_mode = aa;
  note_tick = 0;
}

void key_enter(int scan_code)
{
  int i;

  if (cursor_mode == 0) {
    i = edit->nav.pattern.cursor.xx;
    if (i == 0) enter_loop(scan_code);
    else if (i == 1) enter_ticks(scan_code);
    else if (i < ROW_LCOLUMNS + CHANNEL_COLUMNS) {
      switch(column_channel_offset(i)) {
        case 0:
          if (key_states[scan_code] == 0 && enter_notes(scan_code) && play_mode == 0) set_play_mode(PLAY_PLAY);
          break;
        case 1: enter_octaves(scan_code); break;
        case 2: enter_key_on_values(scan_code); break;
        case 3: enter_inst(scan_code); break;
        case 4: enter_control_hi(scan_code); break;
        case 5: enter_control_lo(scan_code); break;
      }
    } else {
      enter_drums(scan_code);
    }
  }

  if (cursor_mode == 1) {
    i = edit->nav.inst.cursor.xx;
    if (i < 8) {
      if((i & 3) == 3) enter_inst_flags(scan_code);
      else enter_inst_values(scan_code);
    } else {
      if(column_inst_offset(i) < 4) enter_op_flags(scan_code);
      else enter_op_values(scan_code);
    }
  }
}

char copy_mask(char aa, char bb, char mask)
{
  return (aa & ~mask) | (bb & mask);
}

int remap(int aa, int tgt_ll, int src_ll, int src_rr)
{
  return ((aa - tgt_ll) % (src_rr + 1 - src_ll)) + src_ll;
}

#define FOR_MARK(mm, ii, tgt_ii, src_ii) \
  for (tgt_ii = ee_tgt->nav.mm.mark.ii, \
  src_ii = remap(tgt_ii, ee_tgt->nav.mm.mark.ii, ee_src->nav.mm.mark.ii, ee_src->nav.mm.end.ii);\
  tgt_ii <= ee_tgt->nav.mm.end.ii; ++tgt_ii, \
  src_ii = remap(tgt_ii, ee_tgt->nav.mm.mark.ii, ee_src->nav.mm.mark.ii, ee_src->nav.mm.end.ii))

void copy(struct s_edit* ee_tgt, const struct s_edit* ee_src)
{
  struct s_pattern* pt_tgt = &ee_tgt->pt; const struct s_pattern* pt_src = &ee_src->pt;
  struct s_row *tgt_row; const struct s_row *src_row;
  struct s_channel *tgt_ch; const struct s_channel *src_ch;
  struct s_inst *tgt_inst; const struct s_inst *src_inst;
  struct s_op *tgt_op; const struct s_op *src_op;

  int tgt_xx, tgt_yy, src_xx, src_yy;
  int tgt_ii, src_ii, tgt_jj, src_jj;
  char aa;

  if (cursor_mode == 0) {
    FOR_MARK(pattern, yy, tgt_yy, src_yy) {
      tgt_row = pt_tgt->row + tgt_yy;
      src_row = pt_src->row + src_yy;

      FOR_MARK(pattern, xx, tgt_xx, src_xx) {
        tgt_ii = column_channel_offset(tgt_xx); tgt_ch = tgt_row->ch + column_channel(tgt_xx);
        src_ii = column_channel_offset(src_xx); src_ch = src_row->ch + column_channel(src_xx);
       
        if (src_xx == 0 && tgt_xx == 0) {
          set_char(&tgt_row->loop, src_row->loop);
        } else if (src_xx == 1 && tgt_xx == 1) {
          set_char(&tgt_row->ticks, src_row->ticks);
        } else if (src_xx >= ROW_LCOLUMNS && src_xx < ROW_LCOLUMNS + CHANNEL_COLUMNS && 
                   tgt_xx >= ROW_LCOLUMNS && tgt_xx < ROW_LCOLUMNS + CHANNEL_COLUMNS && 
                   (tgt_ii == src_ii || (tgt_ii >= 4 && src_ii >=4)) ) {

          switch(tgt_ii) {
            case 0: set_char(&tgt_ch->note, (tgt_ch->note / 12) * 12 + (src_ch->note % 12)); break;
            case 1: set_char(&tgt_ch->note, (src_ch->note / 12) * 12 + (tgt_ch->note % 12)); break;
            case 2: 
              set_char(&tgt_ch->key_on, src_ch->key_on); 
              break;
            case 3: set_char(&tgt_ch->inst, src_ch->inst); break;
            case 4:
              aa = src_ii == 5 ? src_ch->control & 0xf : src_ch->control >> 4;
	      set_char(&tgt_ch->control, copy_mask(tgt_ch->control, aa << 4, 0xf0));
              break;
            case 5:
              aa = src_ii == 5 ? src_ch->control & 0xf : src_ch->control >> 4;
	      set_char(&tgt_ch->control, copy_mask(tgt_ch->control, aa, 0x0f));
              break;
          }
        } else if (src_xx >= DRUM_COLUMN_START && 
                   tgt_xx >= DRUM_COLUMN_START) {
          set_char(tgt_row->drums + (tgt_xx - DRUM_COLUMN_START), src_row->drums[src_xx - DRUM_COLUMN_START]);
        }
      }
    }
  }

  if (cursor_mode == 1) {
    FOR_MARK(inst, yy, tgt_yy, src_yy) {
      tgt_inst = pt_tgt->inst + tgt_yy;
      src_inst = pt_src->inst + src_yy;
      
      FOR_MARK(inst, xx, tgt_xx, src_xx) {

        if(tgt_xx < 8 && tgt_xx < 8) {
          tgt_ii = tgt_xx & 3; tgt_jj = tgt_xx >> 2;
          src_ii = src_xx & 3; src_jj = src_xx >> 2;
	  if(tgt_ii == 3 && src_ii == 3) {
            set_char(tgt_inst->xc0 + tgt_jj, copy_mask(tgt_inst->xc0[tgt_jj], src_inst->xc0[src_jj], 0x1));
          } else {
            switch(src_ii) {
              case 0: aa = src_inst->porta[src_jj]; break;
              case 1: aa = src_inst->tune[src_jj]; break;
              case 2: aa = src_inst->xc0[src_jj] >> 1; break;
            }
            switch(tgt_ii) {
              case 0: set_char(tgt_inst->porta + tgt_jj, aa); break;
              case 1: set_char(tgt_inst->tune + tgt_jj, aa); break;
              case 2: set_char(tgt_inst->xc0 + tgt_jj, 
                copy_mask(tgt_inst->xc0[tgt_jj], bounds(aa, 0x7) << 1, 0xe)); break;
            }
          }
        } else if (src_xx >= 8 && tgt_xx >= 8) {
          tgt_ii = column_inst_offset(tgt_xx); tgt_op = tgt_inst->op + column_inst(tgt_xx);
          src_ii = column_inst_offset(src_xx); src_op = src_inst->op + column_inst(src_xx);
            
          if(tgt_ii < 4 && src_ii < 4) {
            switch(src_ii) {
              case 0: aa = (src_op->x20 & 0x80) >> 7; break;
              case 1: aa = (src_op->x20 & 0x40) >> 6; break;
              case 2: aa = (src_op->x20 & 0x20) >> 5; break;
              case 3: aa = (src_op->x20 & 0x10) >> 4; break;
            }
            switch(tgt_ii) {
              case 0: set_char(&tgt_op->x20, copy_mask(tgt_op->x20, aa << 7, 0x80)); break;
              case 1: set_char(&tgt_op->x20, copy_mask(tgt_op->x20, aa << 6, 0x40)); break;
              case 2: set_char(&tgt_op->x20, copy_mask(tgt_op->x20, aa << 5, 0x20)); break;
              case 3: set_char(&tgt_op->x20, copy_mask(tgt_op->x20, aa << 4, 0x10)); break;
            }
          }

          if(tgt_ii >= 4 && src_ii >= 4) {
            switch(src_ii) {
              case 4: aa = (src_op->x20 & 0xf); break;
              case 5: aa = (src_op->x40 & 0xc0) >> 6; break;
              case 6: aa = (src_op->x40 & 0x3f); break;
              case 7: aa = (src_op->x60 & 0xf0) >> 4; break;
              case 8: aa = (src_op->x60 & 0xf); break;
              case 9: aa = (src_op->x80 & 0xf0) >> 4; break;
              case 10: aa = (src_op->x80 & 0xf); break;
              case 11: aa = (src_op->xe0 & 0x7); break;
            }
            switch(tgt_ii) {
              case 4: set_char(&tgt_op->x20, copy_mask(tgt_op->x20, bounds(aa, 0xf), 0xf)); break;
              case 5: set_char(&tgt_op->x40, copy_mask(tgt_op->x40, bounds(aa, 0x3) << 6, 0xc0)); break;
              case 6: set_char(&tgt_op->x40, copy_mask(tgt_op->x40, bounds(aa, 0x3f), 0x3f)); break;
              case 7: set_char(&tgt_op->x60, copy_mask(tgt_op->x60, bounds(aa, 0xf) << 4, 0xf0)); break;
              case 8: set_char(&tgt_op->x60, copy_mask(tgt_op->x60, bounds(aa, 0xf), 0xf)); break;
              case 9: set_char(&tgt_op->x80, copy_mask(tgt_op->x80, bounds(aa, 0xf) << 4, 0xf0)); break;
              case 10: set_char(&tgt_op->x80, copy_mask(tgt_op->x80, bounds(aa, 0xf), 0xf)); break;
	      case 11: set_char(&tgt_op->xe0, copy_mask(tgt_op->xe0, bounds(aa, 0x7), 0x7)); break;
            }
          }
        }

      }
    }

  }
}

void record_key(int scan_code, int on)
{
  char aa = enter_note(scan_code, 0);
  if(aa) {
    if(on) { record_note = aa; record_key_on = 1; }
    else if (aa == record_note) record_key_on = 0;
  } 
}

int handle_key_event()
{
  int k = key_event[0], i;
  --key_event_index;
  for(i = 0; i <= key_event_index; ++i) key_event[i] = key_event[i + 1];
  return k;
}

int scan(const char *msg, char *ss, int nchars)
{
  int i, j;
  int c = strlen(ss);
  char cc;

  key_states[SCAN_LEFT_CONTROL] = 0;

  for(;;)
  {
    memset(text + (LINES - 1) * 160, 0, 160);
    text_strcpy(0, LINES - 1, msg, 0x03);
    text_strcpy(strlen(msg), LINES - 1, ss, 0x07);
    draw_color(strlen(msg) + c, LINES - 1, 0x70);
    update_text();

    for(;;) {
      if(key_event_index > 0) {
        i = handle_key_event();
        if(i & 0x200) {
          i &= 0x1ff;
          if(i == SCAN_ESC) return 0;
          if(i == SCAN_ENTER) return 1;
          if(i == SCAN_LEFT_ARROW && c > 0) { --c; break; }
          if(i == SCAN_RIGHT_ARROW && c < strlen(ss)) { ++c; break; }
          if(i == SCAN_BACKSPACE && c > 0) { --c; for(j = strlen(ss) - 1; j >= c; --j) ss[j] = ss[j + 1]; break; }
          if(i == SCAN_DELETE) { for(j = strlen(ss) - 1; j >= c; --j) ss[j] = ss[j + 1]; break; }
          cc = enter_char(i);
          if(cc && c < nchars) { ss[c] = cc; ++c; break; }
        }
      }
    }
  }
}

int scan_int(const char* msg, int* i)
{
   char buffer[8] = {0, 0, 0, 0, 0, 0, 0, 0};
   if(*i != 0) sprintf(buffer, "%i", *i);
   if(scan(msg, buffer, 5)) {
     sscanf(buffer, "%i", i);
     return 1;
   } else return 0;
}

#define MAX_FILES 256
#define SCROLL_FILES (LINES - 3)

struct s_file_list
{
  char dirpath[1024];
  char filename[MAX_FILES][13];
  char fileinfo[MAX_FILES][81];
  char filetype[MAX_FILES];
  char name_entered[13];
  int nfiles;
};

void file_get_directory(struct s_file_list* list)
{
  char filepath[1024];
  DIR *dp;
  struct dirent *ep;     
  FILE *f;
  struct s_pattern_head hh;
  int file_size; char file_size_unit;

  dp = opendir(list->dirpath);
  if (!dp) return;

  list->nfiles = 0;

  while (ep = readdir(dp)) {
    if(ep->d_attr & 0x10) {
      strcpy(list->filename[list->nfiles], ep->d_name);
      sprintf(list->fileinfo[list->nfiles], STRING_FILES_DIR,
        ep->d_name,
        (ep->d_date >> 9) + 1980, (ep->d_date >> 5) & 0xf, ep->d_date & 0x1f,
        ep->d_time >> 11, (ep->d_time >> 5) & 0x3f, (ep->d_time & 0x1f) << 1
      );
      list->filetype[list->nfiles] = 1;
      ++list->nfiles;
    } else {
      sprintf(filepath, "%s/%s", list->dirpath, ep->d_name);
      f = fopen(filepath, "rb");
      fread(&hh, 1, sizeof(struct s_pattern_head), f);
      fclose(f);
      if(hh.magic != TT_MAGIC) continue;
      if(hh.version != TT_VERSION) continue;
 
      file_size = ep->d_size; file_size_unit = 'b';
      if(file_size > 999) { file_size = (file_size + 511) >> 10; file_size_unit = 'k'; }
      if(file_size > 999) { file_size = (file_size + 511) >> 10; file_size_unit = 'M'; }
      if(file_size > 999) { file_size = (file_size + 511) >> 10; file_size_unit = 'G'; }
      strcpy(list->filename[list->nfiles], ep->d_name);
      sprintf(list->fileinfo[list->nfiles], STRING_FILES_FILE, 
        ep->d_name,
        (ep->d_date >> 9) + 1980, (ep->d_date >> 5) & 0xf, ep->d_date & 0x1f,
        ep->d_time >> 11, (ep->d_time >> 5) & 0x3f, (ep->d_time & 0x1f) << 1,
        file_size, file_size_unit, hh.file_version, hh.length, hh.insts, hh.tempo
      );
      list->filetype[list->nfiles] = 0;
      ++list->nfiles;
    }
    if(list->nfiles == MAX_FILES) break;
  }

  closedir(dp);
}

void file_up_directory(char* filepath)
{
  int k;
  for(k = strlen(filepath) - 2; k >= 0; --k) {
    if(filepath[k] == '\\') {
      filepath[k] = 0;
      break;
    }
  }
}

int file_dialog(char* filepath)
{
  struct s_file_list list;
  char aa;
  int i = 0, k, scroll = 0, current = 0;
  unsigned int attr;

  if(0 == _dos_getfileattr(filepath, &attr)) {
    if(!(attr & 0x10)) {
      file_up_directory(filepath);
      strcpy(list.name_entered, filepath + strlen(filepath) + 1);
    } else {
      return 0;
    }
  } else {
    getcwd(filepath, 1024);
    strcpy(list.name_entered, ".");
  }

  strcat(filepath, "\\");
  strcpy(list.dirpath, filepath);
  file_get_directory(&list);

  for(i = 0; i < list.nfiles; ++i) {
    if(0 == _stricmp(list.filename[i], list.name_entered)) current = i;
  }

  for(;;) {
    text_strcpy(0, 0, STRING_FILES_HEAD, 0x25);
    for(i = 0; i < SCROLL_FILES; ++i) {
      if(i + scroll < list.nfiles) {
        text_strcpy(0, i + 1, list.fileinfo[i + scroll], 
          i + scroll == current ? 0x70 : list.filetype[i + scroll] ? 0x13 : 0x15);
      } else {
        text_strcpy(0, i + 1, STRING_FILES_ROW, 
          i + scroll == current ? 0x70 : 0x13);
      }
    }

    for(i = 0; i < 80; ++i) { 
      text[(LINES - 2) * 160 + i * 2] = ' ';
      text[(LINES - 2) * 160 + i * 2 + 1] = 0x25;
    }
    text_strcpy(0, LINES - 2, list.dirpath, 0x25);  
    text_strcpy(strlen(list.dirpath), LINES - 2, list.name_entered, 0x25);  

    update_text();

    for(;;) {
      if(key_event_index > 0) {
        i = handle_key_event();
        if(i & 0x200) {
          i &= 0x1ff;
          if(i == SCAN_ESC) return 0;
          if(i == SCAN_UP_ARROW && current > 0) {
            current--;
            if(scroll > current) scroll--;
            strcpy(list.name_entered, list.filename[current]);
            break;
          }
          if(i == SCAN_DOWN_ARROW && current < list.nfiles - 1) {
            current++;
            if(current > scroll + SCROLL_FILES - 1) scroll++;
            strcpy(list.name_entered, list.filename[current]);
            break;
          }
          if(i == SCAN_ENTER) {
            strcpy(filepath, list.dirpath);
            if(list.name_entered[0] == '.' && 
               list.name_entered[1] == 0) {
              filepath[strlen(filepath) - 1] = 0;
            }
            else if(list.name_entered[0] == '.' &&
                    list.name_entered[1] == '.' && 
                    list.name_entered[2] == 0) {
              file_up_directory(filepath);
            }
            else {
              strcat(filepath, list.name_entered);
            }

	    if(0 == _dos_getfileattr(filepath, &attr)) {
              if(attr & 0x10) {
                strcat(filepath, "\\");
                strcpy(list.dirpath, filepath);
                current = 0; scroll = 0;
                file_get_directory(&list);
                strcpy(list.name_entered, list.filename[current]);
              } else return 1;
            } else return 2;

            break;
          }
          k = strlen(list.name_entered);
          if(i == SCAN_BACKSPACE && k > 0) {
            list.name_entered[k - 1] = 0;
            break;
          }
          aa = enter_char(i);
          if(aa && k < 12) { 
            list.name_entered[k] = aa;
            list.name_entered[k + 1] = 0;
            break; 
          }
        }
      }
    }
  }
}

void help_dialog()
{
  int current = 0, i;

  for(;;) {
    memset(text, 0, 160 * (LINES - 1));
    for(i = 0; i < LINES - 1; ++ i) text_strcpy(0, i, STRING_HELP[i + current], 0x06);
    update_text();

    for(;;) {
      if(key_event_index > 0) {
        i = handle_key_event();
        if(i & 0x200) {
          i &= 0x1ff;
          if(i == SCAN_ESC) return;
	  if(i == SCAN_UP_ARROW && current > 0) { current--; break; }
	  if(i == SCAN_DOWN_ARROW && current < STRING_HELP_LINES - LINES + 1) { current++; break; }
        }
      }
    }
  }
}

void historize()
{
  char filename[32];
  if(!song_change) return;
  sprintf(filename, "save/%x.tt", time(NULL));
  pattern_save(&edit->pt, filename);
}

void handle_save()
{
  char filepath[1024];
  strcpy(filepath, edit->nav.filepath);
  if (0 < file_dialog(filepath)) {
    edit->pt.head.file_version++;
    historize();
    pattern_save(&edit->pt, filepath);
    strcpy(edit->nav.filepath, filepath);
    song_change[current_page] = 0;
  }
}

void handle_load()
{
  char filepath[1024];
  strcpy(filepath, edit->nav.filepath);
  if (1 == file_dialog(filepath)) {
    historize();
    pattern_load(&edit->pt, filepath);
    timer_resolution(edit->pt.head.tempo);
    strcpy(edit->nav.filepath, filepath);
    song_change[current_page] = 0;
  }
}

void copy_row(struct s_row* tgt, const struct s_row* src)
{
  int i;
  for(i = 0; i < sizeof(struct s_row); ++i) set_char(((char*)tgt) + i, ((char*)src)[i]);
}

void key_down(int scan_code) 
{
  struct s_pattern* pt = &edit->pt;
  struct s_select* sel = get_mode_select(cursor_mode);

  int i, j;
  int last_loop = 0, next_loop = TT_PATTERN_ROWS - 1;

  if(cursor_mode == 0) {
    for(i = edit->nav.pattern.cursor.yy; i >= 0; --i)
      if(pt->row[i].loop) { last_loop = i; break; }
    for(i = edit->nav.pattern.cursor.yy; i < TT_PATTERN_ROWS; ++i)
      if(pt->row[i].loop) { next_loop = i; break; }
  } else {
    last_loop = edit->nav.inst.cursor.yy & ~3;
    next_loop = last_loop + 4;
  }

  if(!key_states[SCAN_LEFT_ALT] && 
     !key_states[SCAN_LEFT_CONTROL] && 
     !key_states[SCAN_LEFT_SHIFT]) {

    switch(scan_code) {
      case SCAN_F1:
        help_dialog();
        break;

      case SCAN_F2: 
        handle_save();
        break;
      case SCAN_F3:
        handle_load();
        break;

      case SCAN_KP_PERIOD: case SCAN_F7:
        record_mode ^= 1;
        break;
      case SCAN_KP_8: 
        record_mode ^= 0x40; 
        break;
      case SCAN_KP_9: 
        record_mode ^= 0x80; 
        break;

      case SCAN_F5:
        i = PLAY_PLAY | PLAY_ADVANCE;
        set_play_mode(play_mode == i ? 0 : i); 
        break;
      case SCAN_F6: 
      case SCAN_KP_ENTER:
        i = PLAY_PLAY | PLAY_ADVANCE | PLAY_LOOP;
        set_play_mode(play_mode == i ? 0 : i); 
        break;
      case SCAN_RIGHT_ALT:
        set_play_mode(PLAY_PLAY);
        break;
      case SCAN_SPACE:
        set_play_mode(PLAY_PLAY | PLAY_ADVANCE | PLAY_SINGLE_STEP | PLAY_LOOP);
        break;
      case SCAN_RIGHT_SHIFT:
        if(play_mode & PLAY_TAP) {
          cursor_and_mark_advance(sel, 0, 0);
          note_tick = 0;
        } else {
          set_play_mode(PLAY_PLAY | PLAY_ADVANCE | PLAY_SINGLE_STEP | PLAY_LOOP | PLAY_TAP);
        }
        break;
      case SCAN_F8: 
      case SCAN_KP_0:
        opl_stop();
        set_play_mode(0);
        break;

      case SCAN_ENTER:
	current_page ^= 1;
        if(current_page == 0) edit = &edit_aa; else edit = &edit_bb;
        set_palette();
        timer_resolution(edit->pt.head.tempo);
        break;
      case SCAN_TAB:
        cursor_mode ^= 1;
        if(cursor_mode == 1) show_inst = 1;
        break;
      case SCAN_RIGHT_CONTROL:
        delta_mode ^= 1;
        break;

      case SCAN_LEFT_BRACE:
        if(octave > 0) octave--;
        break;
      case SCAN_RIGHT_BRACE:
        if(octave < 7) octave++;
        break; 

      case SCAN_KP_FORWARD_SLASH:
        if(pt->head.tempo > 60) set_short(&pt->head.tempo, pt->head.tempo - 1);
        timer_resolution(pt->head.tempo);
        break;
      case SCAN_KP_STAR:
        if(pt->head.tempo < 480) set_short(&pt->head.tempo, pt->head.tempo + 1);
        timer_resolution(pt->head.tempo);
        break;

      case SCAN_INSERT:
        j = 0; 
        if (scan_int(STRING_INSERT_LINES, &j)) {
          for(i = TT_PATTERN_ROWS - 1; i > edit->nav.pattern.cursor.yy + j - 1; --i) 
            copy_row(pt->row + i, pt->row + i - j);
          for(; i > edit->nav.pattern.cursor.yy; --i) 
            copy_row(pt->row + i, pt->row + edit->nav.pattern.cursor.yy);
        }
        break;
      case SCAN_DELETE:
        j = 0; 
        if (scan_int(STRING_DELETE_LINES, &j)) {
          for(i = edit->nav.pattern.cursor.yy; i < TT_PATTERN_ROWS - j; ++i) 
            copy_row(pt->row + i, pt->row + i + j);
        }
        break;
    }

    if(record_mode == 0) {
      key_enter(scan_code);
    } else {
      record_key(scan_code, 1);
    }
  }

  if(!key_states[SCAN_LEFT_ALT] && 
     !key_states[SCAN_LEFT_CONTROL] && 
     key_states[SCAN_LEFT_SHIFT]) {

    switch(scan_code) {
      case SCAN_KP_PERIOD: record_mode ^= 0x3e; break;
      case SCAN_KP_1: record_mode ^= 0x2; break;
      case SCAN_KP_2: record_mode ^= 0x4; break;
      case SCAN_KP_3: record_mode ^= 0x8; break;
      case SCAN_KP_4: record_mode ^= 0x10; break;
      case SCAN_KP_5: record_mode ^= 0x20; break;
      case SCAN_TAB:
        cursor_mode = 0; show_inst = 0;
        break;
    }
  }

  if(!key_states[SCAN_LEFT_ALT] && 
     key_states[SCAN_LEFT_CONTROL] && 
     !key_states[SCAN_LEFT_SHIFT]) {

    switch(scan_code) {
      case SCAN_F4:
        historize();
        pattern_nuke(&edit->pt);
        break;

      case SCAN_C:
        edit_cc.nav = edit->nav; 
        copy(&edit_cc, edit);
        break;
      case SCAN_V:
        copy(edit, &edit_cc);
        break;

      case SCAN_Z:
        undo();
        break;
      case SCAN_Y:
        redo();
        break;

      case SCAN_1: channel_on ^= 0x1; break;
      case SCAN_2: channel_on ^= 0x2; break;
      case SCAN_3: channel_on ^= 0x4; break;
      case SCAN_4: channel_on ^= 0x8; break;
      case SCAN_5: channel_on ^= 0x10; break;
      case SCAN_6: channel_on ^= 0x20; break;

      case SCAN_KP_1: drums_on ^= 0x2; break;
      case SCAN_KP_2: drums_on ^= 0x4; break;
      case SCAN_KP_3: drums_on ^= 0x8; break;
      case SCAN_KP_4: drums_on ^= 0x10; break;
      case SCAN_KP_5: drums_on ^= 0x20; break;

      case SCAN_F8: 
      case SCAN_KP_0:
        opl_init();
        set_play_mode(0);
        break;

      case SCAN_T:
        i = pt->head.tempo; 
        if(scan_int(STRING_SET_TEMPO, &i)) {
          set_short(&pt->head.tempo, bounds2(i, 60, 480));
          timer_resolution(pt->head.tempo);
        }
        break;

      case SCAN_L:
        scan_int(STRING_GOTO_LINE, &edit->nav.pattern.scroll);
        break;

      case SCAN_M:
        scan_int(STRING_SET_MASTER_KEY_OFFSET, &master_key_offset);
        break;

    }
  }

  if(key_states[SCAN_LEFT_ALT] && 
     !key_states[SCAN_LEFT_CONTROL] && 
     !key_states[SCAN_LEFT_SHIFT]) {
    switch(scan_code) {
      case SCAN_Z:
        sel->mark = sel->cursor;
        sel->end = sel->cursor;
        break;
      case SCAN_X:
        if(sel->mark.xx != 0 || sel->end.xx != sel->max.xx - 1) {
          sel->mark.xx = 0; sel->end.xx = sel->max.xx - 1;
        } else {
          sel->mark.xx = sel->cursor.xx; sel->end.xx = sel->cursor.xx;
        }
        break;
      case SCAN_C:
        if(sel->mark.yy != last_loop || sel->end.yy != next_loop - 1) {
          sel->mark.yy = last_loop; sel->end.yy = next_loop - 1;
        } else {
          sel->mark.yy = sel->cursor.yy; sel->end.yy = sel->cursor.yy;
        }
        break;
    }
  }

  switch(scan_code) {
   case SCAN_LEFT_ARROW: 
     if (sel->cursor.xx > 0) sel->cursor.xx--;
     if (!key_states[SCAN_LEFT_SHIFT] && sel->mark.xx > 0) sel->mark.xx--;
     if (!key_states[SCAN_LEFT_CONTROL] && sel->end.xx > sel->mark.xx) sel->end.xx--;
     break;
   case SCAN_RIGHT_ARROW: 
     if (sel->cursor.xx < sel->max.xx - 1) sel->cursor.xx++;
     if (!key_states[SCAN_LEFT_CONTROL] && sel->end.xx < sel->max.xx - 1) sel->end.xx++;
     if (!key_states[SCAN_LEFT_SHIFT] && sel->mark.xx < sel->end.xx) sel->mark.xx++;
     break;
   case SCAN_UP_ARROW:
     cursor_and_mark_rewind(sel, !key_states[SCAN_LEFT_SHIFT], !key_states[SCAN_LEFT_CONTROL]);
     break;
   case SCAN_DOWN_ARROW:  
     cursor_and_mark_advance(sel, !key_states[SCAN_LEFT_SHIFT], !key_states[SCAN_LEFT_CONTROL]);
     break;
   case SCAN_PAGE_UP:
     for(i = 0; i < 16; ++i)
       cursor_and_mark_rewind(sel, !key_states[SCAN_LEFT_SHIFT], !key_states[SCAN_LEFT_CONTROL]);
     break;
   case SCAN_PAGE_DOWN:
     for(i = 0; i < 16; ++i)
       cursor_and_mark_advance(sel, !key_states[SCAN_LEFT_SHIFT], !key_states[SCAN_LEFT_CONTROL]);
     break;
  }

  undo_step();

  key_states[scan_code] = 1;
  redraw = 1;
}

void key_up(int scan_code)
{
  char aa = enter_note(scan_code, 0);

  if(record_mode == 0) {
    if((scan_code == SCAN_RIGHT_ALT || aa) && !(play_mode & PLAY_ADVANCE)) {
      opl_stop();
      set_play_mode(0);
    }
  } else {
    record_key(scan_code, 0);
  }

  key_states[scan_code] = 0;  
}

void handle_mouse_event()
{
  static struct s_mouse prev_mouse;
  static struct s_pos prev_mouse_pos; 

  struct s_select *sel;
  struct s_pos screen = get_mouse_screen_pos();
  struct s_pos mouse_pos;

  int xx_ofs, yy_ofs;

  mouse.event = 0;

  if((mouse.bx & 1) && !(prev_mouse.bx & 1)) cursor_mode = get_mouse_mode(screen);

  mouse_pos = get_mouse_pos(screen, cursor_mode);
  sel = get_mode_select(cursor_mode);

  xx_ofs = mouse_pos.xx - prev_mouse_pos.xx;
  yy_ofs = mouse_pos.yy - prev_mouse_pos.yy;

  if(!key_states[SCAN_LEFT_SHIFT]) {
    
    if((mouse.bx & 1) && !(prev_mouse.bx & 1) && !key_states[SCAN_LEFT_CONTROL]) {
      sel->mark = mouse_pos;
      sel->end = mouse_pos;
    }

    if((mouse.bx & 1) && !(prev_mouse.bx & 1)) {
      sel->cursor = mouse_pos;
    }

    if((mouse.bx & 1) && !key_states[SCAN_LEFT_CONTROL]) {
      if(mouse_pos.xx <= sel->cursor.xx) sel->mark.xx = mouse_pos.xx;
      if(mouse_pos.xx >= sel->cursor.xx) sel->end.xx = mouse_pos.xx;
      if(mouse_pos.yy <= sel->cursor.yy) sel->mark.yy = mouse_pos.yy;
      if(mouse_pos.yy >= sel->cursor.yy) sel->end.yy = mouse_pos.yy;
    }

  } else {

    if((mouse.bx & 1)) {

      sel->cursor.xx = bounds(sel->cursor.xx + xx_ofs, sel->max.xx - 1);
      sel->mark.xx = bounds(sel->mark.xx + xx_ofs, sel->max.xx - 1);
      sel->end.xx = bounds(sel->end.xx + xx_ofs, sel->max.xx - 1);

      sel->cursor.yy = bounds(sel->cursor.yy + yy_ofs, sel->max.yy - 1);
      sel->mark.yy = bounds(sel->mark.yy + yy_ofs, sel->max.yy - 1);
      sel->end.yy = bounds(sel->end.yy + yy_ofs, sel->max.yy - 1);
    }
  }

  redraw = 1;

  prev_mouse_pos = mouse_pos;
  prev_mouse = mouse;
}

void record(int tick)
{
  const int drum_keys[5] = { SCAN_KP_1, SCAN_KP_2, SCAN_KP_3, SCAN_KP_4, SCAN_KP_5 };

  int i, tick8; 
  char aa;
  struct s_pattern* pt = &edit->pt;
  struct s_row* row = &pt->row[edit->nav.pattern.cursor.yy];
  struct s_channel* channel;

  if(row->ticks == 0) return;

  tick8 = tick * 8 / row->ticks;

  channel = &row->ch[column_channel(edit->nav.pattern.cursor.xx)];

  if(record_mode & 1) {
    set_char(&channel->note, record_note);

    if(record_key_on) set_char(&channel->key_on, channel->key_on | (1 << tick8));
    else set_char(&channel->key_on, channel->key_on & ~(1 << tick8));
  }

  if(record_mode & 0x40) {
    set_char(&channel->control, ((mouse.cx * 16 / 640) << 4) | (channel->control & 0xf));
  }

  if(record_mode & 0x80) {
    set_char(&channel->control, (channel->control & 0xf0) | (mouse.dx * 16 / 200));
  }

  for(i = 0; i < 5; ++i) if(record_mode & (2 << i)) {
    if(key_states[drum_keys[i]]) set_char(row->drums + i, row->drums[i] | (1 << tick8));
    else set_char(row->drums + i, row->drums[i] & ~(1 << tick8));
  }

  redraw = 1;
}

void play()
{
  struct s_pattern* pt = &edit->pt;
  struct s_select *sel = &edit->nav.pattern;
  struct s_row *row = pt->row + sel->cursor.yy;
  
  if(play_mode & PLAY_TAP) row->ticks = note_tick + 2;

  if(row->ticks == 0) return;

  opl_play_row(pt->inst, row, note_tick, channel_on, drums_on);

  if(play_mode & PLAY_ADVANCE) {
    note_tick++;
    if(note_tick >= row->ticks) {
      note_tick = 0;
      cursor_and_mark_advance(sel, 1, 1);
      row = pt->row + sel->cursor.yy;
      if(((play_mode & PLAY_LOOP) && row->loop) || sel->cursor.yy == TT_PATTERN_ROWS ) do {
        cursor_and_mark_rewind(sel, 1, 1);
        row = pt->row + sel->cursor.yy;
      } while(sel->cursor.yy != 0 && !row->loop);
      if(play_mode & PLAY_SINGLE_STEP) play_mode = 0;
    }
  }

  redraw = 1;
}

void scroll_bounds(struct s_select *ss)
{
  ss->scroll = bounds(ss->scroll, ss->max.yy - ss->rows - 1);
}

void inst_select()
{
  int inst = edit->pt.row[edit->nav.pattern.cursor.yy].ch[column_channel(edit->nav.pattern.cursor.xx)].inst << 2;
  edit->nav.inst.scroll = inst;
  edit->nav.inst.cursor.yy = (edit->nav.inst.cursor.yy & 3) | inst;
  edit->nav.inst.mark.yy = (edit->nav.inst.mark.yy & 3) | inst;
  edit->nav.inst.end.yy = (edit->nav.inst.end.yy & 3) | inst;
}

void nav_init(struct s_nav* nav)
{
  memset(nav, 0, sizeof(struct s_nav));

  nav->pattern.rows = SPLIT;
  nav->pattern.max.xx = PATTERN_COLUMNS;
  nav->pattern.max.yy = TT_PATTERN_ROWS;

  nav->inst.rows = LINES - SPLIT - 2;
  nav->inst.max.xx = INST_COLUMNS;
  nav->inst.max.yy = TT_INSTS;
}

int main(int argc, char *argv[])
{
  int i;


  //set_mode(0x54);
  //set_mode(0x03);

  if(argc > 1) sscanf(argv[1], "%i", &config.lines);
  if(argc > 2) sscanf(argv[2], "%i", &config.port);

  pattern_nuke(&edit_aa.pt); nav_init(&edit_aa.nav);
  pattern_nuke(&edit_bb.pt); nav_init(&edit_bb.nav);
  pattern_nuke(&edit_cc.pt); nav_init(&edit_cc.nav);
  undo_init();

  set_rows(LINES == 50);
  set_palette();
  for(i = 0; i < SYMBOL_COUNT; ++i) set_glyph(symbol_index[i], symbols + i * 16, LINES == 50 ? 8 : 16);


  opl_init();
  timer_init();  
  timer_resolution(edit->pt.head.tempo);
  keyboard_init();
  mouse_init();

  while(!key_states[SCAN_F10]) {

    while(key_event_index > 0) {
      i = handle_key_event();
      if(i & 0x200) key_down(i & 0x1ff); else key_up(i & 0x1ff);
    }
    if(mouse.event) handle_mouse_event();

    while(ticks < milliseconds) {
      ticks++;

      if(record_mode != 0) record(note_tick);
      if(play_mode & PLAY_PLAY) play();

      if((ticks & 0x3) == 0) {
        if(show_inst == 0 && edit->nav.pattern.rows < LINES - 2) {
          edit->nav.pattern.rows++;
          edit->nav.inst.rows--;
          redraw = 1;
        }
        if(show_inst == 1 && edit->nav.pattern.rows > SPLIT) {
          edit->nav.pattern.rows--;
          edit->nav.inst.rows++;
          redraw = 1;
        }

        if((cursor_mode == 0 || (mouse.bx & 2)) && !record_mode) {
          if(mouse.bx && mouse.dx == 0) { 
            if(edit->nav.pattern.scroll > 0) edit->nav.pattern.scroll--; 
            redraw = 1;
          }
          if(mouse.bx && mouse.dx >= 199) {
            edit->nav.pattern.scroll++; 
            redraw = 1;
          }
        }
      }
    }

    if(redraw) {
      scroll_bounds(&edit->nav.pattern);
      scroll_bounds(&edit->nav.inst);
      inst_select();
      redraw = 0;
      pattern_draw();
      draw_mouse();
      update_text();
    }

    for(i = 0; i < 4; ++i) {
      char mm[16];
      sprintf(mm, "%4x", debug_vars[i]);
      text_strcpy(75, i + 20, mm, 0x07);
    }

  }

  historize();

  mouse_kill();
  keyboard_kill();
  timer_kill();
  opl_init();
  set_rows(0);

  return 0;	
}


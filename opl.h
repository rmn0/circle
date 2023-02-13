
#define OPL_PORT (config.port) // 0x388

unsigned char opl_shadow[2][0x100];

unsigned int frequency_table[] = {
  169, 183, 194, 205, 217, 230, 244, 258, 
  274, 290, 307, 326, 345, 365, 387, 410, 
  435, 460, 488, 517, 547, 580, 615, 651, 
  690, 731, 774, 820, 869, 921, 975, 1033,
  1095, 1160, 1229, 1302, 1380, 1462, 1548, 1641,
  1738, 1841, 1951, 2067, 2190, 2320, 2458, 2604,
  2759, 2923, 3097, 3281, 3476, 3683, 3902, 4134,
  4380, 4640, 4916, 5208, 5518, 5846, 6194, 6562,
  6952, 7366, 7804, 8268, 8759, 9280, 9832, 10417,
  11036, 11692, 12388, 13124, 13905, 14731, 15607, 16535,
  17519, 18560, 19664, 20833, 22072, 23385, 24775, 26248,
  27809, 29463, 31215, 33071, 35037, 37121, 39328, 41667
};

void opl_write(
  unsigned char port, 
  unsigned char reg, 
  unsigned char value
)
{
  if(opl_shadow[port ? 1 : 0][reg] != value) {
    opl_shadow[port ? 1 : 0][reg] = value;
    outp(OPL_PORT + (port ? 2 : 0), reg);
    outp(OPL_PORT + (port ? 2 : 0) + 1, value);  
  }
}

void opl_write_mask(
  unsigned char port, 
  unsigned char reg, 
  unsigned char value,
  unsigned char mask
)
{
  opl_write(
    port, 
    reg, 
    (opl_shadow[port ? 1 : 0][reg] & ~mask) | (value & mask)
  );
}

void opl_operator(
  unsigned char port, 
  unsigned char channel, 
  const struct s_op* op
)
{
  opl_write(port, channel + 0x20, op->x20);
  opl_write(port, channel + 0x40, op->x40);
  opl_write(port, channel + 0x60, op->x60);
  opl_write(port, channel + 0x80, op->x80);
  opl_write(port, channel + 0xe0, op->xe0);
}

void opl_drums(char* drums, int tick, char row_ticks, char drums_on)
{
  int i; 
  char aa = 0;

  for(i = 0; i < 5; ++i) {
    unsigned int on = drums[i] & (1 << (tick * 8 / row_ticks));
    if(on) aa |= 1 << i;
  }

  opl_write(0, 0xbd, 0x20 | aa);
}

void opl_stop()  {
  int i;
  for(i = 0; i < 9; ++i) opl_write_mask(0, i + 0xb0, 0, 0x20);
  for(i = 0; i < 9; ++i) opl_write_mask(1, i + 0xb0, 0, 0x20);
  opl_write(0, 0xbd, 0x20);
}

void opl_channel(
  unsigned char port, 
  unsigned char channel, 
  struct s_channel* ch,
  struct s_inst* inst,
  char lr,
  char tick,
  char row_ticks
)
{
  const int porta_map[16] = { 
    0, 0x2, 0x4, 0x8, 
    0x10, 0x18, 0x20, 0x30,
    0x40, 0x50, 0x60, 0x80, 
    0xc0, 0xe0, 0x100, 0x180 };

  static unsigned int freq[18] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0 
  };

  unsigned int target_freq, freq_no;
  unsigned char block;
  unsigned char xa0, xb0;

  unsigned char fchannel = channel + (port ? 9 : 0);

  unsigned int on = ch->key_on & (1 << (tick * 8 / row_ticks));

  target_freq = (frequency_table[bounds(ch->note + master_key_offset, 0x7f)] << 2);
  if(inst->tune[port] > 8) target_freq += (int)(inst->tune[port] - 8) * 0x40;
  if(inst->tune[port] < 8) target_freq -= (int)(8 - inst->tune[port]) * 0x40;

  if(inst->porta[port]) {
    if(target_freq > freq[fchannel]) {
      freq[fchannel] = freq[fchannel] + porta_map[inst->porta[port]];
      if(target_freq < freq[fchannel]) freq[fchannel] = target_freq;
    } else if(target_freq < freq[fchannel]) {
      freq[fchannel] = freq[fchannel] - porta_map[inst->porta[port]];
      if(target_freq > freq[fchannel]) freq[fchannel] = target_freq;
    }
  } else {
    freq[fchannel] = target_freq;
  }

  block = 0; freq_no = freq[fchannel] >> 2;

  while(freq_no >= 0x400) {
    ++block;
    freq_no >>= 1;
  }

  xa0 = freq_no & 0xff;
  xb0 = (block << 2) | (freq_no >> 8);

  opl_write(port, channel + 0xa0, xa0);
  opl_write(port, channel + 0xb0, xb0 | (on ? 0x20 : 0));
  opl_write(port, channel + 0xc0, inst->xc0[port] | lr);
}

#define ROUND_DIVIDE(numer, denom) (((numer) + (denom) / 2) / (denom))

char lerp_mask(char v1, char v2, char v3, char v4, char c, char mask, char shift)
{
  unsigned short iv1 = (v1 & mask) >> shift;
  unsigned short iv2 = (v2 & mask) >> shift;
  unsigned short iv3 = (v3 & mask) >> shift;
  unsigned short iv4 = (v4 & mask) >> shift;
  unsigned short c1 = c & 15;
  unsigned short c2 = c >> 4;

  unsigned short lerp = ROUND_DIVIDE(
    (iv1 * (15 - c1) + iv2 * c1) * (15 - c2) + 
    (iv3 * (15 - c1) + iv4 * c1) * c2,
    15 * 15
  );

  return lerp << shift;
}

#define LERP(reg, mask, shift) lerp_mask(\
  insts[j + 0].reg, insts[j + 1].reg, insts[j + 2].reg, insts[j + 3].reg,\
  cc, mask, shift)

void opl_play_row(struct s_inst *insts, struct s_row *row, char tick, char channel_on, char drums_on) {
  struct s_inst ip_inst[9];

  int operator_map[] = {
    0x00, 0x03, 0x01, 0x04, 0x02, 0x05, 
    0x08, 0x0b, 0x09, 0x0c, 0x0a, 0x0d, 
    0x10, 0x13, 0x11, 0x14, 0x12, 0x15 
  };

  int i, j, k;
  static char prev_cc[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
  char cc;

  for(i = 0; i < 9; ++i) {
    cc = row->ch[i].control;
    if(tick >= row->ticks - 1) prev_cc[i] = cc;
    else cc = 
      ((((int)(cc >> 4) * tick + (int)(prev_cc[i] >> 4) * (row->ticks - tick)) / row->ticks) << 4) |
      ((((int)(cc & 0xf) * tick + (int)(prev_cc[i] & 0xf) * (row->ticks - tick)) / row->ticks) & 0xf);

    j = (int)row->ch[i].inst * 4;
    ip_inst[i].xc0[0] = LERP(xc0[0], 1, 0) | LERP(xc0[0], 0xe, 1);
    ip_inst[i].xc0[1] = LERP(xc0[1], 1, 0) | LERP(xc0[1], 0xe, 1);
    ip_inst[i].porta[0] = LERP(porta[0], 0xf, 0);
    ip_inst[i].porta[1] = LERP(porta[1], 0xf, 0);
    ip_inst[i].tune[0] = LERP(tune[0], 0xf, 0);
    ip_inst[i].tune[1] = LERP(tune[1], 0xf, 0);
    for(k = 0; k < 4; ++k) {
      ip_inst[i].op[k].x20 = 
        LERP(op[k].x20, 0x80, 7) | LERP(op[k].x20, 0x40, 6) |
        LERP(op[k].x20, 0x20, 5) | LERP(op[k].x20, 0x10, 4) |
        LERP(op[k].x20, 0xf, 0);
      ip_inst[i].op[k].x40 = LERP(op[k].x40, 0xc0, 6) | LERP(op[k].x40, 0x3f, 0);
      ip_inst[i].op[k].x60 = LERP(op[k].x60, 0xf0, 4) | LERP(op[k].x60, 0xf, 0);
      ip_inst[i].op[k].x80 = LERP(op[k].x80, 0xf0, 4) | LERP(op[k].x80, 0xf, 0);
      ip_inst[i].op[k].xe0 = LERP(op[k].xe0, 0x7, 0);        
    }
  }

  for(i = 0; i < 9; ++i) {
    opl_operator(0, operator_map[i * 2 + 0], ip_inst[i].op + 0);
    opl_operator(0, operator_map[i * 2 + 1], ip_inst[i].op + 1);
    if(i < 6) {
      opl_operator(1, operator_map[i * 2 + 0], ip_inst[i].op + 2);
      opl_operator(1, operator_map[i * 2 + 1], ip_inst[i].op + 3);
    }
  }

  for(i = 0; i < 6; ++i) if (channel_on & (1 << i)) {
    opl_channel(0, i, row->ch + i, ip_inst + i, 0x10, tick, row->ticks);
    opl_channel(1, i, row->ch + i, ip_inst + i, 0x20, tick, row->ticks);
  }

  for(i = 6; i < 9; ++i) {
    opl_channel(0, i, row->ch + i, ip_inst + i, 0x31, tick, row->ticks);
  }

  opl_drums(row->drums, tick, row->ticks, drums_on);
}

void opl_init()
{
  int i;

  memset(opl_shadow[0], 0xff, 0x100);
  memset(opl_shadow[1], 0xff, 0x100);

  for(i = 0; i < 256; ++i) opl_write(1, 255 - i, 0);
  for(i = 0; i < 256; ++i) opl_write(0, 255 - i, 0);

  opl_write(0, 0x01, 0);
  opl_write(0, 0xbd, 0x20);
  opl_write(1, 0x05, 1);
  opl_write(1, 0x04, 0);
}


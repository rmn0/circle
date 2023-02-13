
#define TT_MAGIC 0x207474 // "tt "
#define TT_VERSION 3

#define TT_PATTERN_ROWS 16384
#define TT_INSTS 1024

struct s_op
{
  unsigned char x20, x40, x60, x80, xe0;
};

struct s_channel
{
  unsigned char key_on, note, inst, control;
};

struct s_inst
{
  struct s_op op[4];
  char xc0[2];
  char porta[2];
  char tune[2];
};


struct s_row {
  struct s_channel ch[9];
  char loop, ticks;
  char drums[5];
  char desc[11];
};

struct s_select {
  struct s_pos cursor, mark, end, max;
  int scroll, rows;
};

struct s_pattern_nav {
  struct s_select pattern, inst;
  char filepath[1024];
  //char pattern_mask[PATTERN_COLUMNS];
  //char inst_mask[INST_COLUMNS];
};

struct s_pattern_head {
  unsigned int magic;
  unsigned short version;
  unsigned short insts;
  unsigned short length;
  unsigned short tempo;
  unsigned short file_version;
  unsigned char reserved[64 - 12];
};

struct s_pattern {
  struct s_pattern_nav nav;
  struct s_pattern_head head;
  struct s_inst inst[TT_INSTS];
  struct s_row row[TT_PATTERN_ROWS];
};

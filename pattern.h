
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
  struct s_pattern_head head;
  struct s_inst inst[TT_INSTS];
  struct s_row row[TT_PATTERN_ROWS];
};

int pattern_length(struct s_pattern* pt)
{
  int i;
  int* data = (int*)pt->row;
  for(i = sizeof(struct s_row) / sizeof(int) * TT_PATTERN_ROWS - 1; i >= 0; --i) {
    if(data[i]) return i * sizeof(int) / sizeof(struct s_row) + 1;
  }
  return 0;
}

int pattern_insts(struct s_pattern* pt)
{
  int i;
  int* data = (int*)pt->inst;
  for(i = sizeof(struct s_inst) / sizeof(int) * TT_INSTS - 1; i >= 0; --i) {
    if(data[i]) return i * sizeof(int) / (sizeof(struct s_inst) * 4) + 1;
  }
  return 0;
}

#define COMPRESSION_ROW_SIZE (sizeof struct s_row)
#define COMPRESSION_ROWS (64)
#define COMPRESSION_ROW_DATA_SIZE (COMPRESSION_ROW_SIZE * COMPRESSION_ROWS)

#define COMPRESSION_INST_SIZE (sizeof struct s_inst)
#define COMPRESSION_INSTS (TT_INSTS)
#define COMPRESSION_INST_DATA_SIZE (COMPRESSION_INST_SIZE * COMPRESSION_INSTS)

#define COMPRESSION_BUFFER_SIZE (LZ4_COMPRESSBOUND(\
  COMPRESSION_ROW_DATA_SIZE > COMPRESSION_INST_DATA_SIZE ?\
  COMPRESSION_ROW_DATA_SIZE : COMPRESSION_INST_DATA_SIZE))

void fwrite_compressed(LZ4_stream_t* const lz4Stream, char* data, int size, FILE* f)
{
  char buffer[COMPRESSION_BUFFER_SIZE];
  unsigned short bb;

  bb = LZ4_compress_fast_continue(lz4Stream, data, buffer, size, COMPRESSION_BUFFER_SIZE, 1);

  fwrite(&bb, 2, 1, f);
  fwrite(buffer, 1, bb, f);
}

void fread_compressed(LZ4_streamDecode_t* const lz4StreamDecode, char* data, int size, FILE* f)
{
  char buffer[COMPRESSION_BUFFER_SIZE];
  unsigned short bb;

  fread(&bb, 2, 1, f);
  fread(buffer, 1, bb, f);

  LZ4_decompress_safe_continue(lz4StreamDecode, buffer, data, bb, size);
}

void pattern_nuke(struct s_pattern* pt)
{
  int i;
  memset(pt, 0, sizeof(struct s_pattern));

  pt->head.magic = TT_MAGIC;
  pt->head.version = TT_VERSION;
  pt->head.tempo = 120;

}

void pattern_save(struct s_pattern* pt, char* filename)
{
  LZ4_stream_t* const lz4Stream = LZ4_createStream();
  int ii;
  FILE *f;

  pt->head.length = pattern_length(pt);
  pt->head.insts = pattern_insts(pt);

  if(pt->head.length == 0 && pt->head.insts == 0) return;

  f = fopen(filename, "wb");

  fwrite(&pt->head, 1, sizeof(struct s_pattern_head), f);

  fwrite_compressed(lz4Stream, (char*)pt->inst, COMPRESSION_INST_DATA_SIZE, f);
  
  for(ii = 0; ii < pt->head.length; ii += COMPRESSION_ROWS) {
    fwrite_compressed(lz4Stream, (char*)(pt->row + ii), COMPRESSION_ROW_DATA_SIZE, f);
  }

  LZ4_freeStream(lz4Stream);
  fclose(f);
}

void pattern_load(struct s_pattern* pt, char* filename)
{
  struct s_pattern_head hh;
  char buffer[COMPRESSION_BUFFER_SIZE];
  LZ4_streamDecode_t* const lz4StreamDecode = LZ4_createStreamDecode();
  int  ii;
  FILE *f;

  f = fopen(filename, "rb");
  if(!f) return;

  fread(&hh, 1, sizeof(struct s_pattern_head), f);
  if(hh.magic != TT_MAGIC) { fclose(f); return; }
  if(hh.version != TT_VERSION) {fclose(f); return; }

  pattern_nuke(pt);
  pt->head = hh;

  fread_compressed(lz4StreamDecode, (char*)pt->inst, COMPRESSION_INST_DATA_SIZE, f);

  for(ii = 0; ii < pt->head.length; ii += COMPRESSION_ROWS) {
    fread_compressed(lz4StreamDecode, (char*)(pt->row + ii), COMPRESSION_ROW_DATA_SIZE, f);
  }

  LZ4_freeStreamDecode(lz4StreamDecode);
  fclose(f);
}


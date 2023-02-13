
#define SYMBOL_COUNT 49

const char *symbol_index = 
  "���������" // 9
  "\xe0\xe1\xe2\xe3\xe4\x10\xe5\xe6" // 8
  "\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8a\x8b\x8c\x8d\x8e\x8f" // 16
  "\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9a\x9b\x9c\x9d\x9e\x9f"; // 16

#define KEY_ON_SEG 0x38, 0x38, 0x38, 0x38
#define KEY_OFF_SEG 0x00, 0x00, 0x00, 0x00

const char symbols[] = { 
  0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, // �
  0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,

  0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, // � (used as key hold)
  0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38,

  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, // �
  0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

  0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0xff, // �
  0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

  0x38, 0x38, 0x38, 0x38, 0x38, 0x7c, 0xfe, 0xff, // �
  0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, // �
  0xff, 0xff, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,

  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, // �
  0xff, 0xff, 0xfe, 0x7c, 0x38, 0x38, 0x38, 0x38,

  0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0xff, // �
  0xff, 0xff, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,

  0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0xff, // � (thin line up) (unused)
  0xff, 0xff, 0xfe, 0x7c, 0x38, 0x38, 0x38, 0x38,

  0x00, 0x10, 0x1c, 0x1e, 0x1f, 0x1f, 0x1f, 0x1f, // \xe0 (play symbol left)
  0x1f, 0x1f, 0x1f, 0x1e, 0x1c, 0x10, 0x00, 0x00,

  0x00, 0x00, 0x00, 0x00, 0x80, 0xc0, 0xf0, 0xf8, // \xe1 (play symbol right)
  0xf0, 0xc0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 

  0x00, 0x00, 0x03, 0x0f, 0x1f, 0x1f, 0x3f, 0x3f, // \xe2 (record symbol left)
  0x3f, 0x3f, 0x1f, 0x1f, 0x0f, 0x03, 0x00, 0x00,

  0x00, 0x00, 0xc0, 0xf0, 0xf8, 0xf8, 0xfc, 0xfc, // \xe3 (record symbol right)
  0xfc, 0xfc, 0xf8, 0xf8, 0xf0, 0xc0, 0x00, 0x00,

  0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, // \xe4 (pane right border)
  0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f,

  0x00, 0x00, 0x00, 0x00, 0x80, 0xc0, 0xe0, 0xf0, // \x10 (used as loop symbol)
  0xe0, 0xc0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00,

  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, // \xe5 (flag off)
  0x14, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x3e, // \xe6 (flag on)
  0x3e, 0x3e, 0x3e, 0x00, 0x00, 0x00, 0x00, 0x00,

  0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x42, 0x42, 0x5a, 0x42, 0x42, 0x3c, // hex 0
  0x00, 0x00, 0x00, 0x00, 

  0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x08, 0x08, 0x08, 0x08, 0x08, 0x1c, // hex 1
  0x00, 0x00, 0x00, 0x00, 

  0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x42, 0x02, 0x3c, 0x40, 0x40, 0x7e, // hex 2
  0x00, 0x00, 0x00, 0x00,  

  0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x42, 0x02, 0x1c, 0x02, 0x42, 0x3c, // hex 3
  0x00, 0x00, 0x00, 0x00,  
  
  0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x14, 0x24, 0x44, 0x7e, 0x04, 0x04, // hex 4
  0x00, 0x00, 0x00, 0x00,  

  0x00, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x40, 0x40, 0x7c, 0x02, 0x42, 0x3c, // hex 5
  0x00, 0x00, 0x00, 0x00, 

  0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x42, 0x40, 0x7c, 0x42, 0x42, 0x3c, // hex 6
  0x00, 0x00, 0x00, 0x00, 

  0x00, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x02, 0x04, 0x08, 0x10, 0x20, 0x20, // hex 7
  0x00, 0x00, 0x00, 0x00, 

  0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x42, 0x42, 0x3c, 0x42, 0x42, 0x3c, // hex 8
  0x00, 0x00, 0x00, 0x00, 

  0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x42, 0x42, 0x3e, 0x02, 0x42, 0x3c, // hex 9
  0x00, 0x00, 0x00, 0x00, 

  0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x42, 0x42, 0x7e, 0x42, 0x42, 0x42, // hex A
  0x00, 0x00, 0x00, 0x00, 

  0x00, 0x00, 0x00, 0x00, 0x00, 0x7c, 0x42, 0x42, 0x7c, 0x42, 0x42, 0x7c, // hex B
  0x00, 0x00, 0x00, 0x00, 

  0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x42, 0x40, 0x40, 0x40, 0x42, 0x3c, // hex C
  0x00, 0x00, 0x00, 0x00, 

  0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0x44, 0x42, 0x42, 0x42, 0x44, 0x78, // hex D
  0x00, 0x00, 0x00, 0x00,

  0x00, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x40, 0x40, 0x7c, 0x40, 0x40, 0x7e, // hex E
  0x00, 0x00, 0x00, 0x00, 

  0x00, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x40, 0x40, 0x7c, 0x40, 0x40, 0x40, // hex F
  0x00, 0x00, 0x00, 0x00,

  KEY_OFF_SEG, KEY_OFF_SEG, KEY_OFF_SEG, KEY_OFF_SEG, // \x90
  KEY_ON_SEG, KEY_OFF_SEG, KEY_OFF_SEG, KEY_OFF_SEG, // \x90
  KEY_OFF_SEG, KEY_ON_SEG, KEY_OFF_SEG, KEY_OFF_SEG, // \x90
  KEY_ON_SEG, KEY_ON_SEG, KEY_OFF_SEG, KEY_OFF_SEG, // \x90

  KEY_OFF_SEG, KEY_OFF_SEG, KEY_ON_SEG, KEY_OFF_SEG, // \x90
  KEY_ON_SEG, KEY_OFF_SEG, KEY_ON_SEG, KEY_OFF_SEG, // \x90
  KEY_OFF_SEG, KEY_ON_SEG, KEY_ON_SEG, KEY_OFF_SEG, // \x90
  KEY_ON_SEG, KEY_ON_SEG, KEY_ON_SEG, KEY_OFF_SEG, // \x90

  KEY_OFF_SEG, KEY_OFF_SEG, KEY_OFF_SEG, KEY_ON_SEG, // \x90
  KEY_ON_SEG, KEY_OFF_SEG, KEY_OFF_SEG, KEY_ON_SEG, // \x90
  KEY_OFF_SEG, KEY_ON_SEG, KEY_OFF_SEG, KEY_ON_SEG, // \x90
  KEY_ON_SEG, KEY_ON_SEG, KEY_OFF_SEG, KEY_ON_SEG, // \x90

  KEY_OFF_SEG, KEY_OFF_SEG, KEY_ON_SEG, KEY_ON_SEG, // \x90
  KEY_ON_SEG, KEY_OFF_SEG, KEY_ON_SEG, KEY_ON_SEG, // \x90
  KEY_OFF_SEG, KEY_ON_SEG, KEY_ON_SEG, KEY_ON_SEG, // \x90
  KEY_ON_SEG, KEY_ON_SEG, KEY_ON_SEG, KEY_ON_SEG, // \x90

};
  
const char* STRING_PATTERN_ROW =
  "   �       �       �       �       �       �       �       �       �            ";


const char* STRING_SEPARATOR =
  "�����������������������������������������������������������������������������";

const char* STRING_INST_ROW =
  "        �    �    �    � �    �    �    � �    �    �    � �    �    �    �  ";

const char* STRING_NOTE_NAMES = "CdDeEFgGaAbB";
const char STRING_HEXA = 0x80;
const char STRING_KEY_ON = 0x90;
const char* STRING_FLAG = "\xe5\xe6";
const char* STRING_SYN_FLAG = "FA";
const char* STRING_LOOP_FLAG = "�\x10";
const char STRING_DELTA = '�';
const char STRING_NOTE_NONE = 0x16;

const char STRING_LIVE = 0x07;
const char STRING_MUTE = 0x09;

const char* STRING_INFO_LOOP = "Loop";
const char* STRING_INFO_TICKS = "Ticks";

const char* STRING_INFO_CHANNEL[] = {
  "Note", "Octave", "Key On", "Instrument", "Control A", "Control B"
};

const char* STRING_INFO_INST[] = {
  "Left Portamento", "Left Tune", "Left Feedback", "Left Synth Type",
  "Right Portamento", "Right Tune", "Right Feedback", "Right Synth Type"
};

const char* STRING_INFO_OPERATOR[] = {
  "Tremolo", "Vibrato", "Sustain", "Envelope Scaling (KSR)",
  "Frequency Modulation Factor", "Key Scale Level", "Level", 
  "Attack", "Decay", "Sustain", "Release"
};

const char* STRING_INFO_WAVE[] = {
  "Sine Wave", "Half Sine Wave", "Abs Sine Wave", "Pulse Sine Wave", 
  "Sine Wave Even", "Abs Sin Wave Even", "Square Wave", "Derived Square Wave"
};

const char* STRING_INFO_PATTERN[] = { 
  "Channel 1", "Channel 2", "Channel 3", "Channel 4", 
  "Channel 5", "Channel 6", "Base drum channel", "HH/CY/SD channel", "HH/CY/TT channel"
};

const char* STRING_INFO_DRUMS[] = {
  "Hi-hat", "Cymbal", "Tomtom", "Snare drum", "Base drum"
};

const char* STRING_INFO_OPERATOR_NAME[] = {
  "Left Modulator", "Left Carrier", "Right Modulator", "Right Carrier"
};

const char* STRING_INFO_DRUM_CHANNEL[] = {
  "Base drum A", "Base drum B",  "Hi-hat", "Snare", "Tomtom", "Cymbal"
};

const char* STRING_EMPTY = "";

const char* STRING_STATUS =
  "Tmp    �Ln    �Oct  �Line    �    �    �    �     " // 50
  "�\x09\x09\x09\x09\x09\x09" // 7
  "�\x09\x09\x09\x09\x09" // 6
  "�\x09\x09\x09\x09\x09�\x09\x09" // 9
  "    \xe0\xe1\xe2\xe3"; // 8

const char* STRING_FILES_HEAD =
  " Filename    �Date      �Time    �Size�Ver�Length�Insts�Tempo ";

const char* STRING_FILES_ROW =
  "             �          �        �    �   �      �     �      ";

const char* STRING_FILES_DIR =
  " %12s�%04i/%02i/%02i�%02i:%02i:%02i�    �   �      �     �      ";

const char* STRING_FILES_FILE =
  " %12s�%04i/%02i/%02i�%02i:%02i:%02i�%3i%c�%3i�%6i�%5i�%5i ";

const char* STRING_SET_TEMPO = "Set tempo: ";
const char* STRING_GOTO_LINE = "Goto line: ";
const char* STRING_SET_MASTER_KEY_OFFSET = "Master key offset: ";
const char* STRING_INSERT_LINES = "Insert lines: ";
const char* STRING_DELETE_LINES = "Delete lines: ";


const int STRING_HELP_LINES = 97;

const char *STRING_HELP[] = {
"HELP",
"",
"  Right mouse ����������������� Describe entry under mouse cursor",
"  F1 �������������������������� This screen",
"",
"FILE",
"",
"  F2 �������������������������� Save current page",
"  F3 �������������������������� Load current page",
"  Ctrl + F4 ������������������� Nuke current page",
"  F10 ������������������������� Exit",
"",
"PLAYBACK / RECORD",
"",
"  F7 or KP Dot ���������������� Toggle record notes",
"  Left Shift + KP1..KP5 ������� Toggle record percussion channel",
"  KP8 and KP9 ����������������� Toggle record mouse into control parameters",
"",
"  F5 �������������������������� Play / pause song from cursor position",
"  F6 or KP Enter �������������� Play / pause loop from cursor position",
"  F8 or KP 0 ������������������ Stop",
"  Ctrl + F8 or KP 0 ����������� Silence",
"",
"  Right Alt ������������������� Play current Row",
"  Space ����������������������� Play current Row and advance (play single step)",
"",
"  Ctrl + 1..6 ����������������� Mute / unmute channel",
"  Ctrl + KP1..KP5 ������������� Mute / unmute percussion channel",
"",
"  Ctrl + M .................... Set master key offset",
"",
"VIEW",
"",
"  Enter ����������������������� Toggle front page (yellow) / back page (blue)",
"  Tab ������������������������� Toggle pattern / instrument cursor",
"  Left Shift + Tab������������� Hide instrument table",
"",
"NAVIGATION / SELECTION",
"",
"  Arrows ���������������������� Move cursor and mark",
"  Left Shift + Arrows ��������� Move cursor and mark end",
"  Left Ctrl + Arrows ���������� Move cursor and mark start",
"  Left Shift + Ctrl + Arrows �� Move cursor only",
"",
"  Page up ��������������������� Move cursor and mark up 16 lines",
"  Page down ������������������� Move cursor and mark down 16 lines",
"",
"  Left mouse click ������������ Set cursor and mark",
"  Left mouse drag ������������� Select region (set mark start and end)",
"  Left Shift + LM drag �������� Move Region (move mark start and end)",
"  Click screen border ��������� Scroll up / down",
"",
"  Alt + Z ��������������������� Reset mark to cursor",
"  Alt + X ��������������������� Toggle select whole line",
"  Alt + C ��������������������� Toggle select whole loop",
"",
"PATTERN EDIT",
"",
"  Right Shift ����������������� Count in beat",
"",
"  Insert ���������������������� Insert line",
"  Delete ���������������������� Delete line",
"  ",
"  Ctrl + C �������������������� Copy mark",
"  Ctrl + V �������������������� Save mark",
"",
"  Ctrl + Z �������������������� Undo",
"  Ctrl + Y �������������������� Redo",
"",
"  KP/ and KP* ����������������� Change Tempo",
"  Ctrl + T .................... Set tempo",
"",
"VALUE EDIT",
"",
"  0..9, A..F ������������������ Enter Value",
"",
"FLAG EDIT",
"",
"  1 or 0 ���������������������� Toggle Flag",
"  2 or = or KP+ ��������������� Set Flag",
"  3 or - or KP- ��������������� Reset Flag",
"",
"NOTE EDIT",
"                                            �������������������������������Ŀ",
"  [ and ] ��������������������� Octave      � �   �   � � �   �   �   � �   �",
"                                            � �   �   � � �   �   �   � �   �",
"  KP1 ������������������������� Hi-hat      � � 2 � 3 � � � 5 � 6 � 7 � �   �",
"  KP2 ������������������������� Cymbal      � � S � F � � � H � J � K � �   �",
"  KP3 ������������������������� Tomtom      � ��������� � ������������� �   �",
"  KP4 ������������������������� Snare drum  � Q � W � E � R � T � Y � U � I �",
"  KP5 ������������������������� Base drum   � Z � X � C � V � B � N � M � , �",
"                                            ���������������������������������",
"KEY ON EDIT",
"",
"  X and Z ��������������������� Set Key On / Off",
"  1..4 ������������������������ Toggle Key On / Off Segments",
"",
""
};

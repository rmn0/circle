
#define KEYBOARD_CONTROLLER_OUTPUT_BUFFER 0x60
#define KEYBOARD_CONTROLLER_STATUS_REGISTER 0x64
#define KEY_PRESSED 1
#define KEY_RELEASED 0
#define PIC_OPERATION_COMMAND_PORT 0x20
#define KEYBOARD_INTERRUPT_VECTOR 0x09

#define PPI_PORT_A 0x60
#define PPI_PORT_B 0x61
#define PPI_PORT_C 0x62
#define PPI_COMMAND_REGISTER 0x63

char key_states[0x200];
volatile short key_event[0x100], key_event_index = 0;

void interrupt (*oldKeyboardIsr)() = (void *)0;

static unsigned char isPreviousCodeExtended = 0;

void interrupt far KeyboardIsr()
{
    static unsigned char scanCode;
    unsigned char ppiPortB;

    _asm {
        cli
    };

    scanCode = 0;
    ppiPortB = 0;

    ppiPortB = inp(PPI_PORT_B); 
    scanCode = inp(KEYBOARD_CONTROLLER_OUTPUT_BUFFER);
    outp(PPI_PORT_B, ppiPortB | 0x80); 
    outp(PPI_PORT_B, ppiPortB);

    switch(scanCode)
    {
    case 0xE0:
        isPreviousCodeExtended = 1;
        break;
    default:
        if(isPreviousCodeExtended)
        {
            isPreviousCodeExtended = 0;
            if(scanCode & 0x80)
            {
                scanCode &= 0x7F;
                key_event[key_event_index] = scanCode | 0x100;
            }
            else
            {
                key_event[key_event_index] = scanCode | 0x300;
            }
        }
        else if(scanCode & 0x80)
        {
            scanCode &= 0x7F;
            key_event[key_event_index] = scanCode;
        }
        else
        {
            key_event[key_event_index] = scanCode | 0x200;
        }
        ++key_event_index;
        break;
    }

    outp(PIC_OPERATION_COMMAND_PORT, 0x20);

    _asm
    {
        sti
    };
}

void keyboard_init()
{
   memset(&key_states[0], 0, 0x200);

   oldKeyboardIsr = _dos_getvect(KEYBOARD_INTERRUPT_VECTOR);
   _dos_setvect(KEYBOARD_INTERRUPT_VECTOR, KeyboardIsr);
}

void keyboard_kill()
{
  _dos_setvect(KEYBOARD_INTERRUPT_VECTOR, oldKeyboardIsr);
  oldKeyboardIsr = (void *)0;
}



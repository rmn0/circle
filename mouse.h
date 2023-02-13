
struct s_mouse {
    int             event;
    unsigned short  ax;
    unsigned short  bx;
    unsigned short  cx;
    unsigned short  dx;
    signed short    si;
    signed short    di;
} mouse = { 0 };

int lock_region( void *address, unsigned length )
{
    union REGS      regs;
    unsigned        linear;

    linear = (unsigned)address;

    regs.w.ax = 0x600;
    
    regs.w.bx = (unsigned short)(linear >> 16);
    regs.w.cx = (unsigned short)(linear & 0xFFFF);
    
    regs.w.si = (unsigned short)(length >> 16);
    regs.w.di = (unsigned short)(length & 0xFFFF);
    int386( 0x31, &regs, &regs );

    return( regs.w.cflag == 0 );
}

#pragma off( check_stack )
void _loadds far mouse_isr( int ax, int bx,
                            int cx, int dx,
                            int si, int di )
{
#pragma aux mouse_isr   __parm [__eax] [__ebx] [__ecx] \
                               [__edx] [__esi] [__edi]
    mouse.event = 1;

    mouse.ax = (unsigned short)ax;
    mouse.bx   = (unsigned short)bx;
    mouse.cx   = (unsigned short)cx;
    mouse.dx   = (unsigned short)dx;
    mouse.si   = (signed short)si;
    mouse.di   = (signed short)di;
}

void cbc_end( void )
{
}
#pragma on( check_stack )

void mouse_init (void)
{
    struct SREGS        sregs;
    union REGS          inregs, outregs;
    unsigned short far  *ptr;
    void (far *function_ptr)();

    segread( &sregs );

    inregs.w.ax = 0;
    int386( 0x33, &inregs, &outregs );

    if(outregs.w.ax != 0xffff) {
      exit(1);
    }

    if( (! lock_region( &mouse, sizeof( mouse ) )) ||
        (! lock_region( (void near *)mouse_isr,
           (char *)cbc_end - (char near *)mouse_isr )) )  {
      exit(1);
    } 

   // inregs.w.ax = 0x1;
   // int386( 0x33, &inregs, &outregs );

   inregs.w.ax  = 0xC;
   inregs.w.cx  = 0x1f;
   function_ptr = ( void (far *)( void ) )mouse_isr;
   inregs.x.edx = FP_OFF( function_ptr );
   sregs.es     = FP_SEG( function_ptr );
   int386x( 0x33, &inregs, &outregs, &sregs );
}

void mouse_kill()
{
  union REGS inregs, outregs;
  inregs.w.ax = 0;
  int386( 0x33, &inregs, &outregs );
}


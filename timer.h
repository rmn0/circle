typedef void (__interrupt __far* INTFUNCPTR)(void);

INTFUNCPTR old_timer_isr; // Original interrupt handler

volatile long int milliseconds = 0; // Elapsed time in milliseconds

int timer_res;

void __interrupt __far timer_isr(void)
{
  static unsigned long count=0; // To keep track of original timer ticks
  ++milliseconds;
  count+=timer_res;
  if(count>=65536) // It is now time to call the original handler
  {
    count-=65536;
    _chain_intr(old_timer_isr);
  }
  else outp(0x20,0x20); // Acknowledge interrupt
}

void timer_init(void)
{
  union REGS r;
  struct SREGS s;
  _disable();
  segread(&s);
  /* Save old interrupt vector: */
  r.h.al=0x08;
  r.h.ah=0x35;
  int386x(0x21,&r,&r,&s);
  old_timer_isr=(INTFUNCPTR)MK_FP(s.es,r.x.ebx);
  /* Install new interrupt handler: */
  milliseconds=0;
  r.h.al=0x08;
  r.h.ah=0x25;
  s.ds=FP_SEG(timer_isr);
  r.x.edx=FP_OFF(timer_isr);
  int386x(0x21,&r,&r,&s);
  _enable();
}

void timer_resolution(int rr)
{
  timer_res = (1193182 * 60) / (rr * 0x80);

  /* Set resolution of timer chip to 1ms: */
  outp(0x43, 0x36);
  outp(0x40, timer_res & 0xff);
  outp(0x40, timer_res >> 8);
}

void timer_kill(void)
{
  union REGS r;
  struct SREGS s;
  _disable();
  segread(&s);
  /* Re-install original interrupt handler: */
  r.h.al=0x08;
  r.h.ah=0x25;
  s.ds=FP_SEG(old_timer_isr);
  r.x.edx=FP_OFF(old_timer_isr);
  int386x(0x21,&r,&r,&s);
  /* Reset timer chip resolution to 18.2...ms: */
  outp(0x43,0x36);
  outp(0x40,0x00);
  outp(0x40,0x00);
  _enable();
}

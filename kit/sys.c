/* sys.c - take over a stock Amiga 500 from AmigaOS (Kickstart 1.3 or later) and give it back.
 * Adapted from the a1200-final kit: no AGA test, no Kickstart 2+ calls, 68000 safe. */
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/dos.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <graphics/gfxbase.h>
#include "sys.h"

extern struct ExecBase *SysBase;
struct GfxBase *GfxBase;

UWORD sys_lines, sys_is_pal;
volatile ULONG sys_frame;
void (*volatile sys_vbl_hook)(void);
volatile UBYTE sys_keys[128];

static struct View *old_view;
static UWORD old_dmacon, old_intena, old_adkcon;
ULONG *sys_vbr;
static ULONG old_lev3;

/* movec vbr,d0 ; rte  - must run in supervisor mode, 68010+ only */
static const UWORD getvbr_code[] = { 0x4e7a, 0x0801, 0x4e73 };

/* Level 3 interrupt: only the vertical blank source is enabled. */
static __attribute__((interrupt)) void lev3(void)
{
    if (custom->intreqr & INTF_VERTB) {
        sys_frame++;
        if (sys_vbl_hook) sys_vbl_hook();
        custom->intreq = INTF_VERTB;
        custom->intreq = INTF_VERTB;    /* twice: needed on 68040/060 */
    }
}

struct Block { struct Block *next; ULONG size; };
static struct Block *blocks;

void *sys_alloc(ULONG size, ULONG memflags)
{
    struct Block *b = AllocMem(size + sizeof *b, memflags);
    if (!b) return 0;
    b->next = blocks; b->size = size + sizeof *b; blocks = b;
    return b + 1;
}

void *sys_chip_copy(const void *src, ULONG size)
{
    void *p = sys_alloc(size, MEMF_CHIP);
    if (p) CopyMem((APTR)src, p, size);
    return p;
}

void sys_free_all(void)
{
    while (blocks) { struct Block *b = blocks; blocks = b->next; FreeMem(b, b->size); }
}

void *sys_load(const char *name, ULONG memflags, ULONG *size)
{
    BPTR fh = Open((STRPTR)name, MODE_OLDFILE);
    LONG len;
    void *p = 0;
    if (!fh) return 0;
    Seek(fh, 0, OFFSET_END);
    len = Seek(fh, 0, OFFSET_BEGINNING);
    if (len > 0 && (p = sys_alloc(len, memflags)) && Read(fh, p, len) != len) p = 0;
    Close(fh);
    if (size) *size = len;
    return p;
}

/* Stop the floppy motors by hand: nobody can do it once the interrupts are off. */
static void floppy_motors_off(void)
{
    ciab->ciaprb |= 0x80;               /* /MTR high = off ... */
    ciab->ciaprb &= ~0x78;              /* ... latched by each drive when its select line goes low */
    ciab->ciaprb |= 0x78;
}

int sys_take(void)
{
    GfxBase = (struct GfxBase *)OpenLibrary((STRPTR)"graphics.library", 0);
    if (!GfxBase) return 0;

    sys_is_pal = (GfxBase->DisplayFlags & PAL) ? 1 : 0;
    sys_lines  = sys_is_pal ? 313 : 263;

    Delay(25);                          /* half a second: the key that started us comes up, the disk settles */
    old_view = GfxBase->ActiView;
    OwnBlitter();
    WaitBlit();
    LoadView(NULL);
    WaitTOF();
    WaitTOF();
    Forbid();
    floppy_motors_off();

    old_adkcon = custom->adkconr;
    old_dmacon = custom->dmaconr;
    old_intena = custom->intenar;
    custom->intena = 0x7fff;
    custom->intreq = 0x7fff;
    custom->dmacon = 0x7fff;
    custom->dmacon = DMAF_SETCLR | DMAF_MASTER;
    custom->color[0] = 0;

    sys_vbr = 0;
    if (SysBase->AttnFlags & AFF_68010)
        sys_vbr = (ULONG *)Supervisor((ULONG (*)())getvbr_code);
    old_lev3 = sys_vbr[0x6c / 4];
    sys_vbr[0x6c / 4] = (ULONG)lev3;

    sys_frame = 0;
    custom->intena = INTF_SETCLR | INTF_INTEN | INTF_VERTB;
    return 1;
}

void sys_give(void)
{
    sys_wait_frame();
    custom->intena = 0x7fff;
    custom->intreq = 0x7fff;
    custom->dmacon = 0x7fff;
    sys_vbr[0x6c / 4] = old_lev3;

    custom->cop1lc = (ULONG)GfxBase->copinit;
    custom->copjmp1 = 0;
    custom->adkcon = 0x7fff;
    custom->adkcon = old_adkcon | 0x8000;
    custom->dmacon = old_dmacon | 0x8000;
    custom->intena = old_intena | 0x8000;

    LoadView(old_view);
    WaitTOF();
    WaitTOF();
    WaitBlit();
    DisownBlitter();
    Permit();
    CloseLibrary((struct Library *)GfxBase);
}

void sys_wait_frame(void)
{
    ULONG f = sys_frame;
    while (sys_frame == f) ;
}

UWORD sys_joy(int port)
{
    UWORD d = port ? custom->joy1dat : custom->joy0dat;
    UWORD r = 0;
    if (d & 0x0002) r |= JOY_RIGHT;
    if (d & 0x0200) r |= JOY_LEFT;
    if ((d ^ (d >> 1)) & 0x0001) r |= JOY_DOWN;
    if ((d ^ (d >> 1)) & 0x0100) r |= JOY_UP;
    if (!(ciaa->ciapra & (port ? 0x80 : 0x40))) r |= JOY_FIRE;
    return r;
}

int sys_mouse_button(void)
{
    return !(ciaa->ciapra & 0x40);
}

/* Keyboard without an interrupt: poll the CIA-A serial flag once per frame. */
void sys_key_poll(void)
{
    UBYTE k, line;
    int i;
    if (!(ciaa->ciaicr & CIAICRF_SP)) return;       /* reading ICR clears it */
    k = ~ciaa->ciasdr;
    k = (UBYTE)((k >> 1) | (k << 7));               /* bit 7 = key up, bits 0-6 = raw code */
    ciaa->ciacra |= CIACRAF_SPMODE;                 /* handshake: pull KDAT low ... */
    for (i = 0; i < 3; i++) {                       /* ... for at least 85 us = 3 scanlines */
        line = custom->vhposr >> 8;
        while ((UBYTE)(custom->vhposr >> 8) == line) ;
    }
    ciaa->ciacra &= ~CIACRAF_SPMODE;
    sys_keys[k & 0x7f] = (k & 0x80) ? 0 : 1;
}

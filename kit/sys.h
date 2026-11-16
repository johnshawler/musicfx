/* sys.h - take over a stock Amiga 500 (68000, OCS, Kickstart 1.3 or later) from AmigaOS and give it back.
 * The same calls as the a1200-final kit, without the AGA parts. Works on any Amiga: nothing here needs AGA.
 * Build: m68k-amigaos-gcc -m68000 -O2 -fomit-frame-pointer -mcrt=nix13
 */
#ifndef SYS_H
#define SYS_H

#include <exec/types.h>
#include <hardware/custom.h>
#include <hardware/cia.h>
#include <hardware/dmabits.h>
#include <hardware/intbits.h>

#define custom ((volatile struct Custom *)0xdff000)
#define ciaa   ((volatile struct CIA *)0xbfe001)
#define ciab   ((volatile struct CIA *)0xbfd000)

#define SYS_CCK_PER_LINE 227            /* colour clocks (bus slots) per scanline, PAL */
extern UWORD sys_lines;                 /* scanlines per frame: 313 PAL, 263 NTSC */
extern UWORD sys_is_pal;                /* 1 = 50 Hz, 0 = 60 Hz */

extern ULONG *sys_vbr;                  /* CPU vector base (0 on a 68000), valid after sys_take() */
extern volatile ULONG sys_frame;        /* +1 at every vertical blank */
extern void (*volatile sys_vbl_hook)(void); /* optional: called from the interrupt each frame */

void *sys_load(const char *name, ULONG memflags, ULONG *size);   /* read a whole file; BEFORE sys_take() only */
void *sys_alloc(ULONG size, ULONG memflags);    /* AllocMem that sys_free_all() gives back; BEFORE sys_take() only */
void *sys_chip_copy(const void *src, ULONG size);   /* chip RAM copy of a block; BEFORE sys_take() only */
void  sys_free_all(void);                       /* call last, after sys_give() */
int   sys_take(void);                   /* 1 = machine is yours; 0 = failed, nothing changed */
void  sys_give(void);                   /* restore everything sys_take() changed */

void  sys_wait_frame(void);             /* sleep until the next vertical blank */

#define JOY_RIGHT 1
#define JOY_LEFT  2
#define JOY_DOWN  4
#define JOY_UP    8
#define JOY_FIRE  16
UWORD sys_joy(int port);                /* port 1 = joystick port (the Amiga's port 2), port 0 = mouse port */
int   sys_mouse_button(void);           /* left mouse button, 1 = down */

extern volatile UBYTE sys_keys[128];    /* 1 = key is down, indexed by raw key code */
void  sys_key_poll(void);               /* set sys_vbl_hook = sys_key_poll; after sys_take() */
#define KEY_ESC   0x45
#define KEY_SPACE 0x40
#define KEY_RET   0x44
#define KEY_F1    0x50                  /* F1..F10 = 0x50..0x59 */
#define KEY_1     0x01                  /* 1..9 = 0x01..0x09, 0 = 0x0a */

#endif

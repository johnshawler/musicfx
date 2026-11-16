/* mfx.h - music and sound effects together, with neither cutting into the other.
 *
 * Three Paula channels play the module (Frank Wille's ptplayer); the fourth belongs to the effects mixer
 * (Jeroen Knoester's Audio Mixer), which adds up to four effects into that one channel. The music never loses
 * a voice to an effect, and an effect is never cut off by another until more than four sound at once.
 * Which channel, how many voices, the mixer's rate and the quality mode are set in mixer_config.i.
 *
 * Order of calls:  mfx_open(); sys_take(); mfx_start(module); ... mfx_play(&sound) ...; mfx_stop(); sys_give();
 */
#ifndef MFX_H
#define MFX_H
#include <exec/types.h>

typedef struct {
    void  *data;        /* signed 8-bit samples at the mixer's rate, made by tools/sfx.py; any kind of RAM */
    ULONG  length;      /* in bytes, a multiple of 4 */
    UWORD  priority;    /* when every voice is busy, an effect takes the least important voice that is not
                           above it (of equals: the one nearest its end); if all are above it, it is refused */
} MfxSound;

int   mfx_open(void);                   /* BEFORE sys_take(): the mixer's chip RAM buffer. 1 = ok */
int   mfx_start(void *module);          /* AFTER sys_take(). module: a ProTracker module in chip RAM, or 0 for
                                           effects only. 0 = the module is not in chip RAM: effects only */
void  mfx_music(int on);                /* pause (and mute) the module, or continue it */
void  mfx_music_volume(UWORD volume);   /* 0-64, any time: the balance between music and effects */
int   mfx_play(const MfxSound *sound);  /* 1 = it plays, 0 = refused: every voice holds a higher priority */
                                        /* call it from the main program, not from an interrupt */
int   mfx_voices_busy(void);            /* effects sounding now */
void  mfx_stop(void);                   /* BEFORE sys_give() */

extern ULONG mfx_requested, mfx_played, mfx_refused;    /* counters for the bench */
extern UWORD mfx_cfg_channels, mfx_cfg_voices, mfx_cfg_period, mfx_cfg_hq;     /* from mixer_config.i */
#endif

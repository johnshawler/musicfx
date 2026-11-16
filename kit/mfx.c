/* mfx.c - see mfx.h. Glue between ptplayer (music, three channels) and the Audio Mixer (effects, one channel). */
#include <proto/exec.h>
#include <exec/memory.h>
#include "sys.h"
#include "mfx.h"
#include "mixer/mixer.h"

void mt_install(void *custom_base __asm("a6"), void *vbr __asm("a0"), UBYTE pal __asm("d0"));
void mt_remove(void *custom_base __asm("a6"));
void mt_init(void *custom_base __asm("a6"), void *module __asm("a0"), void *samples __asm("a1"), UBYTE pos __asm("d0"));
void mt_end(void *custom_base __asm("a6"));
void mt_channelmask(void *custom_base __asm("a6"), UBYTE mask __asm("d0"));
void mt_mastervol(void *custom_base __asm("a6"), UWORD volume __asm("d0"));
extern UBYTE mt_Enable;

#define CUSTOM ((void *)0xdff000)

ULONG mfx_requested, mfx_played, mfx_refused;
static void *mix_buffer;
static int have_module;
static UWORD music_volume = 64;
static struct { ULONG end; UWORD priority; } voice[4];   /* per mixer voice: the frame it falls silent, its effect's priority */
static ULONG bytes_per_frame = 220;

int mfx_open(void)
{
    mix_buffer = sys_alloc(MixerGetBufferSize(), MEMF_CHIP | MEMF_CLEAR);
    return mix_buffer != 0;
}

int mfx_start(void *module)
{
    int i, ok = 1;
    if (module && !(TypeOfMem(module) & MEMF_CHIP)) { module = 0; ok = 0; }    /* Paula would play silence */
    mt_install(CUSTOM, sys_vbr, sys_is_pal);
    have_module = module != 0;
    if (module) {
        mt_init(CUSTOM, module, 0, 0);
        mt_channelmask(CUSTOM, (UBYTE)(~mfx_cfg_channels & 15));    /* the player never touches the mixer's channel */
        mt_mastervol(CUSTOM, music_volume);
        mt_Enable = 1;
    }
    for (i = 0; i < 2; i++) sys_wait_frame();       /* the player first, the mixer second (Audio Mixer manual) */
    bytes_per_frame = (sys_is_pal ? 3546895UL : 3579545UL) / mfx_cfg_period / (sys_is_pal ? 50 : 60);
    for (i = 0; i < 4; i++) voice[i].end = 0;
    MixerSetup(mix_buffer, 0, 0, sys_is_pal ? MIX_PAL : MIX_NTSC, 0);
    MixerInstallHandler(sys_vbr, 0);
    MixerStart();
    return ok;
}

void mfx_music(int on)                  /* a paused module must also be muted: looped notes go on sounding */
{
    if (!have_module) return;
    mt_Enable = on ? 1 : 0;
    mt_mastervol(CUSTOM, on ? music_volume : 0);
}

void mfx_music_volume(UWORD volume)     /* 0-64: balances the music against the effects, loses no resolution */
{
    music_volume = volume > 64 ? 64 : volume;
    if (have_module && mt_Enable) mt_mastervol(CUSTOM, music_volume);
}

/* The voice is chosen here and not by the mixer's MixerPlayFX(), so that the choice is known: Audio Mixer 3.7.2
   reports the chosen voice unreliably (seen: 0x0108 for a voice at period 161), and its MixerGetChannelStatus()
   answers for voice 0 whatever voice is asked about when MIXER_SINGLE is 1. */
int mfx_play(const MfxSound *sound)
{
    MXEffect fx;
    ULONG now = sys_frame;
    int v, best = -1;
    mfx_requested++;
    for (v = 0; v < mfx_cfg_voices && best < 0; v++)
        if (voice[v].end <= now) best = v;                          /* a silent voice */
    if (best < 0)                                                   /* none: the least important voice that is */
        for (v = 0; v < mfx_cfg_voices; v++)                        /* not above this effect, nearest its end  */
            if (voice[v].priority <= sound->priority &&
                (best < 0 || voice[v].priority < voice[best].priority ||
                 (voice[v].priority == voice[best].priority && voice[v].end < voice[best].end))) best = v;
    if (best < 0) { mfx_refused++; return 0; }
    fx.mfx_length = sound->length;
    fx.mfx_sample_ptr = sound->data;
    fx.mfx_loop = MIX_FX_ONCE;
    fx.mfx_priority = sound->priority;
    fx.mfx_loop_offset = 0;
    fx.mfx_plugin = 0;
    if (MixerPlayChannelFX(&fx, mfx_cfg_channels | (MIX_CH0 << best)) == 0xffff) { mfx_refused++; return 0; }
    voice[best].end = now + sound->length / bytes_per_frame + 3;    /* the mixer works a frame's worth at a time */
    voice[best].priority = sound->priority;
    mfx_played++;
    return 1;
}

int mfx_voices_busy(void)
{
    ULONG now = sys_frame;
    int v, n = 0;
    for (v = 0; v < mfx_cfg_voices; v++)
        if (voice[v].end > now) n++;
    return n;
}

void mfx_stop(void)
{
    MixerStop();
    MixerRemoveHandler();
    if (have_module) { mt_Enable = 0; mt_end(CUSTOM); }
    mt_remove(CUSTOM);
}

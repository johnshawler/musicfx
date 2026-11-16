/* demo_mfx.c - the template's recommended route, working: a module on three Paula channels and up to four
 * sound effects mixed into the fourth, so neither cuts into the other.
 *
 *   demo_mfx              play: keys 1-4 one effect each, 5 all four at once, M music on/off, Esc or the
 *                         left mouse button quits. The border flashes when an effect is refused.
 *   demo_mfx bench [N]    measure and quit: writes bench.txt (N frames per phase, default 150)
 *   demo_mfx script       the fixed 20 second sequence used for recordings: music alone, effects alone, both
 *
 * Build: tools/check.sh, or by hand:
 *   CFLAGS="-Ikit" ASFLAGS="-Iexamples" tools/build.sh dist/demo_mfx kit/sys.c kit/mfx.c kit/mfx_cfg.s \
 *       kit/mixer/mixer.asm kit/ptplayer/ptplayer.asm examples/demo_mfx.c
 */
#include <proto/dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sys.h"
#include "mfx.h"
#include "incbin.h"

/* The module must be in chip RAM (Paula plays it from there); the effects may be anywhere, because the mixer
   copies them into its own chip RAM buffer as it mixes. assets/fx/ is made by tools/sfx.py (tools/check.sh). */
CHIP_INCBIN(mod_data, "examples/test.mod");     /* a tiny module made for the kit: channels 0-2, channel 3 empty */
DATA_INCBIN(fx_laser, "assets/fx/laser.raw");
DATA_INCBIN(fx_boom,  "assets/fx/boom.raw");
DATA_INCBIN(fx_coin,  "assets/fx/coin.raw");
DATA_INCBIN(fx_jump,  "assets/fx/jump.raw");
static MfxSound fx[4];
#define KEY_M 0x37

static void all_four(void) { int i; for (i = 0; i < 4; i++) mfx_play(&fx[i]); }

/* Phases of the bench: what happens once per frame. */
static void one_effect(void) { if (mfx_voices_busy() == 0) mfx_play(&fx[1]); }
static void all_voices(void) { int n = mfx_cfg_voices - mfx_voices_busy(); while (n-- > 0) mfx_play(&fx[1]); }

/* Run for some frames and count how often the idle loop turns: the fewer turns, the more time the audio took. */
static ULONG run(ULONG frames, void (*each)(void))
{
    ULONG n = 0, f, end = sys_frame + frames;
    while ((f = sys_frame) < end) {
        if (each) each();
        while (sys_frame == f) n++;
    }
    return n;
}

static ULONG pct10(ULONG base, ULONG n) { return n >= base ? 0 : (base - n) * 1000 / base; }

int main(int argc, char **argv)
{
    int bench = argc > 1 && !strcmp(argv[1], "bench");
    int script = argc > 1 && !strcmp(argv[1], "script");
    ULONG frames = argc > 2 ? strtoul(argv[2], 0, 10) : 150;
    ULONG idle = 0, music = 0, one = 0, all = 0, one_n = 0, all_n = 0, over_played = 0, over_refused = 0;
    char buf[400];
    int i, started = 0;
    ULONG phase_refused = 0;

    fx[0].data = fx_laser; fx[0].length = fx_laser_end - fx_laser;
    fx[1].data = fx_boom;  fx[1].length = fx_boom_end - fx_boom;
    fx[2].data = fx_coin;  fx[2].length = fx_coin_end - fx_coin;
    fx[3].data = fx_jump;  fx[3].length = fx_jump_end - fx_jump;
    for (i = 0; i < 4; i++) fx[i].priority = 1;
    if (!mfx_open() || !sys_take()) { sys_free_all(); return 20; }
    sys_vbl_hook = sys_key_poll;

    if (bench) {
        idle = run(frames, 0);                              /* nothing sounds, no audio interrupt runs */
        started = mfx_start(mod_data);
        music = run(frames, 0);                             /* module on three channels, mixer silent */
        one = run(frames, one_effect);                      /* + one effect at a time */
        one_n = mfx_played;
        all = run(frames, all_voices);                    /* + every voice always busy: the worst case */
        all_n = mfx_played - one_n; phase_refused = mfx_refused;
        run(100, 0);                                        /* let them end */
        mfx_requested = mfx_played = mfx_refused = 0;
        for (i = 0; i < mfx_cfg_voices; i++) { fx[i].priority = 5; mfx_play(&fx[i]); }  /* every voice important ... */
        fx[2].priority = 1; mfx_play(&fx[2]);               /* ... and one more, unimportant: must be refused */
        over_played = mfx_played; over_refused = mfx_refused;
        run(60, 0);
    } else if (script) {
        started = mfx_start(mod_data);
        run(250, 0);                                        /* 0-5 s: music alone */
        mfx_music(0);
        run(50, 0);
        for (i = 0; i < 4; i++) { mfx_play(&fx[i]); run(50, 0); }      /* 6-10 s: each effect alone */
        all_four(); run(100, 0);                            /* 10-12 s: all four together */
        mfx_music(1);
        for (i = 0; i < 4; i++) { mfx_play(&fx[i]); run(50, 0); }      /* 12-16 s: music + each effect */
        all_four(); run(100, 0);                            /* 16-18 s: music + all four */
        run(100, 0);
    } else {
        UBYTE was[128];
        int music_on = 1;
        started = mfx_start(mod_data);
        memset(was, 0, sizeof was);
        while (!sys_keys[KEY_ESC] && !sys_mouse_button()) {
            UWORD colour = 0;
            for (i = 0; i < 4; i++)
                if (sys_keys[KEY_1 + i] && !was[KEY_1 + i] && !mfx_play(&fx[i])) colour = 0xf00;
            if (sys_keys[KEY_1 + 4] && !was[KEY_1 + 4]) all_four();
            if (sys_keys[KEY_M] && !was[KEY_M]) mfx_music(music_on ^= 1);
            for (i = 0; i < 128; i++) was[i] = sys_keys[i];
            custom->color[0] = colour ? colour : (UWORD)(mfx_voices_busy() * 0x020);  /* greener = more voices */
            sys_wait_frame();
        }
        custom->color[0] = 0;
    }

    mfx_stop();
    sys_give();
    sys_free_all();

    if (bench) {
        BPTR fh;
        ULONG m = pct10(idle, music), o = pct10(idle, one), f = pct10(idle, all);
        sprintf(buf, "frames=%lu idle=%lu music_pct=%lu.%lu one_fx_pct=%lu.%lu all_fx_pct=%lu.%lu one_fx_played=%lu all_fx_played=%lu refused=%lu "
                     "overload_played=%lu overload_refused=%lu voices=%u mixer_hz=%lu hq=%u pal=%u music_in_chip=%d\n",
                frames, idle, m / 10, m % 10, o / 10, o % 10, f / 10, f % 10, one_n, all_n, phase_refused, over_played, over_refused,
                mfx_cfg_voices, (sys_is_pal ? 3546895UL : 3579545UL) / mfx_cfg_period, mfx_cfg_hq, sys_is_pal, started);
        fh = Open((STRPTR)"bench.txt", MODE_NEWFILE);
        if (fh) { Write(fh, buf, strlen(buf)); Close(fh); }
    }
    return 0;
}

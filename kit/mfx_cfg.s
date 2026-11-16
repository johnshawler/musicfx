; mfx_cfg.s - hands the settings of mixer_config.i to C (kit/mfx.c), so there is one place to change them.
        include hardware/dmabits.i
        include mixer_config.i
        section mfxcfg,data
        xdef    _mfx_cfg_channels,_mfx_cfg_voices,_mfx_cfg_period,_mfx_cfg_hq
_mfx_cfg_channels:  dc.w    mixer_output_channels   ; Paula channel(s) given to the effects mixer: bit 0 = channel 0 ...
_mfx_cfg_voices:    dc.w    mixer_sw_channels       ; effects that can sound at once on that channel
_mfx_cfg_period:    dc.w    mixer_period            ; Paula period of the mixer: its sample rate is 3546895 / period (PAL)
_mfx_cfg_hq:        dc.w    MIXER_HQ_MODE           ; 1 = effects keep all 8 bits, 0 = effects are pre-divided by the voices

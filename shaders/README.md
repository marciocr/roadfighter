# Upscaling filters (OPTIONS → VIDEO → FILTER)

GLSL shaders in the libretro/RetroArch format, loaded by `src/postfx.cpp`.
The same filters (and shader files) are used by the F-1 Spirit and Maze of
Galious remakes.

The game draws a 512x384 image in software (see `present_screen()` in
`src/main.cpp`); the filter takes it at that size.

Scale2x, 2xSaI, hq2x and xBR render at 2x and are then stretched (bilinear) to
the window. OmniScale and Lanczos are resolution independent and render
straight at the window's size, which suits fractional scales such as
fullscreen 1080p.

| File | Filter | Author | License | Source |
|---|---|---|---|---|
| `scale2x.glsl` | Scale2x (AdvMAME2x) | Andrea Mazzoleni | GPL | libretro/glsl-shaders `scalenx/shaders/scale2x.glsl` |
| `2xsai.glsl` | 2xSaI | Derek Liauw Kie Fa (Kreed), algorithm; port written for the F-1 Spirit remake | GPL-2.0-or-later | github.com/marciocr/f1spirit |
| `hqx-pass1.glsl`, `hqx-pass2.glsl`, `hq2x-lut.png` | hq2x | Maxim Stepin, Cameron Zemek, Jules Blok | LGPL-2.1-or-later | libretro/glsl-shaders `hqx/shader-files/`, `hqx/resources/hq2x.png` |
| `xbr-lv2.glsl` | xBR level 2 | Hyllian | MIT | libretro/glsl-shaders `xbr/shaders/xbr-lv2.glsl` |
| `omniscale.glsl` | OmniScale | Lior Halphon (RetroArch port by hunterk) | MIT | libretro/glsl-shaders `omniscale/shaders/omniscale.glsl` |
| `lanczos3.glsl` | Lanczos-3 (with anti-ringing) | written for the F-1 Spirit remake | GPL-2.0-or-later | github.com/marciocr/f1spirit |

The libretro files are unmodified copies, taken from
https://github.com/libretro/glsl-shaders at commit
`435612f`. Their license headers are kept in each file.

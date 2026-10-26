# v1.0.0 validation

Date: 2026-10-26. The Linux visual suite passes 26,275 counted checks, plus the
retained climb/fall checks. The lifecycle suite passes 237 checks, including all
five ordered menu choices, the native startup default, 40 pack switches and
48 fade allocations/restorations. Its focused Memcheck run reports zero errors
and zero definite/indirect leaks; remaining possible/reachable allocations are
recorded separately in the development notes.

Windows x64 is cross-compiled using MinGW GCC 16.2.1, with the official SDL2
2.32.10 Windows runtime, SDL2_image 2.8.12 and the required DLL dependency
closure. The executable reports Prince of Persia Remastered v1.0.0. Its real
welcome render and SDL window title were checked under Wine; the welcome PNG
matches the Linux 640x400 capture exactly. The Wine host has no working WASAPI
audio endpoint, so sound is not certified by this check. Native Windows input,
audio and a complete playthrough remain pending.

Linux build/startup and packaged-file checks are separate from live gameplay.
Steam Deck testing, a complete played run and broader original-Mac motion
comparisons remain open. Earlier user-confirmed damage/healing, pause/resume
and intro fades remain recorded as those specific checks only.

The original SDLPoP history is retained, with upstream base commit
3c5add5fb7f83d4ceb542823ab66d00146c4271b. No original disk image, extracted
application resources, authentication files, saves or local build caches are
included in the public repository or game downloads. Independent native-image
fixtures needed by the regression suite are included.

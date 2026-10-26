/* Presentation controls. GPL-3.0-or-later; see COPYING. */
#ifndef VIDEO_FILTERS_H
#define VIDEO_FILTERS_H
#include <SDL.h>
#define VIDEO_FILTER_COUNT 4
const char *video_filter_name(void);
void cycle_video_filter(void);
void free_video_filter_buffers(void);
void queue_graphics_pack_cycle(void);
void apply_visual_shortcuts(void);
int scale3x_rgb_surface(SDL_Surface *source, SDL_Surface *destination);
#endif

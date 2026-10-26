/* Prince of Persia Remastered presentation controls.
 * Copyright (C) 2026 The-Alok. SPDX-License-Identifier: GPL-3.0-or-later.
 * Scale3x follows the Scale2x project's published neighbourhood rules:
 * https://www.scale2x.it/algorithm . No game assets or reference executable
 * shader source are embedded here. Game canvases and actor coordinates stay
 * unchanged; the filter is applied only when presenting the final picture. */
#include "common.h"
#include "pak.h"
#include "video_filters.h"

static unsigned pending_pack_cycles;
extern char last_text_input;

const char *video_filter_name(void) {
    static const char *names[] = {"Pixel", "Sharp", "Soft", "Smooth HQ"};
    return names[scaling_type < VIDEO_FILTER_COUNT ? scaling_type : 0];
}

static void visual_setting_changed(void) {
#ifdef USE_MENU
    extern bool were_settings_changed;
    extern int need_full_menu_redraw_count;
    were_settings_changed = true;
    if (is_menu_shown) need_full_menu_redraw_count = 2;
    else save_ingame_settings();
#endif
}

void cycle_video_filter(void) {
    scaling_type = (scaling_type + 1) % VIDEO_FILTER_COUNT;
    visual_setting_changed();
    if (renderer_ && onscreen_surface_) update_screen();
}

static int pack_cycle_allowed(void) {
    return !is_cutscene && !is_ending_sequence && start_level >= 0 &&
           current_level >= 1 && current_level <= 15 && drawn_room >= 1 &&
           drawn_room <= 24 && chtab_addrs[id_chtab_2_kid] && num_available_paks > 1;
}

void queue_graphics_pack_cycle(void) {
    if (pack_cycle_allowed()) pending_pack_cycles = (pending_pack_cycles + 1) % num_available_paks;
}

void apply_visual_shortcuts(void) {
    unsigned count = pending_pack_cycles;
    pending_pack_cycles = 0;
    if (!count || !pack_cycle_allowed()) return;
    /* Reload only at this input/frame boundary, never from SDL's event pump.
     * Legacy room redraws reset the crossing timer and clear keyboard input;
     * their presentation work must not alter a running game's state. */
    char_type kid = Kid, guard = Guard, actor = Char, opponent = Opp;
    dword seed = random_seed; word seeded = seed_was_init;
    int crossing = exit_room_timer, next = next_room;
    int key = last_key_scancode, any = last_any_key_scancode, input = last_text_input;
    int blind = is_blind_mode;
    byte selected = (graphics_pack_index + count) % num_available_paks;
    int switched = switch_graphics_pack(selected);
    Kid = kid; Guard = guard; Char = actor; Opp = opponent;
    random_seed = seed; seed_was_init = seeded;
    exit_room_timer = crossing; next_room = next;
    last_key_scancode = key; last_any_key_scancode = any; last_text_input = input;
    is_blind_mode = blind;
    if (switched) {
        /* Refresh only the health-icon areas, leaving centre status text. */
        surface_type *target = current_target_surface;
        current_target_surface = onscreen_surface_;
        rect_type kid_area = {192, 0, 200, MIN(160, hitp_max * 8)};
        rect_type guard_area = {192, MAX(160, 320 - guardhp_max * 8), 200, 320};
        draw_rect(&kid_area, color_0_black); draw_rect(&guard_area, color_0_black);
        int kid_delta = hitp_delta, guard_delta = guardhp_delta;
        hitp_delta = guardhp_delta = 1; draw_hp();
        hitp_delta = kid_delta; guardhp_delta = guard_delta;
        current_target_surface = target;
        visual_setting_changed();
    }
}

static Uint32 rgb_at(SDL_Surface *s, int x, int y) {
    Uint32 value = 0;
    memcpy(&value, (byte *)s->pixels + y * s->pitch + x * 3, 3);
    return value;
}

int scale3x_rgb_surface(SDL_Surface *source, SDL_Surface *destination) {
    if (!source || !destination || source->format->BytesPerPixel != 3 ||
        destination->format->BytesPerPixel != 3 ||
        source->format->Rmask != destination->format->Rmask ||
        source->format->Gmask != destination->format->Gmask ||
        source->format->Bmask != destination->format->Bmask ||
        destination->w != source->w * 3 || destination->h != source->h * 3) return 0;
    if (SDL_LockSurface(source)) return 0;
    if (SDL_LockSurface(destination)) { SDL_UnlockSurface(source); return 0; }
    for (int y = 0; y < source->h; y++) {
        int above = y ? y - 1 : y, below = y + 1 < source->h ? y + 1 : y;
        for (int x = 0; x < source->w; x++) {
            int left = x ? x - 1 : x, right = x + 1 < source->w ? x + 1 : x;
            Uint32 a = rgb_at(source,left,above), b = rgb_at(source,x,above), c = rgb_at(source,right,above);
            Uint32 d = rgb_at(source,left,y), e = rgb_at(source,x,y), f = rgb_at(source,right,y);
            Uint32 g = rgb_at(source,left,below), h = rgb_at(source,x,below), i = rgb_at(source,right,below);
            Uint32 out[9] = {e,e,e,e,e,e,e,e,e};
            if (b != h && d != f) {
                out[0] = d == b ? d : e;
                out[1] = ((d == b && e != c) || (b == f && e != a)) ? b : e;
                out[2] = b == f ? f : e;
                out[3] = ((d == b && e != g) || (d == h && e != a)) ? d : e;
                out[5] = ((b == f && e != i) || (h == f && e != c)) ? f : e;
                out[6] = d == h ? d : e;
                out[7] = ((d == h && e != i) || (h == f && e != g)) ? h : e;
                out[8] = h == f ? f : e;
            }
            for (int row = 0; row < 3; row++) for (int col = 0; col < 3; col++)
                memcpy((byte *)destination->pixels + (y * 3 + row) * destination->pitch + (x * 3 + col) * 3, &out[row * 3 + col], 3);
        }
    }
    SDL_UnlockSurface(destination); SDL_UnlockSurface(source); return 1;
}

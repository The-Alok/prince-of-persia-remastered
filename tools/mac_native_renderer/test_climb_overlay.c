/* Link this test-only wrapper with the game's object files and
 * -Wl,--wrap=draw_tables -Wl,--wrap=draw_native_image_physical_blit.
 * Run from the project root with SDL's dummy drivers
 * and arguments "megahit 1 mute". It exits after the level assets load.
 * No test hooks are included in the normal game executable.
 */
#include "common.h"
#include <assert.h>
#include "test_native_lifecycle.inc"

void __real_draw_tables(void);
void check_interactive_floors(const char* output);
void check_animated_architecture(const char* output);
void check_palace_walls(const char* output);
void check_native_props(const char* output);
void check_native_cutscenes(const char* output);
void check_native_ui(const char* output);
void check_native_combat(const char* output);
void check_native_world(const char* output);
extern byte tile_left;
extern byte modifier_left;
extern short draw_main_y;
extern short drawn_row, drawn_col;
static int tested;
static int checking_actor, expected_actor_x, expected_actor_y, actor_draws;

void __wrap_show_dialog(const char* message) {
	fprintf(stderr, "Renderer test encountered a dialog: %s\n", message);
	abort();
}

static Uint32 read_rgb(SDL_Surface* surface, int x, int y) {
	Uint32 pixel = 0;
	memcpy(&pixel, (byte*)surface->pixels + y * surface->pitch +
	       x * surface->format->BytesPerPixel, surface->format->BytesPerPixel);
	Uint8 r, g, b;
	SDL_GetRGB(pixel, surface->format, &r, &g, &b);
	return (r << 16) | (g << 8) | b;
}

static void check_mask_edge(midtable_type* overlay, image_type* mask, image_type* base,
                            short mx, short my, short bx, short by, int first_row) {
	const Uint32 background = 0xAA11CC;
	current_target_surface = offscreen_surface;
	set_clip_rect(&rect_top);
	SDL_FillRect(offscreen_surface, NULL, SDL_MapRGB(offscreen_surface->format, 0xAA, 0x11, 0xCC));
	SDL_Rect before, after;
	SDL_GetClipRect(offscreen_surface, &before);
	draw_mid(0);
	SDL_GetClipRect(offscreen_surface, &after);
	assert(memcmp(&before, &after, sizeof(before)) == 0);
	int x = overlay->physical_x + mask->w;
	for (int y = 0; y < mask->h; ++y) {
		assert(read_rgb(offscreen_surface, x, overlay->physical_y + y) ==
		       (y < first_row ? background : read_rgb(base, mask->w + bx - mx, y + by - my)));
		assert(read_rgb(offscreen_surface, x + 1, overlay->physical_y + y) == background);
	}
}

void __real_draw_native_image_physical_blit(image_type*, int, int, int);
void __wrap_draw_native_image_physical_blit(image_type* image, int x, int y, int blit) {
	if (checking_actor) {
		assert(x == expected_actor_x);
		assert(y == expected_actor_y);
		++actor_draws;
		return;
	}
	__real_draw_native_image_physical_blit(image, x, y, blit);
}

static void check_actor_placement(void) {
	Kid.charid = charid_0_kid;
	Kid.x = 114;
	Kid.y = 55;
	for (int facing = 0; facing < 2; ++facing) {
		for (int step = 0; step < 20; ++step) {
			int frame = step < 5 ? 102 + step : 130 + step;
			memset(table_counts, 0, sizeof(table_counts));
			Kid.frame = frame;
			Kid.direction = facing ? dir_FF_left : dir_0_right;
			char_type saved_kid = Kid;
			loadkid();
			load_frame_to_obj();
			image_type* body = get_image(id_chtab_2_kid, obj_id);
			short ox, oy;
			assert(get_native_image_origin(body, &ox, &oy));
			expected_actor_x = calc_screen_x_coord(obj_x) * 2 -
			                   (facing ? ox : body->w - 1 - ox);
			expected_actor_y = obj_y * 2 - oy - (frame >= 141 && frame <= 144 ? 6 : 0);
			sbyte xh, xl;
			x_to_xh_and_xl(obj_x, &xh, &xl);
			assert(add_midtable(id_chtab_2_kid, obj_id + 1, xh, xl,
			                   obj_y, blitters_10h_transp, 0));
			checking_actor = 1;
			draw_mid(0);
			checking_actor = 0;
			assert(memcmp(&Kid, &saved_kid, sizeof(Kid)) == 0);
		}
	}
	assert(actor_draws == 40);
	puts("PASS: 40 climb/fall actor placements; mirrored origins and Kid state preserved");
}

static void check_climb_masks(void) {
	const byte tiles[] = {tiles_1_floor, tiles_3_pillar, tiles_5_stuck, tiles_19_torch};
	int cases = 0;
	draw_mode = 0;
	reset_obj_clip();
	for (int row = 0; row < 3; ++row) {
		for (int facing = 0; facing < 2; ++facing) {
			for (unsigned t = 0; t < sizeof(tiles); ++t) {
				for (int frame = 137; frame <= 144; ++frame) {
					memset(table_counts, 0, sizeof(table_counts));
					clear_tile_wipes();
					Kid.frame = frame;
					Kid.direction = facing ? dir_FF_left : dir_0_right;
					curr_tile = tiles[t];
					curr_modifier = 0;
					tile_left = tiles_0_empty;
					draw_xh = 12;
					draw_main_y = row * 63 + 62;
					redraw_frames_floor_overlay[3] = 1;
					redraw_needed(3);
					assert(midtable_count == 2);
					midtable_type* overlay = &midtable[0];
					assert(overlay->use_physical);
					assert(overlay->id == 0x14 + frame - 137 - 1);
					image_type* mask = get_image(id_chtab_6_environment, overlay->id);
					image_type* base = get_image(id_chtab_6_environment, 0x29 - 1);
					short mx, my, bx, by;
					assert(get_native_image_origin(mask, &mx, &my));
					assert(get_native_image_origin(base, &bx, &by));
					int pressed = curr_tile == tiles_5_stuck;
					int base_x = 192 - bx;
					int base_y = (draw_main_y - 3 + pressed) * 2 - by;
					assert(overlay->physical_x == base_x + bx - mx);
					assert(overlay->physical_y == base_y + by - my);
					/* The front lip retains its independent baseline. */
					short fx, fy;
					assert(get_native_image_origin(get_image(id_chtab_6_environment,
					       midtable[1].id), &fx, &fy));
					assert(midtable[1].physical_y == draw_main_y * 2 - fy);
					if ((frame >= 138 && frame <= 141) || frame == 144) {
						check_mask_edge(overlay, mask, base, mx, my, bx, by, frame == 138 ? 4 : 0);
					}
					++cases;
				}
			}
		}
	}
	printf("PASS: %d climb overlay cases (8 frames, 4 tile types, 3 rows, both directions)\n", cases);
	puts("PASS: 120 one-pixel mask-edge renders; elbow pixels, floor texture and clip bounds preserved");
}

static void capture_pose(int frame, const char* folder, int fresh_room) {
	if (fresh_room) {
		free_peels();
		memset(level.fg, 0, sizeof(level.fg));
		memset(level.bg, 0, sizeof(level.bg));
		memset(level.roomlinks, 0, sizeof(level.roomlinks));
		for (int col = 3; col < 10; ++col) level.fg[col] = tiles_1_floor;
		level.fg[13] = tiles_3_pillar;
		level.fg[23] = tiles_3_pillar;
		drawn_room = 1;
		load_room_links();
		get_room_address(1);
	}
	Kid.room = 1;
	Kid.curr_row = frame < 141 ? 1 : 0;
	Kid.x = frame < 141 ? 109 : 114;
	Kid.y = y_land[Kid.curr_row + 1];
	Kid.frame = frame;
	Kid.direction = dir_0_right;
	Kid.action = actions_1_run_jump;
	Kid.charid = charid_0_kid;
	Kid.alive = -1;
	Kid.sword = sword_0_sheathed;
	Guard.direction = dir_56_none;
	is_blind_mode = 0;
	need_drects = 0;
	if (fresh_room) {
		current_target_surface = offscreen_surface;
		draw_rect(&rect_top, color_0_black);
		redraw_room();
		__real_draw_tables();
	}
	memset(table_counts, 0, sizeof(table_counts));
	draw_moving();
	__real_draw_tables();
	if (frame == 138 || frame == 144) {
		image_type* base = get_image(id_chtab_6_environment, 0x29 - 1);
		int x = frame == 138 ? 226 : 215;
		int top = frame == 138 ? 107 : 112;
		int bottom = frame == 138 ? 109 : 112;
		for (int y = top; y <= bottom; ++y) {
			assert(read_rgb(offscreen_surface, x, y) == read_rgb(base, x - 192, y - 94));
		}
	}
	if (folder) {
		char path[1024];
		snprintf(path, sizeof(path), "%s/climb-frame-%d.png", folder, frame);
		assert(IMG_SavePNG(offscreen_surface, path) == 0);
		printf("Saved %s\n", path);
	}
}

static void probe_location(int i, int* x, int* y) {
	if (i < 15) { *x = 225; *y = 105 + i; }
	else if (i < 22) { *x = 233; *y = 108 + i - 15; }
	else if (i < 25) { *x = 226; *y = 107 + i - 22; }
	else { *x = 215; *y = 112; }
}

static void check_sequence(const char* folder) {
	/* Record clean renders where the outlines escaped.
	 * Then advance without resetting the room, exercising the real peel
	 * restore and dirty redraw paths through the finish of the climb. */
	Uint32 expected[15][26];
	for (int frame = 135; frame <= 149; ++frame) {
		capture_pose(frame, folder, 1);
		for (int i = 0; i < 26; ++i) {
			int x, y;
			probe_location(i, &x, &y);
			expected[frame - 135][i] = read_rgb(offscreen_surface, x, y);
		}
	}
	for (int frame = 135; frame <= 149; ++frame) {
		capture_pose(frame, NULL, frame == 135);
		if (frame >= 138 && frame <= 146) {
			for (int i = 0; i < 26; ++i) {
				int x, y;
				probe_location(i, &x, &y);
				assert(read_rgb(offscreen_surface, x, y) == expected[frame - 135][i]);
			}
		}
	}
	puts("PASS: consecutive climb frames 138..146 match clean renders at all four outline probes");
}

static void capture_fall(int frame, const char* folder, int fresh_room) {
	static const byte xs[] = {102, 100, 101, 101, 102};
	static const byte ys[] = {58, 64, 73, 85, 100};
	if (fresh_room) {
		free_peels();
		memset(level.fg, 0, sizeof(level.fg));
		memset(level.bg, 0, sizeof(level.bg));
		memset(level.roomlinks, 0, sizeof(level.roomlinks));
		for (int col = 3; col < 10; ++col) level.fg[col] = tiles_1_floor;
		level.fg[13] = level.fg[23] = tiles_3_pillar;
		drawn_room = 1;
		load_room_links();
		get_room_address(1);
	}
	Kid.room = 1;
	Kid.curr_row = 1;
	Kid.x = xs[frame - 102];
	Kid.y = ys[frame - 102];
	Kid.frame = frame;
	Kid.direction = dir_FF_left;
	Kid.action = frame == 106 ? actions_4_in_freefall : actions_3_in_midair;
	Kid.charid = charid_0_kid;
	Kid.alive = -1;
	Kid.sword = sword_0_sheathed;
	Guard.direction = dir_56_none;
	is_blind_mode = 0;
	need_drects = 0;
	if (fresh_room) {
		current_target_surface = offscreen_surface;
		draw_rect(&rect_top, color_0_black);
		redraw_room();
		__real_draw_tables();
	}
	memset(table_counts, 0, sizeof(table_counts));
	draw_moving();
	if (frame == 104) {
		int actor = -1, floor = -1;
		for (int i = 0; i < midtable_count; ++i) {
			if (midtable[i].chtab_id == id_chtab_2_kid) actor = i;
			if (midtable[i].chtab_id == id_chtab_6_environment && midtable[i].id == 0x29 - 1 &&
			    midtable[i].use_physical && midtable[i].physical_x == 192 && midtable[i].physical_y == 94) floor = i;
		}
		assert(actor >= 0 && floor > actor);
	}
	__real_draw_tables();
	if (frame == 104) {
		image_type* base = get_image(id_chtab_6_environment, 0x29 - 1);
		assert(read_rgb(offscreen_surface, 236, 114) == read_rgb(base, 44, 20));
	}
	if (folder) {
		char path[1024];
		snprintf(path, sizeof(path), "%s/fall-frame-%d.png", folder, frame);
		assert(IMG_SavePNG(offscreen_surface, path) == 0);
	}
}

static void check_airborne_overlay(void) {
	const byte tiles[] = {tiles_1_floor, tiles_3_pillar, tiles_5_stuck,
	                      tiles_6_closer, tiles_11_loose, tiles_15_opener, tiles_19_torch};
	int cases = 0;
	for (int facing = 0; facing < 2; ++facing) {
		for (int row = 0; row < 3; ++row) {
			for (unsigned t = 0; t < sizeof(tiles); ++t) {
				clear_tile_wipes();
				memset(table_counts, 0, sizeof(table_counts));
				reset_obj_clip();
				Kid.frame = 104;
				Kid.direction = facing ? dir_FF_left : dir_0_right;
				curr_tile = tiles[t];
				curr_modifier = 0;
				tile_left = tiles_0_empty;
				modifier_left = 0;
				drawn_row = row;
				drawn_col = 3;
				draw_xh = 12;
				draw_main_y = row * 63 + 62;
				redraw_frames2[row * 10 + 3] = 1;
				redraw_needed(row * 10 + 3);
				assert(midtable_count >= 2);
				assert(backtable_count == 0 && foretable_count == 0);
				for (int i = 0; i < midtable_count; ++i) assert(midtable[i].use_physical);
				++cases;
			}
		}
	}
	/* Preserve draw_other_overlay()'s special redraw of a previous-cell actor. */
	capture_fall(102, NULL, 1);
	clear_tile_wipes();
	memset(table_counts, 0, sizeof(table_counts));
	drawn_row = 0;
	drawn_col = 4;
	draw_xh = 16;
	draw_main_y = 62;
	curr_tile = tile_left = tiles_1_floor;
	curr_modifier = modifier_left = 0;
	objtable_count = 1;
	memset(&objtable[0], 0, sizeof(objtable[0]));
	objtable[0].tilepos = 3;
	objtable[0].chtab_id = id_chtab_2_kid;
	objtable[0].id = 114;
	redraw_frames2[4] = 1;
	redraw_needed(4);
	assert(midtable_count >= 3 && backtable_count >= 2 && foretable_count == 0);
	assert(midtable[midtable_count - 1].chtab_id == id_chtab_2_kid);
	/* Continuous floor has no exposed-neighbor overlay. */
	clear_tile_wipes();
	memset(table_counts, 0, sizeof(table_counts));
	drawn_col = 5;
	draw_xh = 20;
	redraw_frames2[5] = 1;
	redraw_needed(5);
	assert(midtable_count == 0 && backtable_count == 0 && foretable_count == 0);
	printf("PASS: %d airborne overlay cases plus exposed-neighbor actor ordering\n", cases);
}

static void check_fall_sequence(const char* folder) {
	Uint32 expected[5][2];
	for (int frame = 102; frame <= 106; ++frame) {
		capture_fall(frame, folder, 1);
		expected[frame - 102][0] = read_rgb(offscreen_surface, 236, 114);
		expected[frame - 102][1] = read_rgb(offscreen_surface, 248, 140);
	}
	for (int frame = 102; frame <= 106; ++frame) {
		capture_fall(frame, NULL, frame == 102);
		assert(read_rgb(offscreen_surface, 236, 114) == expected[frame - 102][0]);
		assert(read_rgb(offscreen_surface, 248, 140) == expected[frame - 102][1]);
	}
	puts("PASS: fall frames 102..106 occlude the overlap and preserve consecutive redraws");
}

void __wrap_draw_tables(void) {
	if (!tested && current_level == 1 && chtab_addrs[id_chtab_6_environment] &&
	    chtab_addrs[id_chtab_2_kid]) {
		tested = 1;
		if (getenv("SDLPOP_LIFECYCLE_AUDIT")) {
			check_native_lifecycle();
			exit(0);
		}
		check_climb_masks();
		check_actor_placement();
		check_sequence(getenv("SDLPOP_CLIMB_TEST_OUTPUT"));
		check_airborne_overlay();
		check_fall_sequence(getenv("SDLPOP_CLIMB_TEST_OUTPUT"));
		if (getenv("SDLPOP_FLOOR_AUDIT")) check_interactive_floors(getenv("SDLPOP_CLIMB_TEST_OUTPUT"));
		if (getenv("SDLPOP_ARCHITECTURE_AUDIT")) check_animated_architecture(getenv("SDLPOP_CLIMB_TEST_OUTPUT"));
		if (getenv("SDLPOP_PALACE_AUDIT")) check_palace_walls(getenv("SDLPOP_CLIMB_TEST_OUTPUT"));
		if (getenv("SDLPOP_PROPS_AUDIT")) check_native_props(getenv("SDLPOP_CLIMB_TEST_OUTPUT"));
		if (getenv("SDLPOP_COMBAT_AUDIT")) check_native_combat(getenv("SDLPOP_CLIMB_TEST_OUTPUT"));
		if (getenv("SDLPOP_WORLD_AUDIT")) check_native_world(getenv("SDLPOP_CLIMB_TEST_OUTPUT"));
		if (getenv("SDLPOP_UI_AUDIT")) check_native_ui(getenv("SDLPOP_CLIMB_TEST_OUTPUT"));
		if (getenv("SDLPOP_CUTSCENE_AUDIT")) check_native_cutscenes(getenv("SDLPOP_CLIMB_TEST_OUTPUT"));
		exit(0);
	}
	__real_draw_tables();
}

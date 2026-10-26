/* Full v11 audit, linked only into the temporary SDL dummy-driver test. */
#include "common.h"
#include <assert.h>
extern char last_text_input;

void __real_draw_tables(void);
static const char* audit_output;
static int checks, failures;

static void save_audit(SDL_Surface* image, const char* name) {
	if (!audit_output) return;
	char path[1024];
	snprintf(path, sizeof(path), "%s/%s.png", audit_output, name);
	assert(IMG_SavePNG(image, path) == 0);
}

static void render_full_floor(void) {
	free_peels();
	current_target_surface = offscreen_surface;
	SDL_SetClipRect(offscreen_surface, NULL);
	SDL_FillRect(offscreen_surface, NULL, SDL_MapRGB(offscreen_surface->format, 0, 0, 0));
	redraw_room();
	__real_draw_tables();
	memset(table_counts, 0, sizeof(table_counts));
	draw_mobs();
	redraw_needed_tiles();
	__real_draw_tables();
}

static void render_floor_changes(void) {
	/* Match draw_game_frame(): architecture may request buffered recomposition. */
	if (need_full_redraw) {
		need_full_redraw = 0;
		render_full_floor();
		return;
	}
	memset(table_counts, 0, sizeof(table_counts));
	draw_mobs();
	redraw_needed_tiles();
	__real_draw_tables();
}

static void compare_floor(const char* label) {
	SDL_Surface* incremental = SDL_ConvertSurfaceFormat(offscreen_surface, SDL_PIXELFORMAT_ARGB8888, 0);
	assert(incremental);
	render_full_floor();
	SDL_Surface* reference = SDL_ConvertSurfaceFormat(offscreen_surface, SDL_PIXELFORMAT_ARGB8888, 0);
	assert(reference);
	int changed = 0;
	for (int y = 0; y < reference->h; ++y) {
		Uint32* a = (Uint32*)((byte*)incremental->pixels + y * incremental->pitch);
		Uint32* b = (Uint32*)((byte*)reference->pixels + y * reference->pitch);
		for (int x = 0; x < reference->w; ++x) if (a[x] != b[x]) ++changed;
	}
	++checks;
	if (changed) {
		++failures;
		printf("FLOOR FAIL %s: %d stale/different pixels\n", label, changed);
		if (failures <= 200) {
			char name[256];
			snprintf(name, sizeof(name), "FAIL-%s-incremental", label);
			save_audit(incremental, name);
			snprintf(name, sizeof(name), "FAIL-%s-clean", label);
			save_audit(reference, name);
		}
	}
	SDL_FreeSurface(incremental);
	SDL_FreeSurface(reference);
}

static void setup_floor(byte tile, byte neighbor, int row, int col) {
	free_peels();
	memset(level.fg, 0, sizeof(level.fg));
	memset(level.bg, 0, sizeof(level.bg));
	memset(level.roomlinks, 0, sizeof(level.roomlinks));
	memset(level.doorlinks1, 0, sizeof(level.doorlinks1));
	memset(level.doorlinks2, 0, sizeof(level.doorlinks2));
	mobs_count = trobs_count = 0;
	Kid.room = 0;
	Guard.direction = dir_56_none;
	hitp_delta = guardhp_delta = 0;
	is_blind_mode = 0;
	need_drects = 0;
	need_full_redraw = 0;
	if (row < 2) for (int x = 0; x < 10; ++x) level.fg[(row + 1) * 10 + x] = tiles_20_wall;
	if (col > 0) level.fg[row * 10 + col - 1] = tiles_1_floor;
	level.fg[row * 10 + col] = tile;
	if (col < 9) {
		level.fg[row * 10 + col + 1] = neighbor;
		if (neighbor == tiles_6_closer || neighbor == tiles_15_opener) level.bg[row * 10 + col + 1] = 1;
	}
	drawn_room = 1;
	load_room_links();
	get_room_address(1);
	render_full_floor();
}

static void select_audit_tile(int pos) {
	get_room_address(1);
	get_curr_tile(pos);
	trob.room = 1;
	trob.tilepos = pos;
	trob.type = 0;
}

static void check_buttons(const char* env) {
	const byte neighbors[] = {tiles_0_empty, tiles_1_floor, tiles_6_closer, tiles_15_opener, tiles_11_loose};
	for (int row = 0; row < 3; ++row) {
		for (unsigned n = 0; n < sizeof(neighbors); ++n) {
			for (int kind = 0; kind < 2; ++kind) {
				byte tile = kind ? tiles_15_opener : tiles_6_closer;
				int pos = row * 10 + 4;
				setup_floor(tile, neighbors[n], row, 4);
				for (int press = 1; press >= 0; --press) {
					select_audit_tile(pos);
					set_doorlink_timer(0, press ? 5 : 0);
					redraw_11h();
					render_floor_changes();
					char label[128];
					snprintf(label, sizeof(label), "%s-plate%d-row%d-neighbor%d-%s", env, tile, row, neighbors[n], press ? "pressed" : "released");
					compare_floor(label);
					if (row == 1 && n == 0) save_audit(offscreen_surface, label);
				}
				/* A dropped red plate remains stuck; verify its state-change wipe. */
				if (!kind) {
					select_audit_tile(pos);
					curr_room_tiles[pos] = tiles_5_stuck;
					redraw_11h();
					render_floor_changes();
					char label[128];
					snprintf(label, sizeof(label), "%s-stuck-row%d-neighbor%d", env, row, neighbors[n]);
					compare_floor(label);
				}
			}
		}
	}
}

static void check_spikes(const char* env) {
	for (int row = 0; row < 3; ++row) {
		setup_floor(tiles_2_spike, tiles_0_empty, row, 4);
		for (int step = 0; step < 40; ++step) {
			select_audit_tile(row * 10 + 4);
			animate_tile();
			render_floor_changes();
			char label[128];
			snprintf(label, sizeof(label), "%s-spike-row%d-step%d-mod%d", env, row, step, curr_room_modif[row * 10 + 4]);
			compare_floor(label);
			if (step == 4) save_audit(offscreen_surface, label);
			if (curr_room_modif[row * 10 + 4] == 0) break;
		}
		char name[128];
		snprintf(name, sizeof(name), "%s-spikes-retracted-row%d", env, row);
		save_audit(offscreen_surface, name);
	}
}

static void check_loose(const char* env) {
	for (int row = 0; row < 3; ++row) {
		back_table_type static_pieces[3];
		const int ids[3] = {0x46 - 1, 0x4A - 1, 0x48 - 1};
		setup_floor(tiles_11_loose, tiles_0_empty, row, 4);
		/* Let the slab fall through an empty room into the room below. */
		for (int i = (row + 1) * 10; i < 30; ++i) level.fg[i] = tiles_0_empty;
		level.roomlinks[0].down = 2;
		level.roomlinks[1].up = 1;
		load_room_links();
		render_full_floor();
		for (int step = 0; step < 11; ++step) {
			select_audit_tile(row * 10 + 4);
			animate_tile();
			render_floor_changes();
			char label[128];
			snprintf(label, sizeof(label), "%s-loose-row%d-shake%d", env, row, step + 1);
			compare_floor(label);
			if (step == 9) {
				/* Compare native component positions across the last static
				 * shake and the first speed-zero moving slab. */
				memset(table_counts, 0, sizeof(table_counts));
				draw_room();
				for (int part = 0; part < 3; ++part) {
					int found = 0;
					for (int i = 0; i < backtable_count; ++i) {
						if (backtable[i].chtab_id == id_chtab_6_environment && backtable[i].id == ids[part]) {
							static_pieces[part] = backtable[i];
							++found;
						}
					}
					assert(found == 1);
				}
				render_full_floor();
				save_audit(offscreen_surface, label);
			} else if (step == 10) {
				for (int part = 0; part < 3; ++part) {
					int found = 0;
					for (int i = 0; i < midtable_count; ++i) {
						if (midtable[i].peel && midtable[i].chtab_id == id_chtab_6_environment && midtable[i].id == ids[part]) {
							assert(midtable[i].physical_x == static_pieces[part].physical_x);
							assert(midtable[i].physical_y == static_pieces[part].physical_y);
							++found;
						}
					}
					assert(found == 1);
				}
				save_audit(offscreen_surface, label);
				++checks;
			}
		}
		assert(mobs_count == 1);
		for (int step = 0; step < 17 && mobs_count; ++step) {
			do_mobs();
			render_floor_changes();
			char label[128];
			snprintf(label, sizeof(label), "%s-loose-row%d-fall%d", env, row, step);
			compare_floor(label);
			if (step == 1 || step == 5) save_audit(offscreen_surface, label);
		}
	}
}

static void check_boundaries(const char* env) {
	/* Last/first columns, including native overhangs arriving from a linked
	 * room on the left. */
	for (int row = 0; row < 3; ++row) {
		for (int side = 0; side < 3; ++side) {
			int col = side == 0 ? 0 : 9;
			setup_floor(tiles_2_spike, tiles_0_empty, row, col);
			if (side == 2) {
				level.roomlinks[0].right = 2;
				level.roomlinks[1].left = 1;
				if (row < 2) for (int x = 0; x < 10; ++x) level.fg[30 + (row + 1) * 10 + x] = tiles_20_wall;
				drawn_room = 2;
				load_room_links();
				get_room_address(2);
				render_full_floor();
			}
			for (int step = 0; step < 23; ++step) {
				select_audit_tile(row * 10 + col);
				animate_tile();
				render_floor_changes();
				char label[128];
				snprintf(label, sizeof(label), "%s-boundary-row%d-side%d-spike%d", env, row, side, step);
				compare_floor(label);
			}
		}
	}
	/* Follow a loose slab from the upper-room view into the lower room. */
	setup_floor(tiles_0_empty, tiles_0_empty, 2, 4);
	level.roomlinks[0].down = 2;
	level.roomlinks[1].up = 1;
	mobs_count = 1;
	mobs[0] = (mob_type){.xh = 16, .y = 176, .room = 1, .speed = 3, .type = 0, .row = 2};
	load_room_links();
	render_full_floor();
	for (int step = 0; step < 18 && mobs_count; ++step) {
		if (step == 1) {
			drawn_room = 2;
			load_room_links();
			get_room_address(2);
			render_full_floor();
		}
		do_mobs();
		render_floor_changes();
		char label[128];
		snprintf(label, sizeof(label), "%s-room-crossing-slab-step%d", env, step);
		compare_floor(label);
		if (step == 4 || step == 8) save_audit(offscreen_surface, label);
	}
}

static void check_debris(const char* env) {
	for (int row = 0; row < 3; ++row) {
		for (int torch = 0; torch < 2; ++torch) {
			byte tile = torch ? tiles_19_torch : tiles_1_floor;
			setup_floor(tile, tiles_0_empty, row, 4);
			select_audit_tile(row * 10 + 4);
			curr_room_tiles[row * 10 + 4] = torch ? tiles_30_torch_with_debris : tiles_14_debris;
			redraw_20h();
			render_floor_changes();
			char label[128];
			snprintf(label, sizeof(label), "%s-debris-row%d-torch%d", env, row, torch);
			compare_floor(label);
			save_audit(offscreen_surface, label);
			if (torch) for (int frame = 0; frame < 9; ++frame) {
				select_audit_tile(row * 10 + 4);
				curr_room_modif[row * 10 + 4] = frame;
				set_redraw_anim_right();
				render_floor_changes();
				snprintf(label, sizeof(label), "%s-debris-torch-row%d-frame%d", env, row, frame);
				compare_floor(label);
			}
		}
	}
}

static void check_floor_seams(const char* env) {
	for (int row = 0; row < 3; ++row) {
		for (int side = 0; side < 3; ++side) {
			int col = side == 0 ? 0 : side == 1 ? 4 : 9;
			setup_floor(tiles_1_floor, tiles_1_floor, row, col);
			for (int modifier = 0; modifier < 4; ++modifier) {
				select_audit_tile(row * 10 + col);
				curr_room_modif[row * 10 + col] = modifier;
				redraw_height = 63;
				redraw_tile_height();
				render_floor_changes();
				char label[128];
				snprintf(label, sizeof(label), "%s-floor-seam-row%d-col%d-mod%d", env, row, col, modifier);
				compare_floor(label);
			}
		}
	}
}

void check_interactive_floors(const char* output) {
	setvbuf(stdout, NULL, _IONBF, 0);
	audit_output = output;
	for (int palace = 0; palace < 2; ++palace) {
		if (palace) load_lev_spr(4);
		const char* env = palace ? "palace" : "dungeon";
		check_loose(env);
		check_spikes(env);
		check_buttons(env);
		check_debris(env);
		check_floor_seams(env);
		check_boundaries(env);
	}
	printf("FLOOR AUDIT: %d checks, %d failures\n", checks, failures);
	assert(failures == 0);
}

static void check_gates(const char* env) {
	static const byte expected_fast_speeds[] = {0, 0, 0, 20, 40, 60, 80, 100, 120};
	for (int row = 0; row < 3; ++row) {
		int pos = row * 10 + 4;
		setup_floor(tiles_4_gate, tiles_1_floor, row, 4);
		char name[128];
		snprintf(name, sizeof(name), "%s-gate-row%d-closed", env, row);
		save_audit(offscreen_surface, name);
		for (int phase = 0; phase < 3; ++phase) {
			/* Real opening, slow closing, and fast closing animation states. */
			sbyte anim_type = phase == 0 ? 2 : phase == 1 ? 0 : 3;
			if (phase != 0) {
				get_room_address(1);
				curr_room_modif[pos] = 188;
				render_full_floor();
			}
			for (int step = 0; step < 256; ++step) {
				select_audit_tile(pos);
				trob.type = anim_type;
				int old_modifier = curr_modifier;
				int old_type = anim_type;
				animate_tile();
				anim_type = trob.type;
				int expected;
				if (phase == 0) expected = old_modifier + 4 >= 188 ? 255 : old_modifier + 4;
				else if (phase == 1) expected = old_modifier - 1;
				else expected = MAX(0, old_modifier - expected_fast_speeds[MIN(old_type + 1, 8)]);
				assert(level.bg[pos] == expected);
				render_floor_changes();
				snprintf(name, sizeof(name), "%s-gate-row%d-phase%d-mod%d", env, row, phase, level.bg[pos]);
				compare_floor(name);
				if (row == 1 && (step == 0 || step == 23 || anim_type < 0)) save_audit(offscreen_surface, name);
				if (anim_type < 0) break;
			}
		}
	}
}

static void check_exit_clip(int row) {
	memset(table_counts, 0, sizeof(table_counts));
	draw_room();
	int tested_panel = 0;
	for (int i = 0; i < backtable_count; ++i) {
		back_table_type* entry = &backtable[i];
		if (entry->chtab_id != id_chtab_6_environment || entry->id != 0x21 - 1) continue;
		short oy;
		image_type* panel = get_image(id_chtab_6_environment, 0x21 - 1);
		assert(get_native_image_origin(panel, NULL, &oy));
		assert(entry->physical_clip_top == row * 126 + 92 - oy);
		assert(entry->physical_y + panel->h == entry->physical_clip_top);
		assert(entry->physical_clip_bottom > entry->physical_clip_top);
		SDL_Surface* target = SDL_CreateRGBSurfaceWithFormat(0, offscreen_surface->w, offscreen_surface->h, 32, SDL_PIXELFORMAT_ARGB8888);
		assert(target);
		Uint32 sentinel = SDL_MapRGB(target->format, 0xAA, 0x11, 0xCC);
		SDL_FillRect(target, NULL, sentinel);
		SDL_Surface* saved = current_target_surface;
		current_target_surface = target;
		SDL_Rect before, after;
		SDL_GetClipRect(target, &before);
		draw_back_fore(0, i);
		SDL_GetClipRect(target, &after);
		assert(memcmp(&before, &after, sizeof(before)) == 0);
		for (int y = 0; y < target->h; ++y) {
			if (y >= entry->physical_clip_top && y < entry->physical_clip_bottom) continue;
			Uint32* pixels = (Uint32*)((byte*)target->pixels + y * target->pitch);
			for (int x = 0; x < target->w; ++x) assert(pixels[x] == sentinel);
		}
		current_target_surface = saved;
		SDL_FreeSurface(target);
		++tested_panel;
	}
	assert(tested_panel > 0);
	++checks;
	render_full_floor();
}

static void check_exit_doors(const char* env) {
	for (int row = 0; row < 3; ++row) {
		for (int entrance = 0; entrance < 2; ++entrance) {
			int pos = row * 10 + 4;
			setup_floor(tiles_16_level_door_left, tiles_17_level_door_right, row, 4);
			level.start_room = entrance ? 1 : 2;
			leveldoor_open = 1; // Keep level-4 mirror-spawn gameplay out of this fixture.
			render_full_floor();
			if (row == 1) {
				char label[128];
				snprintf(label, sizeof(label), "%s-exit-row%d-entry%d-closed", env, row, entrance);
				save_audit(offscreen_surface, label);
			}
			for (int phase = 0; phase < 2; ++phase) {
				sbyte anim_type = phase ? 3 : 0;
				for (int step = 0; step < 48; ++step) {
					select_audit_tile(pos);
					trob.type = anim_type;
					animate_tile();
					anim_type = trob.type;
					render_floor_changes();
					char label[128];
					snprintf(label, sizeof(label), "%s-exit-row%d-entry%d-phase%d-mod%d", env, row, entrance, phase, level.bg[pos]);
					compare_floor(label);
					if (row == 1 && (step == 0 || step == 21 || anim_type < 0)) save_audit(offscreen_surface, label);
					if (anim_type < 0) {
						if (phase == 0) check_exit_clip(row);
						break;
					}
				}
			}
		}
	}
}

static void check_chompers(const char* env) {
	for (int row = 0; row < 3; ++row) {
		for (int blood = 0; blood < 2; ++blood) {
			int pos = row * 10 + 4;
			setup_floor(tiles_18_chomper, tiles_1_floor, row, 4);
			get_room_address(1);
			curr_room_modif[pos] = blood ? 0x80 : 0;
			render_full_floor();
			for (int step = 0; step < 30; ++step) {
				select_audit_tile(pos);
				animate_tile();
				render_floor_changes();
				char label[128];
				snprintf(label, sizeof(label), "%s-chomper-row%d-blood%d-mod%d", env, row, blood, level.bg[pos]);
				compare_floor(label);
				if (row == 1 && step < 6) save_audit(offscreen_surface, label);
			}
		}
	}
}

static void check_mirror_appearance(const char* env) {
	for (int row = 0; row < 3; ++row) {
		setup_floor(tiles_1_floor, tiles_1_floor, row, 4);
		for (int shown = 1; shown >= 0; --shown) {
			select_audit_tile(row * 10 + 4);
			curr_room_tiles[row * 10 + 4] = shown ? tiles_13_mirror : tiles_1_floor;
			redraw_at_trob();
			render_floor_changes();
			char label[128];
			snprintf(label, sizeof(label), "%s-mirror-row%d-shown%d", env, row, shown);
			compare_floor(label);
			if (row == 1) save_audit(offscreen_surface, label);
		}
	}
}

static void check_mirror_spawn(void) {
	setup_floor(tiles_16_level_door_left, tiles_17_level_door_right, 1, 4);
	int room = custom->mirror_room;
	int pos = custom->mirror_row * 10 + custom->mirror_column;
	for (int col = 0; col < 10; ++col) level.fg[(room - 1) * 30 + col] = tiles_1_floor;
	drawn_room = room;
	load_room_links();
	get_room_address(room);
	render_full_floor();
	level.bg[14] = 42;
	leveldoor_open = 0;
	need_full_redraw = 0;
	select_audit_tile(14);
	animate_tile();
	assert(level.fg[(room - 1) * 30 + pos] == custom->mirror_tile);
	assert(need_full_redraw); // Door is elsewhere; the newly visible mirror needs its own request.
	render_floor_changes();
	compare_floor("palace-mirror-gameplay-spawn");
	save_audit(offscreen_surface, "palace-mirror-gameplay-spawn");
}

static void benchmark_architecture(void) {
	setup_floor(tiles_4_gate, tiles_1_floor, 1, 4);
	for (int row = 0; row < 3; ++row) {
		for (int col = 0; col < 10; ++col) level.fg[row * 10 + col] = tiles_1_floor;
		level.fg[row * 10 + 4] = tiles_4_gate;
	}
	render_full_floor();
	Uint64 start = SDL_GetPerformanceCounter();
	for (int i = 0; i < 200; ++i) render_full_floor();
	double ms = 1000.0 * (SDL_GetPerformanceCounter() - start) / SDL_GetPerformanceFrequency() / 200;
	printf("ARCHITECTURE BENCHMARK: three-gate room, 200 offscreen redraws, mean %.3f ms\n", ms);
}

static void check_architecture_scheduling(void) {
	setup_floor(tiles_4_gate, tiles_1_floor, 1, 4);
	char saved_pack[POP_MAX_PATH];
	snprintf(saved_pack, sizeof(saved_pack), "%s", graphics_pack_name);
	snprintf(graphics_pack_name, sizeof(graphics_pack_name), "dos");
	select_audit_tile(14);
	trob.type = 2;
	animate_tile();
	assert(level.bg[14] == 4 && !need_full_redraw);
	snprintf(graphics_pack_name, sizeof(graphics_pack_name), "%s", saved_pack);
	++checks;
	/* Substeps within one native gate pose keep the lightweight redraw path. */
	level.bg[14] = 2;
	select_audit_tile(14);
	trob.type = 0;
	animate_tile();
	assert(level.bg[14] == 1 && !need_full_redraw);
	++checks;
	/* An unrelated room's gate must not cause visible-room recomposition. */
	level.fg[23 * 30 + 14] = tiles_4_gate;
	level.bg[23 * 30 + 14] = 0;
	get_room_address(24);
	trob.room = 24;
	trob.tilepos = 14;
	trob.type = 2;
	animate_tile();
	assert(level.bg[23 * 30 + 14] == 4 && !need_full_redraw);
	++checks;
	puts("PASS: architecture recomposition stays native-only, pose-based and visibility-scoped");
}

static void check_gate_priority(const char* env) {
	for (int modifier = 0; modifier <= 188; modifier += 4) {
		setup_floor(tiles_4_gate, tiles_1_floor, 1, 4);
		level.bg[14] = modifier;
		Kid.room = 1;
		Kid.curr_row = 1;
		Kid.curr_col = 4;
		render_full_floor();
		/* Drawing the gate in front of the Prince must never submit its
		 * baked floor pieces or the BACK-only partial/post slice. */
		memset(table_counts, 0, sizeof(table_counts));
		draw_room();
		int bars = 0;
		for (int i = 0; i < foretable_count; ++i) {
			back_table_type* p = &foretable[i];
			if (p->chtab_id != id_chtab_6_environment) continue;
			assert(p->id != 0x2F - 1 && p->id != 0x32 - 1);
			if (p->id == 0x33 - 1) ++bars;
		}
		assert(bars == 1);
		++checks;
		if (modifier == 0) {
			render_full_floor();
			char name[100];
			snprintf(name, sizeof(name), "%s-gate-actor-priority", env);
			save_audit(offscreen_surface, name);
		}
	}
}

static void check_exit_actors(const char* env) {
	for (int row = 0; row < 3; ++row) {
		setup_floor(tiles_16_level_door_left, tiles_17_level_door_right, row, 4);
		level.bg[row * 10 + 4] = 43;
		level.start_room = 2;
		Kid = (char_type){0};
		Kid.room = 1;
		Kid.curr_row = row;
		Kid.curr_col = 4;
		Kid.x = x_bump[4 + FIRST_ONSCREEN_COLUMN] + 10;
		Kid.y = y_land[row + 1];
		Kid.direction = dir_FF_left;
		Kid.charid = charid_0_kid;
		Kid.alive = -1;
		loadkid();
		seqtbl_offset_char(seq_70_go_up_on_level_door);
		savekid();
		for (int frame = 217; frame <= 228; ++frame) {
			loadkid();
			play_seq();
			assert(Char.frame == frame);
			savekid();
			render_full_floor();
			SDL_Surface* background = SDL_ConvertSurface(offscreen_surface, offscreen_surface->format, 0);
			assert(background);
			/* Poison old globals to prove another room/exit cannot hide Kid. */
			leveldoor_ybottom = 190;
			leveldoor_right = 0;
			memset(table_counts, 0, sizeof(table_counts));
			draw_moving();
			if (frame >= 224) {
				assert(objtable_count > 0);
				assert(objtable[0].clip.right > 160);
				assert(objtable[0].clip.top < row * 63 + 10);
			}
			__real_draw_tables();
			int visible = 0;
			for (int y = 0; y < 384; ++y) {
				byte* a = (byte*)background->pixels + y * background->pitch;
				byte* b = (byte*)offscreen_surface->pixels + y * offscreen_surface->pitch;
				for (int x = 0; x < 640 * background->format->BytesPerPixel; ++x) if (a[x] != b[x]) ++visible;
			}
			/* The final pose can be fully behind the jamb. Earlier stair
			 * poses must remain visible, especially the formerly lost 224+. */
			if (row == 1 && frame < 228) assert(visible > 0);
			SDL_FreeSurface(background);
			char label[128];
			snprintf(label, sizeof(label), "%s-exit-row%d-actor-%d", env, row, frame);
			if (row == 1) save_audit(offscreen_surface, label);
			++checks;
		}
		int old_next_level = next_level;
		for (int i = 0; i < 40 && next_level == old_next_level; ++i) {
			loadkid();
			play_seq();
			savekid();
		}
		assert(next_level == old_next_level + 1);
		++checks;
	}
}

static void check_room_redraw_timing(void) {
	load_lev_spr(1);
	setup_floor(tiles_4_gate, tiles_1_floor, 1, 4);
	text_time_remaining = 0;
	different_room = 0;
	is_keyboard_mode = 1;
	for (int timer = 0; timer <= 2; ++timer) {
		exit_room_timer = timer;
		last_key_scancode = SDL_SCANCODE_RIGHT;
		last_text_input = 'x';
		need_full_redraw = 1;
		draw_game_frame();
		assert(exit_room_timer == timer);
		assert(last_key_scancode == SDL_SCANCODE_RIGHT && last_text_input == 'x');
		++checks;
	}
	/* A real pending room transition takes precedence over recomposition. */
	level.roomlinks[0].right = 2;
	level.roomlinks[1].left = 1;
	next_room = 2;
	different_room = need_full_redraw = 1;
	draw_game_frame();
	assert(drawn_room == 2 && !different_room && !need_full_redraw);
	assert(exit_room_timer == 2 && last_key_scancode == 0);
	++checks;
	for (int left = 0; left < 2; ++left) {
		setup_floor(tiles_4_gate, tiles_1_floor, 1, 4);
		level.roomlinks[0].left = left ? 2 : 0;
		level.roomlinks[0].right = left ? 0 : 2;
		load_room_links();
		Kid = (char_type){0};
		Kid.room = next_room = 1;
		Kid.curr_row = 1;
		Kid.curr_col = left ? 0 : 9;
		Kid.x = left ? 40 : 210;
		Kid.y = y_land[2];
		Kid.frame = frame_15_stand;
		Kid.direction = left ? dir_FF_left : dir_0_right;
		Kid.alive = -1;
		exit_room_timer = different_room = 0;
		select_audit_tile(14);
		trob.type = 2;
		animate_tile();
		assert(need_full_redraw);
		draw_game_frame();
		exit_room();
		assert(Kid.room == 2 && next_room == 2);
		check_the_end();
		assert(different_room);
		draw_game_frame();
		assert(drawn_room == 2 && !different_room);
		++checks;
	}
	puts("PASS: native refresh preserves exit timer/input; real room change wins redraw scheduling");
}

static void render_people_changes(void) {
	if (need_full_redraw) {
		need_full_redraw = 0;
		render_full_floor();
	}
	loadkid();
	memset(table_counts, 0, sizeof(table_counts));
	draw_moving();
	__real_draw_tables();
}

static void compare_people(const char* label) {
	SDL_Surface* actual = SDL_ConvertSurfaceFormat(offscreen_surface, SDL_PIXELFORMAT_ARGB8888, 0);
	char_type kid = Kid, guard = Guard;
	render_full_floor();
	render_people_changes();
	assert(memcmp(&kid, &Kid, sizeof(Kid)) == 0 && memcmp(&guard, &Guard, sizeof(Guard)) == 0);
	SDL_Surface* clean = SDL_ConvertSurfaceFormat(offscreen_surface, SDL_PIXELFORMAT_ARGB8888, 0);
	int different = 0;
	for (int y = 0; y < 384; ++y) {
		Uint32* a = (Uint32*)((byte*)actual->pixels + y * actual->pitch);
		Uint32* b = (Uint32*)((byte*)clean->pixels + y * clean->pitch);
		for (int x = 0; x < 640; ++x) if (a[x] != b[x]) ++different;
	}
	++checks;
	if (different) {
		++failures;
		printf("ACTOR FAIL %s: %d stale/different pixels\n", label, different);
		char name[160];
		snprintf(name, sizeof(name), "FAIL-%s-incremental", label);
		save_audit(actual, name);
		snprintf(name, sizeof(name), "FAIL-%s-clean", label);
		save_audit(clean, name);
	}
	SDL_FreeSurface(actual);
	SDL_FreeSurface(clean);
}

static void check_native_shadow_xor(void) {
	SDL_Surface* saved = current_target_surface;
	int saved_drects = need_drects;
	need_drects = 0;
	for (int indexed = 0; indexed < 2; ++indexed) {
		SDL_Surface* source = SDL_CreateRGBSurfaceWithFormat(0, 5, 3, indexed ? 8 : 32,
			indexed ? SDL_PIXELFORMAT_INDEX8 : SDL_PIXELFORMAT_ARGB8888);
		SDL_Color colors[] = {{0x11,0x22,0x33,255}, {0xAB,0xCD,0xEF,255}, {0x12,0x34,0x56,255}};
		if (indexed) SDL_SetPaletteColors(source->format->palette, colors, 0, 3);
		for (int y = 0; y < 3; ++y) for (int x = 0; x < 5; ++x) {
			int c = (x + y) % 3;
			if (indexed) *((byte*)source->pixels + y * source->pitch + x) = c;
			else *((Uint32*)((byte*)source->pixels + y * source->pitch) + x) =
				SDL_MapRGBA(source->format, colors[c].r, colors[c].g, colors[c].b, c ? 255 : 0);
		}
		for (int py = -1; py <= 8; py += 3) for (int px = -3; px <= 11; px += 7) {
			SDL_Surface* target = SDL_CreateRGBSurfaceWithFormat(0, 13, 9, 32, SDL_PIXELFORMAT_ARGB8888);
			current_target_surface = target;
			const Uint32 background = 0xA5335577;
			SDL_FillRect(target, NULL, background);
			SDL_Rect clip = {2, 1, 8, 6}, after;
			SDL_SetClipRect(target, &clip);
			SDL_SetSurfaceBlendMode(target, SDL_BLENDMODE_BLEND);
			draw_native_image_physical_blit(source, px, py, blitters_3_xor);
			SDL_GetClipRect(target, &after);
			assert(memcmp(&clip, &after, sizeof(clip)) == 0);
			SDL_BlendMode blend;
			SDL_GetSurfaceBlendMode(target, &blend);
			assert(blend == SDL_BLENDMODE_BLEND);
			for (int y = 0; y < 9; ++y) for (int x = 0; x < 13; ++x) {
				Uint32 expected = background;
				if (x >= 2 && x < 10 && y >= 1 && y < 7 && x >= px && x < px+5 && y >= py && y < py+3) {
					int c = (x-px+y-py) % 3;
					if (c) expected ^= (colors[c].r << 16) | (colors[c].g << 8) | colors[c].b;
				}
				assert(*((Uint32*)((byte*)target->pixels + y * target->pitch) + x) == expected);
			}
			/* XOR twice must restore the background, including its alpha. */
			draw_native_image_physical_blit(source, px, py, blitters_3_xor);
			for (int y = 0; y < 9; ++y) for (int x = 0; x < 13; ++x)
				assert(*((Uint32*)((byte*)target->pixels + y * target->pitch) + x) == background);
			SDL_FreeSurface(target);
			++checks;
		}
		SDL_FreeSurface(source);
	}
	current_target_surface = saved;
	need_drects = saved_drects;
	puts("PASS: native XOR pixels, transparent RGB, alpha, negative origins and caller clips");
}

static void check_chomper_actors(const char* env) {
	for (int row = 0; row < 3; ++row) for (int facing = 0; facing < 2; ++facing) {
		setup_floor(tiles_18_chomper, tiles_1_floor, row, 4);
		for (int col = 0; col < 10; ++col) level.fg[row * 10 + col] = col == 4 ? tiles_18_chomper : tiles_1_floor;
		Kid = (char_type){0};
		Kid.room = 1;
		Kid.curr_row = row;
		Kid.y = y_land[row + 1];
		Kid.direction = facing ? dir_FF_left : dir_0_right;
		Kid.alive = -1;
		jumped_through_mirror = united_with_shadow = 0;
		render_full_floor();
		for (int step = 0; step < 24; ++step) {
			select_audit_tile(row * 10 + 4);
			animate_tile();
			Kid.x = facing ? 145 - step * 2 : 99 + step * 2;
			Kid.frame = 1 + step % 12;
			loadkid();
			load_fram_det_col();
			savekid();
			render_people_changes();
			char label[120];
			snprintf(label, sizeof(label), "%s-chomper-actor-row%d-face%d-step%d", env, row, facing, step);
			compare_people(label);
			if (row == 1 && facing == 0 && step >= 10 && step <= 15) save_audit(offscreen_surface, label);
		}
	}
	setup_floor(tiles_18_chomper, tiles_1_floor, 1, 4);
	Kid = (char_type){0};
	Kid.room = 1;
	Kid.curr_row = 1;
	Kid.curr_col = 4;
	Kid.x = 124;
	Kid.y = y_land[2];
	Kid.frame = 15;
	Kid.direction = dir_FF_left;
	Kid.alive = -1;
	hitp_curr = hitp_max = 3;
	level.bg[14] = 2;
	render_full_floor();
	loadkid();
	get_tile(1, 4, 1);
	curr_room_modif[curr_tilepos] = 2;
	chomped();
	savekid();
	assert(Kid.frame == frame_178_chomped && (level.bg[14] & 0x80));
	render_people_changes();
	char label[100];
	snprintf(label, sizeof(label), "%s-chomper-actual-death", env);
	compare_people(label);
	save_audit(offscreen_surface, label);
	hitp_delta = guardhp_delta = 0;
}

static void check_architecture_edges(const char* env) {
	for (int side = 0; side < 3; ++side) {
		setup_floor(tiles_1_floor, tiles_1_floor, 1, 4);
		int room = side == 0 ? 1 : 2;
		int pos = side == 2 ? 24 : 19;
		if (side == 0) level.roomlinks[0].right = 2;
		if (side == 1) level.roomlinks[0].left = 2;
		if (side == 2) level.roomlinks[0].up = 2;
		level.fg[(room - 1) * 30 + pos] = tiles_4_gate;
		load_room_links();
		render_full_floor();
		for (int step = 0; step < 48; ++step) {
			trob = (trob_type){.room = room, .tilepos = pos, .type = 2};
			animate_tile();
			render_floor_changes();
			char label[100];
			snprintf(label, sizeof(label), "%s-gate-edge%d-step%d", env, side, step);
			compare_floor(label);
			if (step == 23) save_audit(offscreen_surface, label);
		}
	}
}

static void check_mirror_interactions(void) {
	load_lev_spr(4);
	current_level = 4;
	setup_floor(tiles_13_mirror, tiles_1_floor, 0, 4);
	for (int col = 0; col < 10; ++col) level.fg[col] = col == 4 ? tiles_13_mirror : tiles_1_floor;
	for (int facing = 0; facing < 2; ++facing) for (int frame_index = 0; frame_index < 2; ++frame_index) {
		for (int x = 116; x <= 128; x += 2) {
			Kid = (char_type){0};
			Kid.room = 1;
			Kid.curr_row = 0;
			Kid.curr_col = 4;
			Kid.x = x;
			Kid.y = y_land[1];
			Kid.frame = frame_index ? 43 : 15;
			Kid.direction = facing ? dir_FF_left : dir_0_right;
			Kid.alive = -1;
			Guard.direction = dir_56_none;
			jumped_through_mirror = united_with_shadow = 0;
			loadkid();
			load_fram_det_col();
			savekid();
			render_full_floor();
			loadkid();
			memset(table_counts, 0, sizeof(table_counts));
			draw_moving();
			__real_draw_tables();
			char name[100];
			snprintf(name, sizeof(name), "mirror-reflection-face%d-frame%d-x%d", facing, Kid.frame, x);
			save_audit(offscreen_surface, name);
			compare_people(name);
		}
	}
	setup_floor(tiles_1_floor, tiles_1_floor, 1, 4);
	for (int col = 0; col < 10; ++col) level.fg[10 + col] = tiles_1_floor;
	Guard = (char_type){0};
	Guard.room = 1;
	Guard.curr_row = 1;
	Guard.curr_col = 4;
	Guard.x = 124;
	Guard.y = y_land[2];
	Guard.frame = 15;
	Guard.direction = dir_FF_left;
	Guard.charid = charid_1_shadow;
	Guard.alive = -1;
	render_full_floor();
	loadkid();
	memset(table_counts, 0, sizeof(table_counts));
	draw_moving();
	__real_draw_tables();
	save_audit(offscreen_surface, "shadow-standing");
	compare_people("shadow-standing");
	/* Exercise the actual collision flag and render-time shadow handoff. */
	setup_floor(tiles_13_mirror, tiles_1_floor, 0, 4);
	for (int col = 0; col < 10; ++col)
		level.fg[(custom->mirror_room - 1) * 30 + col] = col == 4 ? tiles_13_mirror : tiles_1_floor;
	drawn_room = custom->mirror_room;
	load_room_links();
	Kid = (char_type){0};
	Kid.room = drawn_room;
	Kid.curr_col = 4;
	Kid.x = 140;
	Kid.y = y_land[1];
	Kid.direction = dir_FF_left;
	Kid.alive = -1;
	Guard.direction = dir_56_none;
	loadkid();
	seqtbl_offset_char(seq_4_run_jump);
	for (int i = 0; i < 20; ++i) { play_seq(); if (Char.frame == 43) break; }
	assert(Char.frame == 43);
	Char.x = 124;
	Char.curr_col = 4;
	savekid();
	render_full_floor();
	loadkid();
	get_tile(Kid.room, 4, 0);
	assert(is_obstacle() == 0);
	assert(jumped_through_mirror == -1 && curr_room_modif[curr_tilepos] == 0x56);
	hitp_max = hitp_curr = 3;
	char_type before = Kid;
	render_people_changes();
	assert(jumped_through_mirror == 0 && Guard.charid == charid_1_shadow);
	assert(Guard.direction == dir_0_right && hitp_curr == 1 && guardhp_curr == 3);
	assert(memcmp(&before, &Kid, sizeof(Kid)) == 0);
	/* This draw consumes a one-shot event; the next clean draw has a
	 * different reflection-eligibility state. Compare subsequent frames. */
	++checks;
	save_audit(offscreen_surface, "mirror-actual-jump-handoff");
	for (int i = 0; i < 10; ++i) {
		play_guard_frame();
		render_people_changes();
		char name[100];
		snprintf(name, sizeof(name), "mirror-shadow-escape-%02d", i);
		compare_people(name);
		save_audit(offscreen_surface, name);
	}
	puts("PASS: mirror reflection redraws, actual jump flag, health/shadow handoff and shadow escape");
}

static void check_slab_architecture(const char* env) {
	const byte landings[] = {tiles_1_floor, tiles_15_opener, tiles_6_closer};
	int full = 0, partial = 0;
	for (int phase = 0; phase < 4; ++phase) for (int landing = 0; landing < 3; ++landing)
	for (int view = 0; view < 4; ++view) {
		setup_floor(tiles_0_empty, tiles_0_empty, 1, 4);
		memset(level.fg, 0, sizeof(level.fg));
		int gate_room = view == 1 || view == 2 ? 2 : 1;
		int gate_row = gate_room == 2 ? 0 : 1;
		int gate_col = view == 3 ? 0 : 3;
		int gate_pos = gate_row * 10 + gate_col;
		int slab_room = view == 3 ? 2 : 1;
		int slab_col = view == 3 ? 9 : 4;
		int land_room = view == 1 || view == 2 ? 2 : slab_room;
		int land_row = land_room == 2 && view != 3 ? 1 : 2;
		int land_pos = land_row * 10 + slab_col;
		if (view == 1 || view == 2) {
			level.roomlinks[0].down = 2;
			level.roomlinks[1].up = 1;
		} else if (view == 3) {
			level.roomlinks[0].left = 2;
			level.roomlinks[1].right = 1;
		}
		for (int col = 0; col < 10; ++col) level.fg[(land_room - 1) * 30 + land_row * 10 + col] = tiles_1_floor;
		level.fg[(land_room - 1) * 30 + land_pos] = landings[landing];
		level.fg[(gate_room - 1) * 30 + gate_pos] = tiles_4_gate;
		level.bg[(gate_room - 1) * 30 + gate_pos] = phase == 0 ? 0 : phase == 2 ? 205 : 188;
		/* One real trigger link from the slab's landing plate to this gate. */
		level.doorlinks1[0] = 0x80 | gate_pos | ((gate_room & 3) << 5);
		level.doorlinks2[0] = (gate_room & 0x1C) << 3;
		trobs_count = 1;
		trobs[0] = (trob_type){.room = gate_room, .tilepos = gate_pos, .type = phase == 0 ? 1 : phase == 3 ? 3 : 0};
		mobs_count = 1;
		mobs[0] = (mob_type){.xh = slab_col * 4, .y = view == 1 || view == 2 ? 174 : 65,
			.room = slab_room, .speed = 0, .type = 0, .row = view == 1 || view == 2 ? 2 : 1};
		drawn_room = view == 1 ? 2 : 1;
		load_room_links();
		Kid.room = 0;
		Guard.direction = dir_56_none;
		render_full_floor();
		for (int step = 0; step < 24; ++step) {
			do_mobs();
			process_trobs();
			if (view == 2 && mobs_count && mobs[0].room == 2 && drawn_room != 2) {
				drawn_room = 2;
				load_room_links();
				need_full_redraw = 1;
			}
			if (need_full_redraw) ++full; else ++partial;
			int count = mobs_count;
			mob_type saved_mobs[14];
			memcpy(saved_mobs, mobs, sizeof(saved_mobs));
			render_floor_changes();
			char label[128];
			snprintf(label, sizeof(label), "%s-slab-gate-phase%d-land%d-view%d-step%02d", env, phase, landing, view, step);
			compare_floor(label);
			assert(mobs_count == count && memcmp(saved_mobs, mobs, sizeof(saved_mobs)) == 0);
			if (phase == 2 && landing == 1 && (step == 2 || step == 6 || step == 12)) save_audit(offscreen_surface, label);
			if (landing == 0 && view == 3 && step == 8 && phase >= 2) save_audit(offscreen_surface, label);
		}
		assert(mobs_count == 0);
		int result = level.fg[(land_room - 1) * 30 + land_pos] & 0x1F;
		assert(result == tiles_14_debris || result == tiles_5_stuck);
	}
	assert(full > 0 && partial > 0);
	printf("PASS: %s combined slab/gate/plate/room cases (%d full, %d incremental frames)\n", env, full, partial);
}

static void check_neighbor_slab_scope(void) {
	setup_floor(tiles_1_floor, tiles_1_floor, 2, 0);
	level.roomlinks[0].left = 2;
	load_room_links();
	curmob = (mob_type){.room = 2, .row = 2, .xh = 36};
	curr_tilepos = 29;
	redraw_at_cur_mob();
	assert(need_full_redraw);
	++checks;
	need_full_redraw = 0;
	curmob.room = 3;
	redraw_at_cur_mob();
	assert(!need_full_redraw);
	++checks;
	char saved_pack[POP_MAX_PATH];
	snprintf(saved_pack, sizeof(saved_pack), "%s", graphics_pack_name);
	snprintf(graphics_pack_name, sizeof(graphics_pack_name), "dos");
	curmob.room = 2;
	redraw_at_cur_mob();
	assert(!need_full_redraw);
	snprintf(graphics_pack_name, sizeof(graphics_pack_name), "%s", saved_pack);
	++checks;
	puts("PASS: neighbor slab refresh stays native-only and skips unrelated rooms");
}

void check_animated_architecture(const char* output) {
	setvbuf(stdout, NULL, _IONBF, 0);
	audit_output = output;
	checks = failures = 0;
	for (int palace = 0; palace < 2; ++palace) {
		load_lev_spr(palace ? 4 : 1);
		const char* env = palace ? "palace" : "dungeon";
		check_gates(env);
		check_exit_doors(env);
		check_chompers(env);
		check_mirror_appearance(env);
		check_gate_priority(env);
		check_exit_actors(env);
		check_chomper_actors(env);
		check_architecture_edges(env);
		check_slab_architecture(env);
	}
	puts("ARCHITECTURE: checking actual mirror-spawn event");
	check_mirror_spawn();
	check_architecture_scheduling();
	check_room_redraw_timing();
	check_native_shadow_xor();
	check_mirror_interactions();
	check_neighbor_slab_scope();
	puts("ARCHITECTURE: measuring redraw cost");
	benchmark_architecture();
	printf("ARCHITECTURE AUDIT: %d checks, %d failures\n", checks, failures);
	assert(failures == 0);
}

static void check_palace_fake_walls(void) {
	const byte cases[][4] = {
		{0,4,1,0}, {0,12,1,1}, {0,5,20,0}, {0,13,20,128},
		{0,50,20,0}, {0,51,20,1}, {0,52,20,2}, {0,53,20,3},
		{1,6,0,0}, {1,14,0,1}, {1,5,20,0}, {1,13,20,128},
		{1,50,20,0}, {1,51,20,1}, {1,52,20,2}, {1,53,20,3},
		{20,64,1,0}, {20,192,1,1}, {20,96,0,0}, {20,224,0,1}
	};
	for (int row = 0; row < 3; ++row) for (int edge = 0; edge < 3; ++edge)
	for (unsigned c = 0; c < sizeof(cases)/sizeof(cases[0]); ++c) {
		int col = edge == 0 ? 0 : edge == 1 ? 4 : 9;
		int pos = row * 10 + col;
		setup_floor(cases[c][0], tiles_1_floor, row, col);
		level.bg[pos] = cases[c][1];
		render_full_floor();
		assert((level.fg[pos] & 0x1F) == cases[c][0] && level.bg[pos] == cases[c][1]);
		SDL_Surface* fake = SDL_ConvertSurfaceFormat(offscreen_surface, SDL_PIXELFORMAT_ARGB8888, 0);
		level.fg[pos] = cases[c][2];
		level.bg[pos] = cases[c][3];
		render_full_floor();
		SDL_Surface* real = SDL_ConvertSurfaceFormat(offscreen_surface, SDL_PIXELFORMAT_ARGB8888, 0);
		int different = 0;
		for (int y = 0; y < 384; ++y)
			if (memcmp((byte*)fake->pixels+y*fake->pitch, (byte*)real->pixels+y*real->pitch, 640*4)) ++different;
		if (different) { printf("PALACE FAKE FAIL row%d col%d case%u\n", row,col,c); ++failures; }
		++checks;
		SDL_FreeSurface(fake);
		SDL_FreeSurface(real);
	}
}

static void check_palace_room_edges(void) {
	setup_floor(tiles_20_wall, tiles_1_floor, 1, 4);
	const link_type links[] = {
		{2,3,4,5}, {0,1,6,8}, {1,0,7,9}, {6,7,0,1}, {8,9,1,0},
		{0,4,0,2}, {4,0,0,3}, {0,5,2,0}, {5,0,3,0}
	};
	memcpy(level.roomlinks, links, sizeof(links));
	for (int room = 1; room <= 9; ++room) for (int pos = 0; pos < 30; ++pos) {
		level.fg[(room-1)*30+pos] = (pos%10 < 2 || pos%10 > 7 || pos/10 == 2) ? tiles_20_wall : tiles_1_floor;
		level.bg[(room-1)*30+pos] = pos%4;
	}
	for (int room = 1; room <= 9; ++room) {
		drawn_room = room;
		load_room_links();
		render_full_floor();
		char label[128];
		snprintf(label,sizeof(label),"palace-linked-room%d",room);
		save_audit(offscreen_surface,label);
		compare_floor(label);
		for (int pos = 0; pos < 30; ++pos) {
			redraw_height = 63;
			set_redraw_full(pos,1);
			set_wipe(pos,1);
			render_floor_changes();
			snprintf(label,sizeof(label),"palace-linked-room%d-tile%d",room,pos);
			compare_floor(label);
		}
	}
}

static void check_wall_pickups(const char* env) {
	/* Real animation/pickup events exercise both cells crossed by a wall's
	 * blue stripe, including fake walls and their no-blue modifier. */
	for (int row = 0; row < 3; ++row) for (int col = 2; col <= 3; ++col)
	for (int wall = 0; wall < 4; ++wall) for (int sword = 0; sword < 2; ++sword) {
		int pos = row * 10 + col, wallpos = row * 10 + 1;
		setup_floor(sword ? tiles_22_sword : tiles_10_potion, tiles_1_floor, row, col);
		level.fg[wallpos] = wall < 2 ? tiles_20_wall : tiles_0_empty;
		level.bg[wallpos] = wall < 2 ? (wall ? 128 : 0) : (wall == 2 ? 5 : 13);
		level.bg[pos] = sword ? 8 : 32;
		render_full_floor();
		for (int step = 0; step < 8; ++step) {
			select_audit_tile(pos);
			animate_tile();
			dword seed = random_seed;
			render_floor_changes();
			char label[120];
			snprintf(label,sizeof(label),"%s-wall%d-item%d-row%d-col%d-step%d",env,wall,sword,row,col,step);
			compare_floor(label);
			assert(random_seed == seed);
		}
		select_audit_tile(pos);
		curr_tilepos = pos; // Pickup uses the character's tile, not trob.tilepos.
		do_pickup(sword ? -1 : 1);
		assert(level.fg[pos] == tiles_1_floor && level.bg[pos] == 0);
		render_floor_changes();
		char label[120];
		snprintf(label,sizeof(label),"%s-wall%d-pickup%d-row%d-col%d",env,wall,sword,row,col);
		compare_floor(label);
		assert(level.fg[wallpos] == (wall < 2 ? tiles_20_wall : tiles_0_empty));
		assert(level.bg[wallpos] == (wall < 2 ? (wall ? 128 : 0) : (wall == 2 ? 5 : 13)));
	}
}

void check_palace_walls(const char* output) {
	audit_output = output;
	checks = failures = 0;
	load_lev_spr(4);
	check_palace_fake_walls();
	check_palace_room_edges();
	check_wall_pickups("palace");
	const int rooms[] = {1, 4, 12, 24};
	for (int r = 0; r < 4; ++r) {
		setup_floor(tiles_20_wall, tiles_20_wall, 1, 4);
		memset(level.fg, 0, sizeof(level.fg));
		memset(level.bg, 0, sizeof(level.bg));
		int room = rooms[r];
		for (int i = 0; i < 30; ++i) {
			level.fg[(room - 1) * 30 + i] = tiles_20_wall;
			level.bg[(room - 1) * 30 + i] = 3;
		}
		drawn_room = room;
		load_room_links();
		random_seed = 0x1234ABCD;
		render_full_floor();
		assert(random_seed == 0x1234ABCD);
		char path[160];
		snprintf(path, sizeof(path), "tools/mac_reference/wall-oracle/room-%d.rgb", room);
		FILE* input = fopen(path, "rb");
		assert(input && "Generate original-Mac palace wall fixtures first");
		byte* expected = malloc(640 * 384 * 3);
		assert(expected && fread(expected, 1, 640 * 384 * 3, input) == 640 * 384 * 3);
		assert(fgetc(input) == EOF);
		fclose(input);
		int different = 0;
		for (int y = 0; y < 384; ++y) for (int x = 0; x < 640; ++x) {
			Uint32 pixel = 0;
			memcpy(&pixel, (byte*)offscreen_surface->pixels + y * offscreen_surface->pitch + x * offscreen_surface->format->BytesPerPixel,
			       offscreen_surface->format->BytesPerPixel);
			byte red, green, blue;
			SDL_GetRGB(pixel, offscreen_surface->format, &red, &green, &blue);
			byte* rgb = expected + (y * 640 + x) * 3;
			if (red != rgb[0] || green != rgb[1] || blue != rgb[2]) ++different;
		}
		free(expected);
		if (different) { printf("PALACE REFERENCE FAIL room%d: %d pixels\n", room, different); ++failures; }
		++checks;
		char label[100];
		snprintf(label, sizeof(label), "palace-wall-reference-room%d", room);
		save_audit(offscreen_surface, label);
		for (int step = 0; step < 30; ++step) {
			redraw_height = 63;
			set_redraw_full(step, 1);
			set_wipe(step, 1);
			render_floor_changes();
			snprintf(label, sizeof(label), "palace-wall-room%d-redraw%d", room, step);
			compare_floor(label);
			assert(random_seed == 0x1234ABCD);
		}
	}
	/* Actor clipping and partial redraws at all four wall-neighbor states. */
	for (int row = 0; row < 3; ++row) for (int edge = 0; edge < 3; ++edge)
	for (int state = 0; state < 4; ++state) for (int facing = 0; facing < 2; ++facing) {
		int col = edge == 0 ? 0 : edge == 1 ? 4 : 9;
		setup_floor(tiles_20_wall, tiles_1_floor, row, col);
		if (col > 0) level.fg[row * 10 + col - 1] = (state & 2) ? tiles_20_wall : tiles_1_floor;
		if (col < 9) level.fg[row * 10 + col + 1] = (state & 1) ? tiles_20_wall : tiles_1_floor;
		level.bg[row * 10 + col] = state;
		Kid = (char_type){0};
		Kid.room = 1;
		Kid.curr_row = row;
		Kid.curr_col = col;
		Kid.x = x_bump[col + FIRST_ONSCREEN_COLUMN] + (facing ? 20 : -2);
		Kid.y = y_land[row + 1];
		Kid.frame = 15;
		Kid.direction = facing ? dir_FF_left : dir_0_right;
		Kid.alive = -1;
		jumped_through_mirror = united_with_shadow = 0;
		render_full_floor();
		render_people_changes();
		char label[100];
		snprintf(label, sizeof(label), "palace-wall-actor-row%d-col%d-state%d-face%d", row, col, state, facing);
		compare_people(label);
		if (row == 1 && col == 4 && state == 0) save_audit(offscreen_surface, label);
	}
	/* Actual game rooms: useful scene captures alongside the isolated oracle. */
	load_level();
	mobs_count = trobs_count = 0;
	for (int room = 1; room <= 4; ++room) {
		drawn_room = room;
		load_room_links();
		Kid.room = 0;
		Guard.direction = dir_56_none;
		render_full_floor();
		char label[100];
		snprintf(label, sizeof(label), "palace-level4-room%d", room);
		save_audit(offscreen_surface, label);
		compare_floor(label);
	}
	load_lev_spr(1);
	check_wall_pickups("dungeon");
	printf("PALACE AUDIT: %d checks, %d failures; includes dungeon pickup regression; original-Mac pixel fixtures and gameplay RNG preserved\n", checks, failures);
	assert(failures == 0);
}

#include "test_native_props.inc"
#include "test_native_cutscenes.inc"
#include "test_native_ui.inc"
#include "test_native_combat.inc"
#include "test_native_world.inc"

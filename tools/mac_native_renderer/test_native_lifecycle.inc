/* Test-only allocation/lifetime checks for release preparation. Pixel suites
 * independently cover the visual output; this path stays short under Memcheck. */
#include "pak.h"
void play_digi_sound(sound_buffer_type* buffer);
extern short digi_playing;

static void check_native_lifecycle(void) {
	int checks = 0;
	uint32_t audio_hash = 2166136261u;
	int audio_count = 0;
	for (unsigned i = 0; i < COUNT(sound_pointers); ++i) {
		sound_buffer_type* sound = sound_pointers[i];
		if (!sound || sound->type != sound_digi_converted) continue;
		const byte* samples = (const byte*)sound->converted.samples;
		for (int j = 0; j < sound->converted.length; ++j)
			audio_hash = (audio_hash ^ samples[j]) * 16777619u;
		++audio_count;
	}
	printf("Converted audio: %d buffers, FNV1a=%08x\n", audio_count, audio_hash);
	char saved_name[sizeof(graphics_pack_name)];
	memcpy(saved_name, graphics_pack_name, sizeof(saved_name));
	strcpy(graphics_pack_name, "MAC_NATIVE_FULL");
	assert(uses_mac_native_full()); ++checks;
	strcpy(graphics_pack_name, "dos");
	assert(!uses_mac_native_full()); ++checks;
	memcpy(graphics_pack_name, saved_name, sizeof(saved_name));

	/* Exercise the previously uninitialised no-HOME/no-override fallback without
	 * writing a save or making a directory in the user's home. */
	const char* home = getenv("HOME");
	const char* override = getenv("SDLPOP_SAVE_PATH");
	char* saved_home = home ? strdup(home) : NULL;
	char* saved_override = override ? strdup(override) : NULL;
	unsetenv("HOME"); unsetenv("SDLPOP_SAVE_PATH");
	char path[POP_MAX_PATH];
	assert(strcmp(get_writable_file_path(path, sizeof(path), "TEST.SAV"), "TEST.SAV") == 0); ++checks;
	if (saved_home) { setenv("HOME", saved_home, 1); free(saved_home); }
	if (saved_override) { setenv("SDLPOP_SAVE_PATH", saved_override, 1); free(saved_override); }

	/* Real menu pack switching replaces the PAK buffer and frees/reloads chtabs. */
	byte original_pack = graphics_pack_index;
	const char* packs[] = {"mac_native_full", "dos", "mac_prince", "snes_alt", "snes"};
	const char* names[] = {"Mac Native FULL v" REMASTERED_VERSION, "DOS", "SNES 1", "SNES 2", "SNES 3"};
	assert(num_available_paks == 5); ++checks;
	for (unsigned i = 0; i < COUNT(packs); ++i) {
		assert(strcmp(available_paks[i].id, packs[i]) == 0 &&
		       strcmp(available_paks[i].name, names[i]) == 0); ++checks;
	}
	assert(graphics_pack_index == 0); ++checks;
	assert(uses_mac_native_full()); ++checks;
	for (int repeat = 0; repeat < 8; ++repeat) {
		for (unsigned pack = 0; pack < COUNT(packs); ++pack) {
			int found = 0;
			for (byte i = 0; i < num_available_paks; ++i) {
				if (strcmp(available_paks[i].id, packs[pack]) != 0) continue;
				assert(switch_graphics_pack(i)); ++checks;
				assert(uses_mac_native_full() == (pack == 0)); ++checks;
				assert(chtab_addrs[id_chtab_2_kid] && chtab_addrs[id_chtab_6_environment]); ++checks;
				found = 1;
				break;
			}
			assert(found); ++checks;
		}
	}
	assert(switch_graphics_pack(original_pack)); ++checks;

	SDL_Surface* source = SDL_CreateRGBSurfaceWithFormat(0, 640, 400, 32, SDL_PIXELFORMAT_RGBA32);
	assert(source);
	SDL_FillRect(source, NULL, SDL_MapRGB(source->format, 70, 80, 90));
	is_cutscene = 0;
	for (int repeat = 0; repeat < 16; ++repeat) {
		palette_fade_type* fade = make_pal_buffer_fadein(source, 0xFFFF, 0);
		assert(fade->native_source == source && fade->native_snapshot == NULL); ++checks;
		fade->fade_pos = 2;
		fade_in_frame(fade);
		pal_restore_free_fadein(fade);
		fade = make_pal_buffer_fadeout(0xFFFF, 0);
		assert(fade->native_snapshot && fade->native_source == fade->native_snapshot); ++checks;
		assert(fade->native_snapshot->h == 400); ++checks;
		fade_out_frame(fade);
		pal_restore_free_fadeout(fade);
		is_cutscene = 1;
		fade = make_pal_buffer_fadeout(0xFFFF, 0);
		assert(fade->native_source == offscreen_surface && !fade->native_snapshot); ++checks;
		pal_restore_free_fadeout(fade);
		is_cutscene = 0;
	}
	SDL_FreeSurface(source);
	free_peels();
	sound_buffer_type* playing = calloc(1, sizeof(*playing));
	playing->type = sound_digi_converted;
	playing->converted.length = 4096;
	playing->converted.samples = calloc(1, 4096);
	play_digi_sound(playing);
	free_sound(playing);
	assert(!digi_playing); ++checks;
	stop_sounds();
	free_all_sounds();
	printf("LIFECYCLE AUDIT: %d checks, 0 failures; 40 pack switches and 48 fade allocations/restorations\n", checks);
}

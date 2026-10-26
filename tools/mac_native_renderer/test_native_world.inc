/* V17: real bundled level maps, separate from the synthetic subsystem oracles. */
static int world_checks,world_failures,world_rooms,world_steps;
static unsigned world_tiles[32];
static FILE* world_manifest;

static void world_check(int ok,const char* label) {
	++world_checks;
	if (!ok) { ++world_failures; printf("WORLD FAIL level%d room%d %s\n",current_level,drawn_room,label); }
}

static void world_consistent(const char* label) {
	SDL_Surface* consecutive=ui_clone(offscreen_surface);
	level_type map=level; dword seed=random_seed;
	render_full_floor();
	int equal=combat_same(consecutive,offscreen_surface);
	world_check(equal,label);
	if (!equal && world_failures<6) {
		char name[120]; snprintf(name,sizeof(name),"FAIL-world-L%d-R%d-consecutive",current_level,drawn_room); save_audit(consecutive,name);
		snprintf(name,sizeof(name),"FAIL-world-L%d-R%d-clean",current_level,drawn_room); save_audit(offscreen_surface,name);
	}
	world_check(memcmp(&map,&level,sizeof(level))==0 && seed==random_seed,"room composition preserves map and gameplay RNG");
	SDL_FreeSurface(consecutive);
}

static void world_capture(const char* name,int entry) {
	save_audit(entry?onscreen_surface_:offscreen_surface,name);
	if (world_manifest) fprintf(world_manifest,"%d,%d,%s,%d,%d,%d,%d,%d\n",current_level,drawn_room,name,entry,
	                          room_L,room_R,room_A,room_B);
}

void check_native_world(const char* output) {
	audit_output=output; world_checks=world_failures=world_rooms=world_steps=0;
	memset(world_tiles,0,sizeof(world_tiles));
	level_type saved_map=level; char_type kid=Kid,guard=Guard,character=Char;
	short saved_level=current_level,saved_next=next_level,saved_room=drawn_room;
	dword seed=random_seed;
	byte validate=is_validate_mode; is_validate_mode=1; stop_sounds();
	if (output) {
		char path[1024]; snprintf(path,sizeof(path),"%s/world-scenes.csv",output); world_manifest=fopen(path,"w"); assert(world_manifest);
		fprintf(world_manifest,"level,room,capture,entry,left,right,up,down\n");
	}
	for (int level_number=1;level_number<=14;++level_number) {
		load_lev_spr(level_number); load_level();
		mobs_count=trobs_count=0; is_blind_mode=upside_down=united_with_shadow=0;
		Kid.room=0; Guard.direction=dir_56_none;
		int rooms=0;
		for (int room=1;room<=24;++room) {
			int populated=room==level.start_room;
			for (int tile=0;tile<30;++tile) if (level.fg[(room-1)*30+tile]&31) populated=1;
			if (!populated) continue;
			++rooms; ++world_rooms;
			for (int tile=0;tile<30;++tile) ++world_tiles[level.fg[(room-1)*30+tile]&31];
			drawn_room=room; load_room_links(); mobs_count=trobs_count=0;
			render_full_floor();
			char name[120]; snprintf(name,sizeof(name),"world-level%02d-room%02d",level_number,room);
			world_capture(name,0); world_consistent("consecutive real-room background");
			/* Real animation setup includes the linked room's protruding torches. */
			get_room_address(drawn_room); anim_tile_modif();
			Char.room=drawn_room; Char.alive=-1;
			for (int row=0;row<3;++row) { Char.curr_row=row; start_chompers(); }
			/* The game initializes animation modifiers before its entry redraw. */
			render_full_floor();
			for (int tick=0;tick<4;++tick) {
				process_trobs();
				render_floor_changes(); world_consistent("real-room animation versus clean redraw"); ++world_steps;
			}
		}
		world_check(rooms>0,"level contributes real room captures");
		/* Use the actual entry initializer (including L1 fall/L13 run), then
		 * compose actors/HUD. This is an entry fixture, not a simulated playthrough. */
		load_level(); mobs_count=trobs_count=0; checkpoint=seamless=0;
		Kid=(char_type){0}; Guard=(char_type){0}; Guard.direction=dir_56_none;
		Char=(char_type){0}; have_sword=level_number==1?0:-1; do_startpos();
		drawn_room=Kid.room; load_room_links(); pos_guards(); enter_guard();
		get_room_address(drawn_room); loadkid(); anim_tile_modif(); start_chompers();
		current_target_surface=onscreen_surface_; ui_fill(onscreen_surface_,0);
		render_full_floor(); render_people_changes();
		copy_screen_rect(&rect_top); current_target_surface=onscreen_surface_;
		draw_kid_hp(hitp_curr,hitp_max); if (Guard.direction!=dir_56_none) draw_guard_hp(guardhp_curr,guardhp_max);
		char name[120]; snprintf(name,sizeof(name),"world-level%02d-entry",level_number); world_capture(name,1);
		SDL_Surface* actual=ui_clone(offscreen_surface); char_type entry_kid=Kid,entry_guard=Guard;
		render_full_floor(); render_people_changes();
		world_check(combat_same(actual,offscreen_surface),"entry actor layers match clean composition");
		world_check(memcmp(&entry_kid,&Kid,sizeof(Kid))==0 && memcmp(&entry_guard,&Guard,sizeof(Guard))==0,"entry rendering preserves gameplay actors");
		SDL_FreeSurface(actual);
	}
	if (world_manifest) { fclose(world_manifest); world_manifest=NULL; }
	if (output) {
		char path[1024]; snprintf(path,sizeof(path),"%s/world-tile-coverage.json",output); FILE* file=fopen(path,"w"); assert(file);
		fprintf(file,"{\"levels\":14,\"rooms\":%d,\"animation_steps\":%d,\"tile_counts\":[",world_rooms,world_steps);
		for (int t=0;t<32;++t) fprintf(file,"%s%u",t?",":"",world_tiles[t]);
		fprintf(file,"]}\n"); fclose(file);
	}
	load_lev_spr(saved_level); level=saved_map; Kid=kid; Guard=guard; Char=character;
	current_level=saved_level; next_level=saved_next; drawn_room=saved_room; load_room_links();
	random_seed=seed; is_validate_mode=validate; mobs_count=trobs_count=0;
	printf("WORLD AUDIT: %d checks, %d failures; 14 levels, %d real rooms, %d animation steps\n",world_checks,world_failures,world_rooms,world_steps);
	fflush(stdout); assert(world_failures==0);
}

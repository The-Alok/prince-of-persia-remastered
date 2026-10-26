/* Test-only companion to test_interactive_floors.c, sharing its scene and
 * pixel-comparison helpers. Never compiled into the normal game. */
extern short drawn_row, drawn_col, draw_bottom_y, draw_main_y;
extern word chtab_palette_bits;

static void check_potion_selection(const char* env, int palace) {
	/* Mac CODE 4:$41BC selects the tall bottle for native type $40 only;
	 * $42F8 suppresses bubbles for empty/life, uses green for feather/invert
	 * and blue for hurt/open. SDLPoP's effect type occupies bits 3..7. */
	const byte bottle[7] = {12,12,13,12,12,12,12};
	const sbyte bubble_offset[7] = {-1,0,-1,6,6,12,12};
	const byte bubble_frame[8] = {0,16,17,18,19,20,21,0};
	for (int kind = 0; kind < 7; ++kind) for (int phase = 0; phase < 8; ++phase) {
		setup_floor(tiles_10_potion,tiles_1_floor,1,4);
		level.bg[14] = kind*8+phase;
		render_full_floor();
		char label[100];
		snprintf(label,sizeof(label),"%s-potion-type%d-phase%d",env,kind,phase);
		if (phase == 3) save_audit(offscreen_surface,label);
		memset(table_counts,0,sizeof(table_counts));
		load_leftroom(); drawn_row=1; load_rowbelow(); drawn_col=4;
		draw_bottom_y=128; draw_main_y=125; load_curr_and_left_tile(); draw_tile();
		int expected_bottle=bottle[kind]+palace*2;
		int expected_bubble=bubble_offset[kind]<0 || !bubble_frame[phase] ? 0 : bubble_frame[phase]+bubble_offset[kind];
		int bottles=0, bubbles=0, other=0;
		for (int i=0;i<foretable_count;++i) {
			back_table_type* e=&foretable[i];
			if (e->chtab_id!=id_chtab_1_flameswordpotion) continue;
			int id=e->id+1;
			if (id==expected_bottle) ++bottles;
			else if (expected_bubble && id==expected_bubble) ++bubbles;
			else ++other;
		}
		++checks;
		if (bottles!=1 || bubbles!=(expected_bubble!=0) || other) {
			printf("PROP SELECTION FAIL %s: expected bottle%d bubble%d; counts %d/%d/%d\n",label,expected_bottle,expected_bubble,bottles,bubbles,other);
			++failures;
		}
		assert(level.bg[14]==kind*8+phase);
	}
}

static void check_potion_motion(const char* env) {
	const byte neighbors[]={tiles_0_empty,tiles_1_floor,tiles_3_pillar,tiles_20_wall};
	for (int row=0;row<3;++row) for (int edge=0;edge<3;++edge)
	for (int kind=0;kind<7;++kind) for (int n=0;n<4;++n) {
		int col=edge==0?0:edge==1?4:9, pos=row*10+col;
		setup_floor(tiles_10_potion,neighbors[n],row,col);
		level.bg[pos]=kind*8;
		render_full_floor();
		for (int step=0;step<8;++step) {
			select_audit_tile(pos); animate_tile();
			byte modifier=level.bg[pos]; dword seed=random_seed;
			render_floor_changes();
			char label[120]; snprintf(label,sizeof(label),"%s-potion%d-row%d-col%d-neighbor%d-step%d",env,kind,row,col,neighbors[n],step);
			compare_floor(label);
			assert(level.bg[pos]==modifier && random_seed==seed);
		}
		select_audit_tile(pos); curr_tilepos=pos;
		do_pickup(kind); render_floor_changes();
		char label[120]; snprintf(label,sizeof(label),"%s-potion%d-removed-row%d-col%d-neighbor%d",env,kind,row,col,neighbors[n]);
		compare_floor(label);
		assert(level.fg[pos]==tiles_1_floor && level.bg[pos]==0 && pickup_obj_type==kind);
	}
}

static void check_prop_torches(const char* env) {
	for (int row=0;row<3;++row) for (int edge=0;edge<4;++edge) for (int debris=0;debris<2;++debris) {
		int col=edge==0?0:edge==1?4:9, pos=row*10+col;
		setup_floor(debris?tiles_30_torch_with_debris:tiles_19_torch,tiles_1_floor,row,col);
		if (edge==3) { level.roomlinks[0].right=2; level.roomlinks[1].left=1; drawn_room=2; load_room_links(); }
		render_full_floor();
		for (int phase=0;phase<45;++phase) {
			select_audit_tile(pos);
			if (phase<9) { level.bg[pos]=phase; set_redraw_anim_right(); }
			else animate_tile();
			dword seed=random_seed; byte modifier=level.bg[pos];
			render_floor_changes();
			char label[120]; snprintf(label,sizeof(label),"%s-torch%d-row%d-edge%d-phase%d",env,debris,row,edge,phase);
			compare_floor(label);
			assert(random_seed==seed && level.bg[pos]==modifier);
			if (row==1 && edge==1 && phase<9) save_audit(offscreen_surface,label);
		}
	}
}

static void check_sword_edges(const char* env) {
	const byte neighbors[]={tiles_0_empty,tiles_1_floor,tiles_10_potion,tiles_3_pillar};
	for (int row=0;row<3;++row) for (int edge=0;edge<3;++edge) for (int n=0;n<4;++n) {
		int col=edge==0?0:edge==1?4:9, pos=row*10+col;
		setup_floor(tiles_22_sword,neighbors[n],row,col);
		level.bg[pos]=2;
		if (n==2 && col<9) level.bg[pos+1]=3*8+3;
		render_full_floor();
		for (int step=0;step<16;++step) {
			select_audit_tile(pos); animate_tile();
			dword seed=random_seed; byte modifier=level.bg[pos];
			render_floor_changes();
			char label[120]; snprintf(label,sizeof(label),"%s-sword-row%d-col%d-neighbor%d-step%d",env,row,col,neighbors[n],step);
			compare_floor(label);
			assert(random_seed==seed && level.bg[pos]==modifier);
			if (row==1 && col==4 && n==0 && step<2) save_audit(offscreen_surface,label);
		}
		select_audit_tile(pos); curr_tilepos=pos; do_pickup(-1);
		render_floor_changes();
		char label[120]; snprintf(label,sizeof(label),"%s-sword-removed-row%d-col%d-neighbor%d",env,row,col,neighbors[n]);
		compare_floor(label);
		assert(level.fg[pos]==tiles_1_floor && level.bg[pos]==0 && pickup_obj_type==-1);
	}
}

static Uint32 prop_pixel(SDL_Surface* surface, int x, int y) {
	Uint32 pixel=0;
	memcpy(&pixel,(byte*)surface->pixels+y*surface->pitch+x*surface->format->BytesPerPixel,surface->format->BytesPerPixel);
	return pixel;
}

static void check_lattice_pixels(SDL_Surface* background, int tile, const char* label) {
	const byte ids[]={1,3,4,5};
	image_type* image=get_image(id_chtab_6_environment,ids[tile-tiles_26_lattice_down]-1);
	assert(image && is_native_resolution_image(image));
	short ox,oy; assert(get_native_image_origin(image,&ox,&oy));
	int left=256-ox,top=2*(126-4+(tile==tiles_26_lattice_down?0:-10))-oy;
	Uint32 key=0; int has_key=SDL_GetColorKey(image,&key)==0;
	int different=0;
	for (int y=0;y<image->h;++y) for (int x=0;x<image->w;++x) {
		Uint32 pixel=prop_pixel(image,x,y); Uint8 r,g,b,a;
		SDL_GetRGBA(pixel,image->format,&r,&g,&b,&a);
		if (!a || (has_key && pixel==key)) continue;
		if (prop_pixel(background,left+x,top+y)!=prop_pixel(offscreen_surface,left+x,top+y)) ++different;
	}
	++checks;
	if (different) { printf("PROP OCCLUSION FAIL %s: %d opaque lattice pixels overwritten\n",label,different); ++failures; }
}

static void check_static_props(const char* env) {
	const byte tiles[]={tiles_21_skeleton,tiles_14_debris,tiles_23_balcony_left,tiles_24_balcony_right,
	 tiles_25_lattice_pillar,tiles_26_lattice_down,tiles_27_lattice_small,tiles_28_lattice_left,tiles_29_lattice_right};
	for (unsigned t=0;t<sizeof(tiles);++t) for (int row=0;row<3;++row) for (int edge=0;edge<3;++edge) {
		int col=edge==0?0:edge==1?4:9, pos=row*10+col;
		setup_floor(tiles[t],tiles_1_floor,row,col);
		if (tiles[t]==tiles_23_balcony_left && col<9) level.fg[pos+1]=tiles_24_balcony_right;
		if (tiles[t]==tiles_24_balcony_right && col>0) level.fg[pos-1]=tiles_23_balcony_left;
		if (tiles[t]==tiles_26_lattice_down && col<9) level.fg[pos+1]=tiles_12_doortop;
		render_full_floor();
		for (int step=0;step<2;++step) {
			redraw_height=63; set_wipe(pos,1); set_redraw_full(pos,1);
			if (step && col<9) { set_wipe(pos+1,1); set_redraw_full(pos+1,1); }
			render_floor_changes();
			char label[100]; snprintf(label,sizeof(label),"%s-prop%d-row%d-col%d-refresh%d",env,tiles[t],row,col,step);
			compare_floor(label);
			if (row==1 && col==4 && !step) save_audit(offscreen_surface,label);
		}
	}
}

static void check_lattice_layers(const char* env) {
#ifdef USE_SUPER_HIGH_JUMP
	byte saved=fixes->enable_super_high_jump;
	for (int enabled=0;enabled<2;++enabled) for (int kind=0;kind<5;++kind) {
		fixes->enable_super_high_jump=enabled;
		int tile=kind<4?tiles_26_lattice_down+kind:tiles_12_doortop;
		const byte bases[]={1,3,4,5,6};
		int base=bases[kind];
		setup_floor(tile,tiles_1_floor,1,4);
		if (kind==4) level.fg[13]=tiles_26_lattice_down;
		memset(table_counts,0,sizeof(table_counts));
		load_leftroom(); drawn_row=1; load_rowbelow(); drawn_col=4;
		draw_bottom_y=128; draw_main_y=125; load_curr_and_left_tile(); draw_tile();
		int back=0,fore=0;
		for (int i=0;i<backtable_count;++i) if (backtable[i].chtab_id==id_chtab_6_environment && backtable[i].id==base-1) ++back;
		for (int i=0;i<foretable_count;++i) if (foretable[i].chtab_id==id_chtab_6_environment && foretable[i].id==base-1) ++fore;
		++checks;
		if (back!=!enabled || fore!=enabled) {
			printf("PROP LAYER FAIL %s lattice%d enabled%d: back%d fore%d\n",env,kind,enabled,back,fore); ++failures;
		}
	}
	fixes->enable_super_high_jump=saved;
#endif
}

static void check_prop_actors(const char* env) {
	byte saved_jump_fix=fixes->enable_super_high_jump;
	fixes->enable_super_high_jump=1;
	const byte tiles[]={tiles_10_potion,tiles_22_sword,tiles_19_torch,tiles_30_torch_with_debris,
	 tiles_21_skeleton,tiles_14_debris,tiles_23_balcony_left,tiles_25_lattice_pillar,
	 tiles_26_lattice_down,tiles_27_lattice_small,tiles_28_lattice_left,tiles_29_lattice_right};
	for (unsigned t=0;t<sizeof(tiles);++t) for (int facing=0;facing<2;++facing)
	for (int pose=0;pose<3;++pose) {
		setup_floor(tiles[t],tiles_1_floor,1,4);
		if (tiles[t]==tiles_10_potion) level.bg[14]=2*8+3;
		if (tiles[t]==tiles_23_balcony_left) level.fg[15]=tiles_24_balcony_right;
		Kid=(char_type){0}; Kid.room=1; Kid.curr_row=1; Kid.curr_col=4;
		Kid.x=x_bump[4+FIRST_ONSCREEN_COLUMN]+6; Kid.y=y_land[2]-(pose==2?12:0);
		Kid.frame=pose==0?15:pose==1?109:79;
		Kid.action=pose==2?actions_4_in_freefall:actions_0_stand;
		Kid.direction=facing?dir_FF_left:dir_0_right; Kid.alive=-1;
		jumped_through_mirror=united_with_shadow=0;
		render_full_floor();
		SDL_Surface* background=SDL_ConvertSurface(offscreen_surface,offscreen_surface->format,0);
		assert(background);
		render_people_changes();
		char label[120]; snprintf(label,sizeof(label),"%s-prop%d-actor-face%d-pose%d",env,tiles[t],facing,pose);
		compare_people(label);
		if (custom->tbl_level_type[current_level] && tiles[t]>=tiles_26_lattice_down && tiles[t]<=tiles_29_lattice_right)
			check_lattice_pixels(background,tiles[t],label);
		SDL_FreeSurface(background);
		if (facing==0 && pose!=1) save_audit(offscreen_surface,label);
		Kid.x+=2; render_people_changes(); compare_people(label);
	}
	fixes->enable_super_high_jump=saved_jump_fix;
}

static void check_skeleton_wake(void) {
	load_lev_spr(custom->skeleton_level);
	int row=custom->skeleton_row, col=custom->skeleton_column, room=custom->skeleton_room;
	assert(room==1); // Fixture for the bundled levelset.
	setup_floor(tiles_21_skeleton,tiles_1_floor,row,col);
	Kid=(char_type){0}; Kid.room=room; Kid.curr_row=row;
	Kid.curr_col=custom->skeleton_trigger_column_1;
	Kid.x=x_bump[Kid.curr_col+FIRST_ONSCREEN_COLUMN]+14; Kid.y=y_land[row+1];
	Kid.frame=15; Kid.direction=dir_0_right; Kid.alive=-1;
	leveldoor_open=1;
	Guard.direction=dir_56_none; render_full_floor();
	check_skel();
	assert(Guard.charid==charid_4_skeleton && Guard.alive==-1 && guardhp_curr==3);
	assert(level.fg[row*10+col]==tiles_1_floor);
	render_floor_changes(); compare_floor("dungeon-skeleton-wake-background");
	render_people_changes(); compare_people("dungeon-skeleton-wake-actors");
	save_audit(offscreen_surface,"dungeon-skeleton-wake-actors");
}

static void check_prop_loader_scope(void) {
	char saved_pack[POP_MAX_PATH];
	snprintf(saved_pack,sizeof(saved_pack),"%s",graphics_pack_name);
	word saved_palette_bits=chtab_palette_bits;
	snprintf(graphics_pack_name,sizeof(graphics_pack_name),"dos");
	dat_type* dat=open_dat("PRINCE.DAT",'G');
	chtab_type* sprites=load_sprites_from_file(150,1<<3,1);
	assert(sprites && sprites->n_images==23);
	free_chtab(sprites); close_dat(dat);
	chtab_palette_bits=saved_palette_bits;
	snprintf(graphics_pack_name,sizeof(graphics_pack_name),"%s",saved_pack);
	assert(chtab_addrs[id_chtab_1_flameswordpotion]->n_images==33);
	++checks;
}

void check_native_props(const char* output) {
	setvbuf(stdout,NULL,_IONBF,0); audit_output=output; checks=failures=0;
	assert(chtab_addrs[id_chtab_1_flameswordpotion]->n_images==33);
	for (int palace=0;palace<2;++palace) {
		load_lev_spr(palace?4:1);
		const char* env=palace?"palace":"dungeon";
		check_potion_selection(env,palace);
		check_potion_motion(env);
		check_prop_torches(env);
		check_sword_edges(env);
		check_static_props(env);
		if (palace) check_lattice_layers(env); // Lattice SHPD art is palace-only.
		check_prop_actors(env);
	}
	check_skeleton_wake();
	check_prop_loader_scope();
	printf("PROPS AUDIT: %d checks, %d failures\n",checks,failures);
	assert(failures==0);
}

/* Original-Mac pixel oracle from build_combat_reference.py, not runtime tables. */
typedef struct combat_case { short family,frame,right,body,sword,raw_x,bx,by,wx,wy; } combat_case;
static int combat_checks,combat_failures;
static combat_case combat_cases[512];
static int combat_count;
static const char* combat_folders[]={"KID","GUARD","FAT","VIZIER","SHADOW","SKEL"};

static void combat_check(int ok,const char* label) {
	++combat_checks;
	if (!ok) { if (combat_failures<10) printf("COMBAT FAIL %s\n",label); ++combat_failures; }
}

static int combat_same(SDL_Surface* a,SDL_Surface* b) {
	if (a->w!=b->w || a->h!=b->h) return 0;
	SDL_Surface* aa=SDL_ConvertSurfaceFormat(a,SDL_PIXELFORMAT_ARGB8888,0);
	SDL_Surface* bb=SDL_ConvertSurfaceFormat(b,SDL_PIXELFORMAT_ARGB8888,0);
	assert(aa && bb); int same=1;
	for (int y=0;y<aa->h && same;++y) {
		Uint32* ap=(Uint32*)((byte*)aa->pixels+y*aa->pitch);
		Uint32* bp=(Uint32*)((byte*)bb->pixels+y*bb->pitch);
		for (int x=0;x<aa->w;++x) if ((ap[x]&0xffffff)!=(bp[x]&0xffffff)) { same=0; break; }
	}
	SDL_FreeSurface(aa); SDL_FreeSurface(bb); return same;
}

static void combat_render(void) {
	free_peels(); memset(table_counts,0,sizeof(table_counts));
	current_target_surface=offscreen_surface; SDL_SetClipRect(offscreen_surface,NULL);
	SDL_FillRect(offscreen_surface,NULL,SDL_MapRGB(offscreen_surface->format,27,23,51));
	load_frame_to_obj(); obj_tilepos=30;
	add_objtable(Char.charid==charid_0_kid?0:2); add_sword_to_objtable();
	draw_objtable_items_at_tile(30); __real_draw_tables();
}

static SDL_Surface* combat_reference(combat_case* c) {
	char path[240]; snprintf(path,sizeof(path),"tools/mac_reference/combat-oracle/%s-%03d-%s.png",
	 c->family==0?"kid":c->family==1?"guard":c->family==2?"fat":c->family==3?"vizier":c->family==4?"shadow":"skel",
	 c->frame,c->right?"right":"left");
	SDL_Surface* image=IMG_Load(path); assert(image); return image;
}

static void combat_compare(combat_case* c,SDL_Surface* reference,const char* label) {
	char_type before=Char;
	combat_render();
	int raw=(Char.x+(c->right?cur_frame.dx:-cur_frame.dx))*2-116;
	if ((sbyte)(cur_frame.flags^Char.direction)>=0) ++raw;
	int shift=calc_screen_x_coord(raw)-c->bx,dy=Char.y-168;
	objtable_type* weapon=objtable_count==2?&objtable[1]:NULL;
	combat_check(weapon && weapon->chtab_id==id_chtab_0_sword && weapon->id+701==c->sword &&
	             weapon->xh*8+weapon->xl==c->wx+shift && weapon->y==c->wy+dy &&
	             memcmp(&before,&Char,sizeof(Char))==0,label);
	SDL_Surface* expected=SDL_ConvertSurfaceFormat(reference,SDL_PIXELFORMAT_ARGB8888,0);
	assert(expected);
	SDL_FillRect(expected,NULL,SDL_MapRGB(expected->format,27,23,51));
	SDL_Rect pos={shift*2,dy*2,0,0}; assert(SDL_BlitSurface(reference,NULL,expected,&pos)==0);
	int equal=combat_same(expected,offscreen_surface); combat_check(equal,label);
	if (Char.x==114 && Char.y==168) {
		char name[120]; snprintf(name,sizeof(name),"combat-%s-%03d-%s",combat_folders[c->family],c->frame,c->right?"right":"left");
		save_audit(offscreen_surface,name);
	}
	SDL_FreeSurface(expected);
}

static void combat_sequences(int family) {
	const int sequences[]={seq_55_draw_sword,seq_56_guard_forward_with_sword,seq_57_back_with_sword,
	 seq_58_guard_strike,seq_61_parry_after_strike,seq_62_parry,seq_66_strike_after_parry,
	 seq_75_strike,seq_86_forward_with_sword,seq_91_get_sword,seq_92_put_sword_away};
	for (unsigned s=0;s<sizeof(sequences)/sizeof(*sequences);++s) {
		if (family && (sequences[s]==seq_55_draw_sword || sequences[s]==seq_75_strike ||
		               sequences[s]==seq_86_forward_with_sword || sequences[s]==seq_91_get_sword || sequences[s]==seq_92_put_sword_away)) continue;
		if (!family && (sequences[s]==seq_56_guard_forward_with_sword || sequences[s]==seq_58_guard_strike)) continue;
		for (int right=0;right<2;++right) {
			Char=(char_type){0}; Char.charid=family?charid_2_guard:charid_0_kid;
			Char.x=114; Char.y=168; Char.direction=right?dir_0_right:dir_FF_left;
			Char.alive=-1; Char.room=drawn_room=1; Char.sword=sword_2_drawn;
			seqtbl_offset_char(sequences[s]);
			for (int tick=0;tick<20;++tick) {
				play_seq(); load_frame();
				combat_case* found=NULL;
				for (int i=0;i<combat_count;++i) if (combat_cases[i].family==family && combat_cases[i].frame==Char.frame && combat_cases[i].right==(Char.direction==dir_0_right)) { found=&combat_cases[i]; break; }
				if (found) {
					SDL_Surface* reference=combat_reference(found);
					combat_compare(found,reference,"actual pickup/draw/attack/parry/step/sheath sequence"); SDL_FreeSurface(reference);
				} else if (!(cur_frame.sword&63)) {
					combat_render(); combat_check(objtable_count==1,"sheathed/no-weapon sequence frame adds no sword");
				}
			}
		}
	}
}

void check_native_combat(const char* output) {
	audit_output=output; combat_checks=combat_failures=0;
	FILE* f=fopen("tools/mac_reference/combat-oracle/cases.bin","rb"); assert(f);
	combat_count=0;
	while (combat_count<512 && fread(&combat_cases[combat_count],sizeof(combat_case),1,f)==1) {
		short* fields=(short*)&combat_cases[combat_count];
		for (int i=0;i<10;++i) fields[i]=SDL_SwapLE16(fields[i]);
		++combat_count;
	}
	fclose(f); assert(combat_count==322);
	char_type kid=Kid,guard=Guard,character=Char;
	short saved_level=current_level,saved_next=next_level;
	dword seed=random_seed;
	int loaded=-1; const int levels[]={1,1,6,13,12,3};
	const int positions[]={70,113,114,115,200};
	free_peels(); is_blind_mode=united_with_shadow=0;
	if (offscreen_surface->h!=384) { free_surface(offscreen_surface); offscreen_surface=make_offscreen_buffer(&rect_top); }
	for (int i=0;i<combat_count;++i) {
		combat_case* c=&combat_cases[i];
		if (loaded!=c->family) { load_lev_spr(levels[c->family]); loaded=c->family; }
		SDL_Surface* reference=combat_reference(c);
		for (unsigned p=0;p<sizeof(positions)/sizeof(*positions);++p) {
			Char=(char_type){0}; Char.charid=c->family==0?charid_0_kid:c->family==5?charid_4_skeleton:charid_2_guard;
			Char.frame=c->frame; Char.x=positions[p]; Char.y=168;
			Char.direction=c->right?dir_0_right:dir_FF_left; Char.alive=-1; Char.sword=sword_2_drawn;
			char label[120]; snprintf(label,sizeof(label),"%s frame%d %s x%d",combat_folders[c->family],c->frame,c->right?"right":"left",Char.x);
			combat_compare(c,reference,label);
		}
		SDL_FreeSurface(reference);
	}
	if (!getenv("SDLPOP_COMBAT_BASELINE")) {
		load_lev_spr(1); combat_sequences(0); combat_sequences(1);
	}
	Kid=kid; Guard=guard; Char=character; current_level=saved_level; next_level=saved_next; random_seed=seed;
	printf("COMBAT AUDIT: %d checks, %d failures; 322 source composites, both facings and five X phases/edges\n",combat_checks,combat_failures);
	fflush(stdout); assert(combat_failures==0);
}

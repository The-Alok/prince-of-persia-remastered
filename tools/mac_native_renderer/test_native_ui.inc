/* Test-only v16 UI audit; run before cutscenes replace the room sprite tables. */
void draw_pause_menu(void);
void draw_settings_menu(void);
void draw_overlay(void);
extern rgb_type palette[256];
extern struct star_type { short x,y,color; } stars[6];
static int ui_checks, ui_failures;

static void ui_check(int ok, const char* label) {
	++ui_checks;
	if (!ok) { if (ui_failures < 16) printf("UI FAIL: %s\n", label); ++ui_failures; }
}

static Uint32 ui_pixel(SDL_Surface* s, int x, int y) {
	Uint32 pixel=0; Uint8 r,g,b,a;
	memcpy(&pixel,(byte*)s->pixels+y*s->pitch+x*s->format->BytesPerPixel,s->format->BytesPerPixel);
	SDL_GetRGBA(pixel,s->format,&r,&g,&b,&a);
	return ((Uint32)a<<24)|(r<<16)|(g<<8)|b;
}

static int ui_same(SDL_Surface* a, SDL_Surface* b) {
	if (a->w!=b->w || a->h!=b->h) return 0;
	for (int y=0;y<a->h;++y) for (int x=0;x<a->w;++x)
		if ((ui_pixel(a,x,y)&0xffffff)!=(ui_pixel(b,x,y)&0xffffff)) return 0;
	return 1;
}

static SDL_Surface* ui_clone(SDL_Surface* s) {
	SDL_Surface* result=SDL_ConvertSurfaceFormat(s,SDL_PIXELFORMAT_ARGB8888,0);
	assert(result); SDL_SetSurfaceBlendMode(result,SDL_BLENDMODE_NONE); return result;
}

static void ui_fill(SDL_Surface* s, Uint32 rgb) {
	SDL_SetClipRect(s,NULL);
	SDL_FillRect(s,NULL,SDL_MapRGB(s->format,rgb>>16,(rgb>>8)&255,rgb&255));
}

static void ui_stamp(SDL_Surface* dst, SDL_Surface* source, int x, int y) {
	SDL_Rect pos={x,y,0,0}; assert(SDL_BlitSurface(source,NULL,dst,&pos)==0);
}

static void ui_health(void) {
	SDL_Surface* full=IMG_Load("tools/mac_reference/source-images/KID/0617_macgroup400.png");
	SDL_Surface* empty=IMG_Load("tools/mac_reference/source-images/KID/0618_macgroup400.png");
	assert(full && empty);
	SDL_SetSurfaceBlendMode(full,SDL_BLENDMODE_NONE);
	SDL_SetSurfaceBlendMode(empty,SDL_BLENDMODE_NONE);
	/* Retained SDLPoP HUD placement: physical x=16*i, y=384. Native art is
	 * compared directly; this does not assert original Mac menu/HUD layout. */
	for (int maximum=1;maximum<=10;++maximum) for (int hp=0;hp<=maximum;++hp) {
		ui_fill(onscreen_surface_,0); current_target_surface=onscreen_surface_;
		draw_kid_hp(hp,maximum);
		SDL_Surface* expected=ui_clone(onscreen_surface_); ui_fill(expected,0);
		for (int i=0;i<maximum;++i) ui_stamp(expected,i<hp?full:empty,16*i,384);
		ui_check(ui_same(expected,onscreen_surface_),"Kid native flask pixels, counts and screen edges");
		SDL_FreeSurface(expected);
	}
	ui_fill(onscreen_surface_,0); draw_kid_hp(10,10); draw_kid_hp(3,10);
	SDL_Surface* consecutive=ui_clone(onscreen_surface_);
	ui_fill(onscreen_surface_,0); draw_kid_hp(3,10);
	ui_check(ui_same(consecutive,onscreen_surface_),"Kid damage clears previous full flasks");
	SDL_FreeSurface(consecutive);
	/* Exercise each installed guard-family icon against its own source PNG. */
	const int levels[]={1,6,12,13}; const char* folders[]={"GUARD","FAT","SHADOW","VIZIER"};
	const int groups[]={1750,3750,4750,5750};
	for (int family=0;family<4;++family) {
		load_lev_spr(levels[family]);
		Guard.charid=family==2?charid_1_shadow:charid_2_guard;
		char path[200]; snprintf(path,sizeof(path),"tools/mac_reference/source-images/%s/0751_macgroup%d.png",folders[family],groups[family]);
		SDL_Surface* icon=IMG_Load(path); assert(icon); SDL_SetSurfaceBlendMode(icon,SDL_BLENDMODE_NONE);
		char loaded_name[80]; snprintf(loaded_name,sizeof(loaded_name),"ui-loaded-%s",folders[family]);
		save_audit(chtab_addrs[id_chtab_5_guard]->images[0],loaded_name);
		for (int hp=0;hp<=10;++hp) {
			ui_fill(onscreen_surface_,0); current_target_surface=onscreen_surface_;
			draw_guard_hp(10,10); draw_guard_hp(hp,10);
			SDL_Surface* expected=ui_clone(onscreen_surface_); ui_fill(expected,0);
			for (int i=0;i<hp;++i) ui_stamp(expected,icon,624-16*i,384);
			ui_check(ui_same(expected,onscreen_surface_),"guard-family icon, damage and right edge");
			if (hp==3 && !ui_same(expected,onscreen_surface_)) {
				char name[80]; snprintf(name,sizeof(name),"FAIL-ui-guard-%s-expected",folders[family]); save_audit(expected,name);
			}
			SDL_FreeSurface(expected);
		}
		ui_fill(onscreen_surface_,0); draw_kid_hp(3,3); draw_guard_hp(3,3);
		char name[80]; snprintf(name,sizeof(name),"ui-health-%s",folders[family]); save_audit(onscreen_surface_,name);
		SDL_FreeSurface(icon);
	}
	const int chars[]={charid_4_skeleton,charid_24_mouse,charid_1_shadow};
	current_level=1;
	for (int i=0;i<3;++i) {
		Guard.charid=chars[i]; ui_fill(onscreen_surface_,0x142638);
		SDL_Surface* before=ui_clone(onscreen_surface_); draw_guard_hp(3,3);
		ui_check(ui_same(before,onscreen_surface_),"non-HUD characters do not draw health"); SDL_FreeSurface(before);
	}
	load_lev_spr(1); Guard.charid=charid_2_guard;
	hitp_curr=guardhp_curr=1; hitp_max=guardhp_max=3; hitp_delta=guardhp_delta=1;
	for (int blink=0;blink<2;++blink) {
		global_blink_state=blink; rem_tick=(rem_tick&~1)|blink;
		ui_fill(onscreen_surface_,0); draw_hp();
		SDL_Surface* actual=ui_clone(onscreen_surface_);
		ui_fill(onscreen_surface_,0);
		int on=fixes->fix_one_hp_stops_blinking?global_blink_state:(rem_tick&1);
		draw_kid_hp(on?1:0,3); draw_guard_hp(on?1:0,3);
		ui_check(ui_same(actual,onscreen_surface_),"real low-health blink clears and restores icons");
		SDL_FreeSurface(actual);
	}
	SDL_FreeSurface(full); SDL_FreeSurface(empty);
	printf("UI health: %d checks, %d failures\n",ui_checks,ui_failures);
}

static void ui_text_menu_dialog(void) {
	const char* messages[]={"LEVEL 4","60 MINUTES LEFT","GAME PAUSED","QUICKSAVE","TIME HAS EXPIRED!","WORD 12 LINE 3 PAGE 10"};
	for (unsigned i=0;i<sizeof(messages)/sizeof(*messages);++i) {
		ui_fill(onscreen_surface_,0x142638); current_target_surface=onscreen_surface_;
		display_text_bottom(messages[i]);
		int ink=0,escaped=0;
		for (int y=0;y<400;++y) for (int x=0;x<640;++x) {
			Uint32 c=ui_pixel(onscreen_surface_,x,y)&0xffffff;
			if (x>=140 && x<500 && y>=386) ink+=c!=0 && c!=0x142638;
			else escaped+=c!=0x142638;
		}
		ui_check(ink>0 && !escaped,"bottom message visible and confined to footer");
		char name[60]; snprintf(name,sizeof(name),"ui-bottom-message-%u",i); save_audit(onscreen_surface_,name);
		erase_bottom_text(0); int cleared=1;
		for (int y=386;y<400;++y) for (int x=140;x<500;++x)
			if (ui_pixel(onscreen_surface_,x,y)&0xffffff) cleared=0;
		ui_check(cleared,"erasing bottom message clears glyphs");
	}
	/* Menu is the retained SDLPoP overlay; exercise render functions with no
	 * input, then check transparent footer and restoration of the base image. */
	ui_fill(onscreen_surface_,0x506478); display_text_bottom("GAME PAUSED");
	SDL_Surface* base=ui_clone(onscreen_surface_);
	clear_menu_controls(); have_mouse_input=false; pressed_enter=false; mouse_clicked=false;
	font_type* font=textstate.ptr_font; textstate.ptr_font=&hc_small_font;
	current_target_surface=overlay_surface; SDL_SetClipRect(overlay_surface,NULL);
	draw_pause_menu();
	int clear=1;
	for (int y=386;y<400;++y) for (int x=140;x<500;++x)
		if (ui_pixel(overlay_surface,x,y)>>24) clear=0;
	ui_check(clear,"pause overlay leaves footer transparent");
	is_paused=1; is_menu_shown=1; draw_overlay();
	save_audit(get_final_surface(),"ui-pause-menu");
	ui_check(ui_same(base,onscreen_surface_),"menu composition preserves base framebuffer");
	is_paused=0; is_menu_shown=0; draw_overlay();
	ui_check(get_final_surface()==onscreen_surface_ && ui_same(base,onscreen_surface_),"closing menu reveals original screen");
	current_target_surface=overlay_surface; draw_settings_menu(); save_audit(overlay_surface,"ui-settings-overlay");
	textstate.ptr_font=font;
	current_target_surface=onscreen_surface_;
	peel_type* peel=read_peel_from_screen(&copyprot_dialog->peel_rect);
	draw_dialog_frame(copyprot_dialog);
	rect_type rect; shrink2_rect(&rect,&copyprot_dialog->text_rect,2,1);
	show_text_with_color(&rect,halign_center,valign_middle,"Word 12, line 3, page 10.\nPress any key to continue.",color_15_brightwhite);
	save_audit(onscreen_surface_,"ui-copyprot-dialog");
	ui_check(!ui_same(base,onscreen_surface_),"dialog frame and copy-protection text render");
	restore_peel(peel); ui_check(ui_same(base,onscreen_surface_),"dialog peel restores every background pixel");
	SDL_FreeSurface(base);
	printf("UI text/menu/dialog cumulative: %d checks, %d failures\n",ui_checks,ui_failures);
}

static void ui_pattern(SDL_Surface* s) {
	for (int y=0;y<s->h;++y) for (int x=0;x<s->w;++x) {
		Uint32 p=SDL_MapRGBA(s->format,(7*x+3*y)&255,(x^y)&255,(x+3*y)&255, s->format->Amask?(x+y)&255:255);
		memcpy((byte*)s->pixels+y*s->pitch+x*s->format->BytesPerPixel,&p,s->format->BytesPerPixel);
	}
}

static int ui_fade_pixels(SDL_Surface* source, SDL_Surface* before, int amount, SDL_Rect clip) {
	for (int y=0;y<onscreen_surface_->h;++y) for (int x=0;x<onscreen_surface_->w;++x) {
		Uint32 expected=ui_pixel(before,x,y);
		if (x<source->w && y<source->h && x>=clip.x && x<clip.x+clip.w && y>=clip.y && y<clip.y+clip.h) {
			Uint32 p=ui_pixel(source,x,y); int r=(p>>16)&255,g=(p>>8)&255,b=p&255;
			expected=(p&0xff000000)|((r>amount?r-amount:0)<<16)|((g>amount?g-amount:0)<<8)|(b>amount?b-amount:0);
		}
		Uint32 actual=ui_pixel(onscreen_surface_,x,y);
		Uint32 mask=onscreen_surface_->format->Amask?0xffffffff:0xffffff;
		if ((actual&mask)!=(expected&mask)) return 0;
	}
	return 1;
}

static void ui_fades(void) {
	int baseline=getenv("SDLPOP_UI_BASELINE")!=NULL;
	SDL_Surface* original_on=onscreen_surface_;
	SDL_Surface* original_off=offscreen_surface;
	byte validate=is_validate_mode; is_validate_mode=1;
	for (int mode=0;mode<(baseline?1:3);++mode) {
		if (mode==2) {
			onscreen_surface_=SDL_CreateRGBSurfaceWithFormat(0,640,400,32,SDL_PIXELFORMAT_ARGB8888);
			assert(onscreen_surface_);
		}
		SDL_Surface* source=SDL_CreateRGBSurfaceWithFormat(0,mode==1?637:640,mode==1?399:400,
		                                               mode?32:24,mode?SDL_PIXELFORMAT_ABGR8888:SDL_PIXELFORMAT_RGB24);
		assert(source); ui_pattern(source);
		offscreen_surface=make_offscreen_buffer(&rect_top); ui_fill(offscreen_surface,0xF023A8);
		ui_fill(onscreen_surface_,0x341278);
		SDL_Rect clip=mode==1?(SDL_Rect){3,7,627,387}:(SDL_Rect){0,0,640,400};
		SDL_SetClipRect(onscreen_surface_,&clip);
		SDL_Surface* before=ui_clone(onscreen_surface_);
		rgb_type saved_palette[256]; read_palette_256(saved_palette);
		palette_fade_type* fade=make_pal_buffer_fadein(source,0x1800,0);
		ui_check(memcmp(saved_palette,palette,sizeof(saved_palette))==0,"native RGB fade leaves UI palette unchanged");
		for (int step=1;step<=64;++step) {
			int done=fade_in_frame(fade);
			ui_check(done==(step==64),"fade-in completion preserves 64 steps");
			if (step==1 || step==32 || step==64) ui_check(ui_fade_pixels(source,before,(64-step)*4,clip),"fade-in supplied source, RGB/alpha, pitch and clip");
			if (mode==0 && step==32) save_audit(onscreen_surface_,"ui-fade-in-midpoint");
		}
		pal_restore_free_fadein(fade);
		ui_check(ui_fade_pixels(source,before,0,clip),"fade-in finish copies supplied source");
		/* Early interruption must also restore from the same borrowed source. */
		fade=make_pal_buffer_fadein(source,0x800,0); fade_in_frame(fade); pal_restore_free_fadein(fade);
		ui_check(ui_fade_pixels(source,before,0,clip),"interrupted fade-in restores correct source");
		SDL_FreeSurface(before); SDL_FreeSurface(source);
		SDL_SetClipRect(onscreen_surface_,NULL); ui_pattern(onscreen_surface_);
		before=ui_clone(onscreen_surface_);
		/* Static fade-out must cover all 400 rows despite a 384-row room. */
		is_cutscene=0; fade=make_pal_buffer_fadeout(0x1800,0);
		for (int step=1;step<=64;++step) {
			int done=fade_out_frame(fade);
			ui_check(done==(step==64),"fade-out completion preserves 64 steps");
			if (step==1 || step==32 || step==64) ui_check(ui_fade_pixels(before,before,step*4,(SDL_Rect){0,0,640,400}),"fade-out includes footer and uses visible source");
			if (mode==0 && step==32) save_audit(onscreen_surface_,"ui-fade-out-midpoint");
		}
		current_target_surface=onscreen_surface_; pal_restore_free_fadeout(fade);
		ui_check(memcmp(saved_palette,palette,sizeof(saved_palette))==0,"fade restores palette on completion");
		SDL_FreeSurface(before); SDL_FreeSurface(offscreen_surface); offscreen_surface=original_off;
		if (mode==2) { SDL_FreeSurface(onscreen_surface_); onscreen_surface_=original_on; }
	}
	/* Live cutscene source changes must reach the next fade-out frame. */
	if (!baseline) {
		offscreen_surface=make_offscreen_buffer(&screen_rect); ui_fill(offscreen_surface,0xD0B080);
		is_cutscene=1; palette_fade_type* fade=make_pal_buffer_fadeout(0x6689,0);
		ui_fill(offscreen_surface,0xB08060); fade_out_frame(fade);
		ui_check((ui_pixel(onscreen_surface_,319,399)&0xffffff)==0xAC7C5C,"cutscene fade-out reads newly animated source");
		current_target_surface=onscreen_surface_; pal_restore_free_fadeout(fade); is_cutscene=0;
		SDL_FreeSurface(offscreen_surface); offscreen_surface=original_off;
		strcpy(graphics_pack_name,"dos"); fade=make_pal_buffer_fadein(original_off,0x800,0);
		ui_check(fade->native_source==NULL && fade->native_snapshot==NULL,"other graphics packs retain legacy fade path");
		pal_restore_free_fadein(fade); strcpy(graphics_pack_name,"mac_native_full");
	}
	is_validate_mode=validate; current_target_surface=onscreen_surface_;
}

static void ui_scene_fades(void) {
	init_princess(); saveshad(); init_vizier(); savekid(); set_hourglass_state(2);
	for (int outgoing=0;outgoing<2;++outgoing) {
		palette_fade_type* fade=outgoing?make_pal_buffer_fadeout(0x6689,0):make_pal_buffer_fadein(offscreen_surface,0x6689,0);
		SDL_Surface* first=NULL;
		for (int step=1;step<=64;++step) {
			play_both_seq(); draw_princess_room_bg();
			if (step==1) first=ui_clone(offscreen_surface);
			int done=outgoing?fade_out_frame(fade):fade_in_frame(fade);
			ui_check(done==(step==64),"real animated scene fade completion");
			if (step==1 || step==32 || step==64) {
				ui_check(ui_fade_pixels(offscreen_surface,offscreen_surface,(outgoing?step:64-step)*4,(SDL_Rect){0,0,640,400}),"real Princess-room fade uses current composition to all edges");
				if (step==32) save_audit(onscreen_surface_,outgoing?"ui-princess-fade-out":"ui-princess-fade-in");
			}
		}
		ui_check(!ui_same(first,offscreen_surface),"actors/flames continue animating during fade"); SDL_FreeSurface(first);
		if (outgoing) pal_restore_free_fadeout(fade); else pal_restore_free_fadein(fade);
	}
}

static void ui_scene_transitions(void) {
	dword seed=random_seed;
	char_type kid=Kid,guard=Guard;
	byte validate=is_validate_mode; is_validate_mode=1;
	stop_sounds();
	int height=offscreen_surface->h;
	load_intro(0,ui_scene_fades,0);
	ui_check(offscreen_surface->h==height && !is_cutscene,"animated fades return to gameplay buffer");
	free_surface(offscreen_surface); offscreen_surface=make_offscreen_buffer(&screen_rect);
	ui_pattern(offscreen_surface); ui_fill(onscreen_surface_,0x341278);
	current_target_surface=onscreen_surface_;
	transition_ltr();
	ui_check(ui_same(offscreen_surface,onscreen_surface_),"real left-to-right transition copies final column and bottom row");
	save_audit(onscreen_surface_,"ui-transition-complete");
	free_surface(offscreen_surface); offscreen_surface=make_offscreen_buffer(&rect_top);
	Kid=kid; Guard=guard; random_seed=seed; is_validate_mode=validate;
}

void check_native_ui(const char* output) {
	audit_output=output; ui_checks=ui_failures=0;
	char_type saved_kid=Kid,saved_guard=Guard;
	dword seed=random_seed;
	short minute=rem_min,tick=rem_tick,level_number=current_level,next=next_level;
	short hc=hitp_curr,hm=hitp_max,hd=hitp_delta,gc=guardhp_curr,gm=guardhp_max,gd=guardhp_delta;
	byte blink=global_blink_state;
	struct star_type saved_stars[6]; memcpy(saved_stars,stars,sizeof(saved_stars));
	short torches[2]={princess_torch_frame[0],princess_torch_frame[1]};
	ui_health(); ui_text_menu_dialog(); ui_fades();
	if (!getenv("SDLPOP_UI_BASELINE")) ui_scene_transitions();
	ui_check(random_seed==seed && rem_min==minute && memcmp(&Kid,&saved_kid,sizeof(Kid))==0,"UI draws preserve Kid, RNG and game minutes");
	Kid=saved_kid; Guard=saved_guard; rem_tick=tick; current_level=level_number; next_level=next;
	hitp_curr=hc; hitp_max=hm; hitp_delta=hd; guardhp_curr=gc; guardhp_max=gm; guardhp_delta=gd; global_blink_state=blink;
	memcpy(stars,saved_stars,sizeof(saved_stars));
	princess_torch_frame[0]=torches[0]; princess_torch_frame[1]=torches[1];
	printf("UI AUDIT: %d checks, %d failures\n",ui_checks,ui_failures); fflush(stdout);
	assert(ui_failures==0);
}

/* Test-only audit. Runs last: load_intro replaces the scene sprite tables. */
extern short hourglass_state, hourglass_sandflow, princess_torch_frame[2], which_torch;
extern short cutscene_frame_time, cutscene_wait_frames;
extern word chtab_palette_bits;
static int cut_checks, cut_failures, cut_variant, cut_pillar_cases;
static SDL_Surface* cut_source[16];

static void cut_check(int ok, const char* label) {
 ++cut_checks;
 if (!ok) { ++cut_failures; printf("CUTSCENE FAIL %s\n", label); fflush(stdout); }
}

static int cut_difference(SDL_Surface* a, SDL_Surface* b) {
 assert(a->w==640 && b->w==640 && a->h==400 && b->h==400);
 SDL_Surface* aa=SDL_ConvertSurfaceFormat(a,SDL_PIXELFORMAT_ARGB8888,0);
 SDL_Surface* bb=SDL_ConvertSurfaceFormat(b,SDL_PIXELFORMAT_ARGB8888,0);
 assert(aa && bb);
 int changed=0;
 for (int y=0;y<400;++y) {
  Uint32* ap=(Uint32*)((byte*)aa->pixels+y*aa->pitch);
  Uint32* bp=(Uint32*)((byte*)bb->pixels+y*bb->pitch);
  for (int x=0;x<640;++x) if ((ap[x]&0xffffff)!=(bp[x]&0xffffff)) ++changed;
 }
 SDL_FreeSurface(aa); SDL_FreeSurface(bb);
 return changed;
}

static void cut_stamp(SDL_Surface* dst, int index, int x, int y) {
 SDL_Rect p={x,y,0,0};
 assert(SDL_BlitSurface(cut_source[index],NULL,dst,&p)==0);
}

static void cut_ignore_stars(SDL_Surface* expected, SDL_Surface* actual) {
 /* Star coordinates/twinkle are retained from SDLPoP, outside this oracle. */
 const int x[]={20,16,23,17,24,18},y[]={97,104,110,116,120,128};
 for (int i=0;i<6;++i) {
  SDL_Rect a={2*x[i],2*y[i],2,2},b=a;
  assert(SDL_BlitSurface(actual,&a,expected,&b)==0);
 }
}

static void cut_prop_oracle(int state, int flow) {
 /* Independent source PNG composition in physical 640x400 pixels. CODE 3
  * $1D30/$2512/$2578 and SHPD origins, not submitted render tables. */
 SDL_Surface* expected=SDL_ConvertSurfaceFormat(cut_source[0],SDL_PIXELFORMAT_ARGB8888,0);
 assert(expected);
 cut_stamp(expected,13+princess_torch_frame[0]%3,156,213);
 cut_stamp(expected,13+princess_torch_frame[1]%3,580,213);
 if (!(state>=7 && flow>=0) && state) cut_stamp(expected,state+2,297,284);
 cut_stamp(expected,2,473,219);
 if (state<7 && flow>=0) {
  int phase=(flow+1)%3;
  cut_stamp(expected,10+phase,313,phase==1?312:313);
 }
 cut_ignore_stars(expected,offscreen_surface);
 char label[100]; snprintf(label,sizeof(label),"source-pixels variant%d hourglass%d flow%d",cut_variant,state,flow);
 int changed=cut_difference(expected,offscreen_surface);
 cut_check(changed==0,label);
 if (changed && cut_failures<5) { save_audit(expected,"FAIL-cutscene-oracle"); save_audit(offscreen_surface,"FAIL-cutscene-actual"); }
 SDL_FreeSurface(expected);
}

static void cut_frame(const char* label) {
 char_type kid=Kid,guard=Guard;
 dword seed=random_seed;
 short torches[2]={princess_torch_frame[0],princess_torch_frame[1]};
 short which=which_torch,flow=hourglass_sandflow;
 short clock=cutscene_frame_time,wait=cutscene_wait_frames;
 /* Preserve the existing order and exactly two torch RNG draws. */
 short expected[2]={torches[0],torches[1]},next=which;
 for (int i=0;i<2;++i) { next=!next; expected[next]=get_torch_frame(expected[next]); }
 dword expected_seed=random_seed; random_seed=seed;
 draw_princess_room_bg();
 cut_check(memcmp(&Kid,&kid,sizeof(kid))==0 && memcmp(&Guard,&guard,sizeof(guard))==0 &&
           random_seed==expected_seed && princess_torch_frame[0]==expected[0] && princess_torch_frame[1]==expected[1] &&
           cutscene_frame_time==clock && cutscene_wait_frames==wait,label);
 SDL_Surface* consecutive=SDL_ConvertSurfaceFormat(offscreen_surface,SDL_PIXELFORMAT_ARGB8888,0);
 assert(consecutive);
 free_peels(); SDL_SetClipRect(offscreen_surface,NULL);
 SDL_FillRect(offscreen_surface,NULL,0xF023A8);
 princess_torch_frame[0]=torches[0]; princess_torch_frame[1]=torches[1];
 which_torch=which; hourglass_sandflow=flow; random_seed=seed;
 draw_princess_room_bg();
 cut_check(cut_difference(consecutive,offscreen_surface)==0,label);
 if (cut_failures && cut_failures<5) { save_audit(consecutive,"FAIL-cutscene-consecutive"); save_audit(offscreen_surface,"FAIL-cutscene-clean"); }
 SDL_FreeSurface(consecutive);
 /* Check real actors passing the pillar against every opaque source pixel. */
 int overlap=0;
 for (int i=0;i<objtable_count;++i) {
  objtable_type* actor=&objtable[i];
  image_type* image=get_image(actor->chtab_id,actor->id);
  short ox,oy;
  if (!image || !get_native_image_origin(image,&ox,&oy)) continue;
  if (actor->direction==dir_0_right) ox=image->w-1-ox;
  int x=calc_screen_x_coord(actor->xh*8+actor->xl)*2-ox;
  int y=actor->y*2-oy;
  if (x<505 && x+image->w>473 && y<400 && y+image->h>219) overlap=1;
 }
 if (overlap) {
  SDL_Surface* image=SDL_ConvertSurfaceFormat(cut_source[2],SDL_PIXELFORMAT_ARGB8888,0);
  SDL_Surface* actual=SDL_ConvertSurfaceFormat(offscreen_surface,SDL_PIXELFORMAT_ARGB8888,0);
  assert(image && actual); int changed=0;
  for (int y=0;y<181;++y) for (int x=0;x<32;++x) {
   Uint32 a=*(Uint32*)((byte*)image->pixels+y*image->pitch+x*4);
   Uint32 b=*(Uint32*)((byte*)actual->pixels+(y+219)*actual->pitch+(x+473)*4);
   if ((a>>24) && (a&0xffffff)!=(b&0xffffff)) ++changed;
  }
  cut_check(changed==0,"opaque native pillar hides actual actor"); ++cut_pillar_cases;
  SDL_FreeSurface(image); SDL_FreeSurface(actual);
 }
}

static void cut_sequence(const char* name, int frames) {
 for (int i=0;i<frames;++i) {
  play_both_seq(); cut_frame(name);
  if (i==frames-1) save_audit(offscreen_surface,name);
 }
}

static void capture_cutscene(void) {
 char name[80]; snprintf(name,sizeof(name),"cutscene-loaded-background-%d",cut_variant);
 save_audit(offscreen_surface,name);
 SDL_Surface* expected=SDL_ConvertSurfaceFormat(cut_source[0],SDL_PIXELFORMAT_ARGB8888,0);
 cut_ignore_stars(expected,offscreen_surface);
 cut_check(cut_difference(expected,offscreen_surface)==0,"native background without DOS bed");
 SDL_FreeSurface(expected);
 chtab_type* props=chtab_addrs[id_chtab_8_princessroom];
 cut_check(props && props->n_images==15,"native group 960 loaded");
 for (int i=1;i<15 && props && i<props->n_images;++i)
  cut_check(is_native_resolution_image(props->images[i]),"every prop is native");
 Kid.frame=Guard.frame=0;
 for (int state=0;state<=7;++state) for (int flow=-1;flow<3;++flow) {
  set_hourglass_state(state); hourglass_sandflow=flow;
  SDL_SetClipRect(offscreen_surface,NULL);
  SDL_FillRect(offscreen_surface,NULL,0xF023A8);
  draw_princess_room_bg(); cut_prop_oracle(state,flow);
  if (flow==-1) {
   snprintf(name,sizeof(name),"cutscene-hourglass-%d-%d",cut_variant,state);
   save_audit(offscreen_surface,name);
  }
 }
 set_hourglass_state(2);
 if (!cut_variant) {
  init_princess(); saveshad(); init_vizier(); savekid();
  cut_sequence("cutscene-jaffar-enter",8);
  seqtbl_offset_shad_char(98); cut_sequence("cutscene-princess-turn",5);
  seqtbl_offset_kid_char(96); cut_sequence("cutscene-jaffar-walk",36);
  seqtbl_offset_kid_char(97); cut_sequence("cutscene-jaffar-stop",35);
  seqtbl_offset_kid_char(102); cut_sequence("cutscene-jaffar-conjure",60);
  seqtbl_offset_shad_char(99); cut_sequence("cutscene-princess-step-back",17);
  seqtbl_offset_shad_char(113); cut_sequence("cutscene-princess-look-down",28);
 } else {
  Kid.frame=0; princess_lying(); saveshad(); cut_sequence("cutscene-princess-lying",26);
  init_mouse_cu8(); savekid(); princess_crouching(); saveshad();
  cut_sequence("cutscene-mouse-princess",20);
  seqtbl_offset_kid_char(107); cut_sequence("cutscene-mouse-away",20);
  seqtbl_offset_shad_char(111); cut_sequence("cutscene-princess-rise",20);
  princess_stand(); saveshad(); init_mouse_go(); savekid();
  cut_sequence("cutscene-mouse-arrive",5);
  seqtbl_offset_shad_char(112); cut_sequence("cutscene-princess-crouch",9);
  seqtbl_offset_kid_char(114); cut_sequence("cutscene-mouse-wait",58);
  init_ending_princess(); saveshad(); init_ending_kid(); savekid();
  cut_sequence("cutscene-ending-run",8);
  seqtbl_offset_shad_char(108); cut_sequence("cutscene-ending-hug-start",5);
  seqtbl_offset_kid_char(13); cut_sequence("cutscene-ending-stop",2);
  Kid.frame=0; cut_sequence("cutscene-ending-embrace",39);
  init_mouse_1(); savekid(); cut_sequence("cutscene-ending-mouse",9);
  seqtbl_offset_kid_char(101); cut_sequence("cutscene-ending-mouse-up",41);
 }
 Kid.frame=Guard.frame=0; set_hourglass_state(7); hourglass_sandflow=-1;
 cut_sequence(cut_variant?"cutscene-expired-1":"cutscene-expired-0",4);
}

static void cut_story_cards(void) {
 /* Native art is independently loaded from the preserved source folder. */
 free_peels();
 if (offscreen_surface) free_surface(offscreen_surface);
 offscreen_surface=make_offscreen_buffer(&screen_rect);
 SDL_Surface* source[5];
 for (int i=0;i<5;++i) {
  char path[160]; snprintf(path,sizeof(path),"tools/mac_reference/source-images/TITLE/%04d_macgroup40.png",41+i);
  source[i]=IMG_Load(path); assert(source[i]);
 }
 SDL_Surface* palace=IMG_Load("tools/mac_reference/source-images/TITLE/0051_macgroup50.png");
 assert(palace);
 for (int blue=1;blue>=0;--blue) {
  load_title_images(blue);
  current_target_surface=offscreen_surface; reset_clip_rect();
  SDL_FillRect(offscreen_surface,NULL,0xF023A8);
  draw_full_image(STORY_FRAME);
  /* Original CODE 3:$0D3A/$0DAA passes CLUT23; $263C passes CLUT16.
   * The frame PNG carries alpha, so compose onto that independent RGB fill. */
  SDL_Surface* background=SDL_ConvertSurfaceFormat(source[0],SDL_PIXELFORMAT_ARGB8888,0);
  assert(background);
  SDL_FillRect(background,NULL,SDL_MapRGB(background->format,blue?16:128,0,blue?96:0));
  assert(SDL_BlitSurface(source[0],NULL,background,NULL)==0);
  cut_check(cut_difference(background,offscreen_surface)==0,"native story underlay and opaque border pixels");
  for (int id=STORY_ABSENCE;id<=STORY_CREDITS;++id) {
  current_target_surface=offscreen_surface; reset_clip_rect();
  /* Preserve the old stable-pack comparator, including its black frame. */
  strcpy(graphics_pack_name,"mac_native");
  draw_full_image(STORY_FRAME); draw_full_image(id);
  strcpy(graphics_pack_name,"mac_native_full");
  char name[100]; snprintf(name,sizeof(name),blue?"cutscene-story-before-%d":"cutscene-story-red-before-%d",id);
  save_audit(offscreen_surface,name);
  draw_full_image(STORY_FRAME); draw_full_image(id);
  SDL_Surface* expected=SDL_ConvertSurfaceFormat(background,SDL_PIXELFORMAT_ARGB8888,0);
  int index=id-STORY_ABSENCE+1;
  SDL_Rect pos={index==4?66:71,index==4?50:51,0,0};
  assert(SDL_BlitSurface(source[index],NULL,expected,&pos)==0);
  cut_check(cut_difference(expected,offscreen_surface)==0,"story card source pixels and SHPD inset");
  snprintf(name,sizeof(name),blue?"cutscene-story-%d":"cutscene-story-red-%d",id); save_audit(offscreen_surface,name);
  SDL_FreeSurface(expected);
  }
  /* STORY_FRAME's underlay must not tint the separate palace title scene. */
  draw_full_image(TITLE_MAIN);
  SDL_FillRect(background,NULL,SDL_MapRGB(background->format,0,0,0));
  assert(SDL_BlitSurface(source[0],NULL,background,NULL)==0);
  SDL_Rect pos={33,0,0,0};
  assert(SDL_BlitSurface(palace,NULL,background,&pos)==0);
  cut_check(cut_difference(background,offscreen_surface)==0,"title retains original black backdrop");
  char name[80]; snprintf(name,sizeof(name),"cutscene-title-bg%d",blue);
  save_audit(offscreen_surface,name);
  SDL_FreeSurface(background);
  release_title_images();
 }
 SDL_FreeSurface(palace);
 for (int i=0;i<5;++i) SDL_FreeSurface(source[i]);
 current_target_surface=onscreen_surface_;
 /* Simulate the gameplay-sized entry path as well as title's full buffer. */
 free_surface(offscreen_surface); offscreen_surface=make_offscreen_buffer(&rect_top);
}

static void cut_loader_scope(void) {
 word saved=chtab_palette_bits;
 strcpy(graphics_pack_name,"dos");
 dat_type* dat=open_dat("PV.DAT",'G');
 dat_shpl_type* shpl=(dat_shpl_type*)load_from_opendats_alloc(950,"pal",NULL,NULL);
 assert(shpl); int count=shpl->n_images; free(shpl);
 chtab_type* sprites=load_sprites_from_file(950,1<<13,1);
 cut_check(sprites && sprites->n_images==count,"DOS prop loader keeps original count");
 close_dat(dat); free_chtab(sprites);
 strcpy(graphics_pack_name,"mac_native_full"); chtab_palette_bits=saved;
}

static void cut_fullscreen_entry(void) {
 Kid.frame=Guard.frame=0; hourglass_state=0; hourglass_sandflow=-1;
 cutscene_wait_frames=2; draw_proom_drects();
 cut_check(cut_difference(onscreen_surface_,offscreen_surface)==0,"full 400-row cutscene presentation");
 cut_prop_oracle(0,-1);
}

void check_native_cutscenes(const char* output) {
 audit_output=output; cut_checks=cut_failures=0;
 for (int i=0;i<16;++i) if (i!=1) {
  char path[160]; snprintf(path,sizeof(path),"tools/mac_reference/source-images/PV/%04d_macgroup%d.png",i?960+i:951,i?960:950);
  cut_source[i]=IMG_Load(path); assert(cut_source[i]);
 }
 free_peels(); clear_tile_wipes(); is_blind_mode=0; draw_mode=0;
 united_with_shadow=0; stop_sounds();
 cut_story_cards(); cut_loader_scope();
 for (cut_variant=0;cut_variant<2;++cut_variant) {
  int height=offscreen_surface->h;
  load_intro(cut_variant,capture_cutscene,0);
  cut_check(offscreen_surface->h==height && !is_cutscene,"return restores gameplay buffer and scene flag");
 }
 cut_check(cut_pillar_cases>0,"actor sequences crossed foreground pillar");
 free_surface(offscreen_surface); offscreen_surface=make_offscreen_buffer(&screen_rect);
 load_intro(0,cut_fullscreen_entry,0);
 cut_check(offscreen_surface->h==400,"title-sized cutscene entry keeps full buffer");
 for (int i=0;i<16;++i) if (cut_source[i]) SDL_FreeSurface(cut_source[i]);
 printf("Cutscene audit: %d checks, %d failures\n",cut_checks,cut_failures);
 fflush(stdout);
 assert(cut_failures==0);
}

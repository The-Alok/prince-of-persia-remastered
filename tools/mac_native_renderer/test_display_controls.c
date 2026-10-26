/* Real engine display/input regression. No test hooks ship with the game. */
#include "common.h"
#include "pak.h"
#include "video_filters.h"
#include <assert.h>
#include <unistd.h>
extern char last_text_input;
extern int need_full_menu_redraw_count;
extern const rect_type splash_text_1_rect, splash_text_2_rect;
extern const char *splash_text_1, *splash_text_2;
void draw_pause_menu(void);
void __real_draw_tables(void);
void __real_load_ingame_settings(void);
void __real_SDL_RenderPresent(SDL_Renderer *);
static int tested, checks;
static const char *capture_name;

void __wrap_load_ingame_settings(void) {} /* Never borrow the player's CFG. */
void __wrap_SDL_RenderPresent(SDL_Renderer *renderer) {
    if(capture_name) {
        int w,h;SDL_GetRendererOutputSize(renderer,&w,&h);
        SDL_Surface *s=SDL_CreateRGBSurfaceWithFormat(0,w,h,24,SDL_PIXELFORMAT_RGB24);
        assert(s&&SDL_RenderReadPixels(renderer,NULL,s->format->format,s->pixels,s->pitch)==0);
        char path[1024];snprintf(path,sizeof(path),"%s/%s.png",getenv("DISPLAY_CAPTURE_DIR"),capture_name);
        assert(IMG_SavePNG(s,path)==0);SDL_FreeSurface(s);capture_name=NULL;
    }
    __real_SDL_RenderPresent(renderer);
}
static Uint32 pixel(SDL_Surface *s,int x,int y) {Uint32 p=0;memcpy(&p,(byte*)s->pixels+y*s->pitch+x*3,3);return p;}
static uint32_t hash(SDL_Surface *s){uint32_t h=2166136261u;for(int y=0;y<s->h;y++)for(int x=0;x<s->w*3;x++){h^=((byte*)s->pixels)[y*s->pitch+x];h*=16777619u;}return h;}
static void scale3x_oracle(void) {
    SDL_Surface *s=SDL_CreateRGBSurfaceWithFormat(0,3,3,24,SDL_PIXELFORMAT_RGB24);
    SDL_Surface *d=SDL_CreateRGBSurfaceWithFormat(0,9,9,24,SDL_PIXELFORMAT_RGB24);assert(s&&d);
    for(unsigned pattern=0;pattern<512;pattern++) {
        for(int y=0;y<3;y++)for(int x=0;x<3;x++){
            Uint32 p=(pattern&(1u<<(y*3+x)))?0xDDAA33:0x123456;
            memcpy((byte*)s->pixels+y*s->pitch+x*3,&p,3);
        }
        assert(scale3x_rgb_surface(s,d));
        for(int y=0;y<3;y++)for(int x=0;x<3;x++){
            int l=MAX(0,x-1),r=MIN(2,x+1),u=MAX(0,y-1),v=MIN(2,y+1);
            Uint32 A=pixel(s,l,u),B=pixel(s,x,u),C=pixel(s,r,u),D=pixel(s,l,y),E=pixel(s,x,y),F=pixel(s,r,y),G=pixel(s,l,v),H=pixel(s,x,v),I=pixel(s,r,v);
            /* Independent unoptimised edge predicates from the specification. */
            int ul=D==B&&B!=F&&D!=H,ur=B==F&&B!=D&&F!=H;
            int ll=D==H&&D!=B&&H!=F,lr=H==F&&D!=H&&B!=F;
            Uint32 want[9]={ul?D:E,((ul&&E!=C)||(ur&&E!=A))?B:E,ur?F:E,
                ((ul&&E!=G)||(ll&&E!=A))?D:E,E,((ur&&E!=I)||(lr&&E!=C))?F:E,
                ll?D:E,((ll&&E!=I)||(lr&&E!=G))?H:E,lr?F:E};
            for(int dy=0;dy<3;dy++)for(int dx=0;dx<3;dx++){assert(pixel(d,x*3+dx,y*3+dy)==want[dy*3+dx]);checks++;}
        }
    }
    assert(!scale3x_rgb_surface(s,s));checks++;
    SDL_FreeSurface(s);SDL_FreeSurface(d);
}
static void event(int type,int scancode,int repeat) {
    SDL_Event e;memset(&e,0,sizeof(e));e.type=type;e.key.keysym.scancode=scancode;e.key.repeat=repeat;
    assert(SDL_PushEvent(&e)==1);process_events();
}
typedef struct {
    char_type kid,guard,actor,opponent;level_type level;
    dword seed;word seeded,crossing,next,minute,tick,paused;
    int key,any;char input;byte held[SDL_NUM_SCANCODES];
    short hp,ghp,kdelta,gdelta;
} State;
static State snapshot(void) {
    State s;memset(&s,0,sizeof(s));s.kid=Kid;s.guard=Guard;s.actor=Char;s.opponent=Opp;s.level=level;
    s.seed=random_seed;s.seeded=seed_was_init;s.crossing=exit_room_timer;s.next=next_room;s.minute=rem_min;s.tick=rem_tick;s.paused=is_paused;
    s.key=last_key_scancode;s.any=last_any_key_scancode;s.input=last_text_input;memcpy(s.held,key_states,sizeof(s.held));
    s.hp=hitp_curr;s.ghp=guardhp_curr;s.kdelta=hitp_delta;s.gdelta=guardhp_delta;return s;
}
static void assert_state(State *before) {State after=snapshot();assert(memcmp(before,&after,sizeof(after))==0);checks++;}
void __wrap_draw_tables(void) {
    __real_draw_tables();
    if(tested||is_cutscene||current_level!=1||drawn_room<1||!chtab_addrs[id_chtab_2_kid])return;
    /* The first draw hook can occur inside the initial blind/clear pass.
     * Finish a normal room composition before testing player shortcuts. */
    tested=1;is_blind_mode=0;scale3x_oracle();
    /* Recompose at a stable boundary; the wrapper cannot run a second time. */
    redraw_screen(0);copy_screen_rect(&rect_top);
    byte original=graphics_pack_index;
    SDL_SetWindowSize(window_,1600,1000);use_correct_aspect_ratio=0;use_integer_scaling=0;apply_aspect_ratio();
    const char *names[]={"Pixel","Sharp","Soft","Smooth HQ"};
    for(int pack=0;pack<num_available_paks;pack++) {
        assert(switch_graphics_pack(pack));
        redraw_screen(0);copy_screen_rect(&rect_top);
        unsigned room_pixels=0;for(int y=0;y<384;y++)for(int x=0;x<640;x++)if(pixel(onscreen_surface_,x,y))room_pixels++;
        assert(room_pixels>30000);checks++;
        uint32_t source=hash(onscreen_surface_);dword seed=random_seed;
        printf("SOURCE RGB %s %08x\n",available_paks[pack].id,source);
        for(int mode=0;mode<4;mode++) {
            scaling_type=mode;assert(!strcmp(video_filter_name(),names[mode]));
            char path[96];snprintf(path,sizeof(path),"%s-%d",available_paks[pack].id,mode);
            capture_name=path;update_screen();
            assert(hash(onscreen_surface_)==source&&random_seed==seed);checks++;
        }
        /* Exercise Sharp's streaming/software fallback and window-size changes. */
        bool target=is_renderer_targettexture_supported;is_renderer_targettexture_supported=0;scaling_type=1;
        update_screen();SDL_SetWindowSize(window_,1280,800);window_resized();update_screen();
        SDL_SetWindowSize(window_,1600,1000);window_resized();is_renderer_targettexture_supported=target;update_screen();
        assert(hash(onscreen_surface_)==source);checks++;
    }
    assert(switch_graphics_pack(original));
    scaling_type=0;last_key_scancode=last_any_key_scancode=0;
    event(SDL_KEYDOWN,SDL_SCANCODE_F7,0);assert(scaling_type==1);checks++;
    event(SDL_KEYDOWN,SDL_SCANCODE_F7,1);event(SDL_KEYDOWN,SDL_SCANCODE_F7,0);assert(scaling_type==1);checks++;
    event(SDL_KEYUP,SDL_SCANCODE_F7,0);
    for(int n=0;n<3;n++){event(SDL_KEYDOWN,SDL_SCANCODE_F7,0);event(SDL_KEYUP,SDL_SCANCODE_F7,0);}
    assert(scaling_type==0&&last_key_scancode==0&&last_any_key_scancode==0);checks++;
    for(int paused=0;paused<2;paused++)for(int n=0;n<num_available_paks;n++) {
        is_paused=paused;is_menu_shown=paused;
        exit_room_timer=1;next_room=Kid.room;last_key_scancode=SDL_SCANCODE_RIGHT;last_any_key_scancode=SDL_SCANCODE_RIGHT;last_text_input='x';
        key_states[SDL_SCANCODE_RIGHT]=KEYSTATE_HELD;
        byte previous=graphics_pack_index;
        event(SDL_KEYDOWN,SDL_SCANCODE_F8,0);event(SDL_KEYDOWN,SDL_SCANCODE_F8,1);
        assert(graphics_pack_index==previous);checks++;
        State before=snapshot();apply_visual_shortcuts();assert_state(&before);
        assert(graphics_pack_index==(previous+1)%num_available_paks);checks++;
        apply_visual_shortcuts();assert(graphics_pack_index==(previous+1)%num_available_paks);checks++;
        event(SDL_KEYUP,SDL_SCANCODE_F8,0);
    }
    is_paused=0;is_menu_shown=0;last_key_scancode=last_any_key_scancode=0;
    byte pack=graphics_pack_index;is_cutscene=1;
    event(SDL_KEYDOWN,SDL_SCANCODE_F8,0);event(SDL_KEYUP,SDL_SCANCODE_F8,0);is_cutscene=0;
    apply_visual_shortcuts();assert(graphics_pack_index==pack);checks++;
    assert(switch_graphics_pack(original));scaling_type=3;
    save_ingame_settings();scaling_type=0;
    char cwd[4096];assert(getcwd(cwd,sizeof(cwd)));
#ifdef _WIN32
    char *settings_dir=SDL_GetBasePath();assert(settings_dir);
    assert(chdir(settings_dir)==0);SDL_free(settings_dir);
#else
    assert(chdir(getenv("HOME"))==0);
#endif
    __real_load_ingame_settings();assert(chdir(cwd)==0);assert(scaling_type==3);checks++;
    /* Capture the production welcome text and pause footer for visual review. */
    SDL_Surface *game_picture=SDL_ConvertSurface(onscreen_surface_,onscreen_surface_->format,0);assert(game_picture);
    SDL_SetWindowSize(window_,640,400);window_resized();scaling_type=0;
    current_target_surface=onscreen_surface_;draw_rect(&screen_rect,color_0_black);
    show_text_with_color(&splash_text_1_rect,halign_center,valign_middle,splash_text_1,color_15_brightwhite);
    show_text_with_color(&splash_text_2_rect,halign_center,valign_top,splash_text_2,color_7_lightgray);
    capture_name="welcome-shortcuts";update_screen();
    SDL_BlitSurface(game_picture,NULL,onscreen_surface_,NULL);SDL_FreeSurface(game_picture);init_menu();is_paused=1;is_menu_shown=-1;
    current_target_surface=overlay_surface;textstate.ptr_font=&hc_small_font;draw_pause_menu();
    capture_name="pause-shortcuts";update_screen();
    printf("DISPLAY CONTROLS: %d checks, 0 failures; all packs/four filters, key repeats, paused/active switches, Scale3x, no game-state changes\n",checks);
    stop_sounds();free_all_sounds();restore_stuff();exit(0);
}

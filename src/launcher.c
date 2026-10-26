/* Prince of Persia Remastered launcher. Copyright (C) 2026 The-Alok.
 * SPDX-License-Identifier: GPL-3.0-or-later. See COPYING.
 * UI artwork is drawn here; no Macintosh game images are embedded. */
#include <SDL.h>
#include "config.h"
#include "mac_asset_import.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#include <commdlg.h>
#include <SDL_syswm.h>
#else
#include <dirent.h>
#include <unistd.h>
#endif

#define W 800
#define H 560
#define PATH_CAP 4096
typedef struct { const char *letters; unsigned char rows[7]; } Glyph;
/* Five-column geometric pixel alphabet, drawn at integer scale. */
static const Glyph glyphs[] = {
 {"A",{14,17,17,31,17,17,17}},{"B",{30,17,17,30,17,17,30}},
 {"C",{14,17,16,16,16,17,14}},{"D",{30,17,17,17,17,17,30}},
 {"E",{31,16,16,30,16,16,31}},{"F",{31,16,16,30,16,16,16}},
 {"G",{14,17,16,23,17,17,15}},{"H",{17,17,17,31,17,17,17}},
 {"I",{14,4,4,4,4,4,14}},{"J",{7,2,2,2,18,18,12}},
 {"K",{17,18,20,24,20,18,17}},{"L",{16,16,16,16,16,16,31}},
 {"M",{17,27,21,21,17,17,17}},{"N",{17,25,21,19,17,17,17}},
 {"O",{14,17,17,17,17,17,14}},{"P",{30,17,17,30,16,16,16}},
 {"Q",{14,17,17,17,21,18,13}},{"R",{30,17,17,30,20,18,17}},
 {"S",{15,16,16,14,1,1,30}},{"T",{31,4,4,4,4,4,4}},
 {"U",{17,17,17,17,17,17,14}},{"V",{17,17,17,17,17,10,4}},
 {"W",{17,17,17,21,21,21,10}},{"X",{17,17,10,4,10,17,17}},
 {"Y",{17,17,10,4,4,4,4}},{"Z",{31,1,2,4,8,16,31}},
 {"0",{14,17,19,21,25,17,14}},{"1",{4,12,4,4,4,4,14}},
 {"2",{14,17,1,2,4,8,31}},{"3",{30,1,1,14,1,1,30}},
 {"4",{2,6,10,18,31,2,2}},{"5",{31,16,16,30,1,1,30}},
 {"6",{14,16,16,30,17,17,14}},{"7",{31,1,2,4,8,8,8}},
 {"8",{14,17,17,14,17,17,14}},{"9",{14,17,17,15,1,1,14}},
 {".",{0,0,0,0,0,12,12}},{",",{0,0,0,0,4,4,8}},
 {":",{0,12,12,0,12,12,0}},{"-",{0,0,0,31,0,0,0}},
 {"/",{1,2,2,4,8,8,16}},{"\\",{16,8,8,4,2,2,1}},
 {"(",{2,4,8,8,8,4,2}},{")",{8,4,2,2,2,4,8}},
 {"[",{14,8,8,8,8,8,14}},{"]",{14,2,2,2,2,2,14}},
 {"_",{0,0,0,0,0,0,31}},{"+",{0,4,4,31,4,4,0}},
 {"?",{14,17,1,2,4,0,4}},{"!",{4,4,4,4,4,0,4}},
 {"'",{4,4,8,0,0,0,0}},{"=",{0,31,0,31,0,0,0}},
 {"<",{2,4,8,16,8,4,2}},{">",{8,4,2,1,2,4,8}}
};
static const SDL_Color ink = {233,235,244,255}, muted = {145,160,191,255}, gold = {225,174,83,255};
static const SDL_Color green = {80,218,151,255}, red = {244,110,116,255};
typedef struct {
    char *disk, *pack; char error[512]; SDL_atomic_t done; int success;
} ImportJob;
typedef struct { char *name; int directory; } BrowserEntry;
typedef struct {
    SDL_Window *window; SDL_Renderer *renderer;
    char *base, *pack, *config, *disk;
    int ready, disk_found, running, browse, scroll;
    char message[512], browser_message[160], directory[PATH_CAP], path_input[PATH_CAP];
    BrowserEntry files[2048]; int count;
    SDL_Thread *worker; ImportJob job;
} Launcher;

static char *join(const char *base, const char *name) {
    size_t n = strlen(base) + strlen(name) + 2; char *s = malloc(n);
    if (s) SDL_snprintf(s, n, "%s%s", base, name);
    return s;
}
static void fill(SDL_Renderer *r, SDL_Rect rect, SDL_Color c) {
    SDL_SetRenderDrawColor(r,c.r,c.g,c.b,c.a); SDL_RenderFillRect(r,&rect);
}
static void text(SDL_Renderer *r, int x, int y, int scale, const char *s, SDL_Color c) {
    int left = x;
    for (; *s; s++) {
        unsigned ch = (unsigned char)*s;
        if (ch == '\n') { x = left; y += scale * 10; continue; }
        if ((ch & 192) == 128) continue;
        if (ch >= 'a' && ch <= 'z') ch -= 32;
        const Glyph *glyph = NULL;
        if (ch != ' ') {
            for (size_t g = 0; g < sizeof(glyphs)/sizeof(*glyphs); g++) if ((unsigned char)glyphs[g].letters[0] == ch) { glyph = &glyphs[g]; break; }
            if (!glyph) glyph = &glyphs[sizeof(glyphs)/sizeof(*glyphs) - 6];
            for (int row=0;row<7;row++) for(int col=0;col<5;col++) if(glyph->rows[row]&(16>>col))
                fill(r,(SDL_Rect){x+col*scale,y+row*scale,scale,scale},c);
        }
        x += scale * 6;
    }
}
static void wrapped(SDL_Renderer *r,int x,int y,int width,const char *s,SDL_Color color) {
    char line[128]; int used=0, limit=width/12;
    if(limit>120) limit=120;
    while(*s) {
        if(*s=='\n' || used==limit) {
            int cut=used;
            if(*s!='\n') { while(cut>0 && line[cut-1]!=' ') cut--; if(cut==0)cut=used; }
            char tail[128]; int rest=used-cut; memcpy(tail,line+cut,rest);
            line[cut]=0;text(r,x,y,2,line,color);y+=22;
            memcpy(line,tail,rest);used=rest;
            if(*s=='\n')s++;
        } else line[used++]=*s++;
    }
    line[used]=0;text(r,x,y,2,line,color);
}
static void led(SDL_Renderer *r,int cx,int cy,int on) {
    SDL_Color c=on?green:red;
    for(int y=-7;y<=7;y++)for(int x=-7;x<=7;x++)if(x*x+y*y<=49)fill(r,(SDL_Rect){cx+x,cy+y,1,1},c);
    fill(r,(SDL_Rect){cx-2,cy-4,3,2},(SDL_Color){240,255,245,255});
}
static int inside(int x,int y,SDL_Rect r) {return x>=r.x&&y>=r.y&&x<r.x+r.w&&y<r.y+r.h;}
static void button(Launcher *a,SDL_Rect r,const char *label,int primary,int enabled) {
    SDL_Color c=enabled?(primary?(SDL_Color){191,132,48,255}:(SDL_Color){39,57,86,255}):(SDL_Color){29,35,50,255};
    fill(a->renderer,r,c);SDL_SetRenderDrawColor(a->renderer,enabled?gold.r:60,enabled?gold.g:69,enabled?gold.b:85,255);SDL_RenderDrawRect(a->renderer,&r);
    text(a->renderer,r.x+(r.w-(int)strlen(label)*12)/2,r.y+(r.h-14)/2,2,label,enabled?ink:muted);
}
static int readable(const char *path) {
    SDL_RWops *f=path?SDL_RWFromFile(path,"rb"):NULL;if(!f)return 0;
    Sint64 n=SDL_RWsize(f);SDL_RWclose(f);return n>0;
}
static const char *filename(const char *path) {
    const char *result=path;for(const char *p=path;*p;p++)if(*p=='/'||*p=='\\')result=p+1;return result;
}
static void remember(Launcher *a) {
    SDL_RWops *f=SDL_RWFromFile(a->config,"wb");
    if(f){SDL_RWwrite(f,a->disk,1,strlen(a->disk));SDL_RWclose(f);}
}
static int import_worker(void *context) {
    ImportJob *j=context;j->success=mac_import_disk(j->disk,j->pack,j->error,sizeof(j->error));SDL_AtomicSet(&j->done,1);return 0;
}
static void select_disk(Launcher *a,const char *path) {
    if(a->worker)return;
    char *copy=SDL_strdup(path);
    if(!copy)return;
    free(a->disk);a->disk=copy;a->disk_found=readable(copy);
    if(!a->disk_found){SDL_strlcpy(a->message,"Cannot read that image. Choose a file from your Macintosh colour game.",sizeof(a->message));return;}
    remember(a);
    if(a->ready){SDL_strlcpy(a->message,"Your graphics are ready. Press Play to start.",sizeof(a->message));return;}
    a->job.disk=a->disk;a->job.pack=a->pack;a->job.success=0;SDL_AtomicSet(&a->job.done,0);
    a->worker=SDL_CreateThread(import_worker,"Mac graphics import",&a->job);
    SDL_strlcpy(a->message,a->worker?"Preparing your Macintosh graphics...":"Could not start extraction. Close the launcher and try again.",sizeof(a->message));
}

#ifndef _WIN32
static void clear_browser(Launcher *a){for(int i=0;i<a->count;i++)free(a->files[i].name);a->count=0;}
static int browser_order(const void *l,const void *r){const BrowserEntry*x=l,*y=r;if(x->directory!=y->directory)return y->directory-x->directory;return SDL_strcasecmp(x->name,y->name);}
static int image_extension(const char *name){const char*e=strrchr(name,'.');return e&&(!SDL_strcasecmp(e,".hfs")||!SDL_strcasecmp(e,".dsk")||!SDL_strcasecmp(e,".dc42")||!SDL_strcasecmp(e,".iso")||!SDL_strcasecmp(e,".hqx")||!SDL_strcasecmp(e,".sit"));}
static void open_directory(Launcher *a,const char *directory) {
    char next[PATH_CAP];SDL_strlcpy(next,directory,sizeof(next));
    DIR *d=opendir(next);if(!d){SDL_strlcpy(a->browser_message,"Cannot open that folder. Choose another location.",sizeof(a->browser_message));return;}
    a->browser_message[0]=0;
    clear_browser(a);SDL_strlcpy(a->directory,next,sizeof(a->directory));a->scroll=0;
    size_t len=strlen(a->directory);if(len&&a->directory[len-1]!='/')SDL_strlcat(a->directory,"/",sizeof(a->directory));
    struct dirent *entry;
    while((entry=readdir(d))&&a->count<2048) {
        if(entry->d_name[0]=='.')continue;
        char path[PATH_CAP];SDL_snprintf(path,sizeof(path),"%s%s",a->directory,entry->d_name);struct stat st;
        if(stat(path,&st))continue;
        int is_dir=S_ISDIR(st.st_mode);
        if(!is_dir&&(!S_ISREG(st.st_mode)||!image_extension(entry->d_name)))continue;
        a->files[a->count].name=SDL_strdup(entry->d_name);a->files[a->count].directory=is_dir;
        if(a->files[a->count].name)a->count++;
    }
    closedir(d);qsort(a->files,a->count,sizeof(*a->files),browser_order);
    SDL_strlcpy(a->path_input,a->directory,sizeof(a->path_input));a->browse=1;SDL_StartTextInput();
}
static void browse(Launcher *a){const char*home=SDL_getenv("HOME");open_directory(a,home?home:a->base);}
static void choose_path(Launcher *a) {
    struct stat st;if(stat(a->path_input,&st)){SDL_strlcpy(a->browser_message,"File not found. Choose a folder or paste the image path.",sizeof(a->browser_message));return;}
    if(S_ISDIR(st.st_mode))open_directory(a,a->path_input);
    else if(S_ISREG(st.st_mode)){a->browse=0;SDL_StopTextInput();select_disk(a,a->path_input);}
    else SDL_strlcpy(a->browser_message,"Choose a disk image file.",sizeof(a->browser_message));
}
#else
static wchar_t *wide(const char *s){int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s,-1,NULL,0);wchar_t*w=n?malloc(n*sizeof(*w)):NULL;if(w)MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s,-1,w,n);return w;}
static void browse(Launcher *a) {
    wchar_t file[32768]={0};OPENFILENAMEW dialog;SDL_zero(dialog);dialog.lStructSize=sizeof(dialog);
    SDL_SysWMinfo info;SDL_VERSION(&info.version);if(SDL_GetWindowWMInfo(a->window,&info))dialog.hwndOwner=info.info.win.window;
    dialog.lpstrFile=file;dialog.nMaxFile=32768;dialog.lpstrTitle=L"Choose your Macintosh Prince of Persia disk image";
    dialog.lpstrFilter=L"Macintosh game images and archives\0*.hfs;*.dsk;*.dc42;*.iso;*.hqx;*.sit\0All files\0*.*\0\0";
    dialog.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR|OFN_EXPLORER;
    if(GetOpenFileNameW(&dialog)) {
        int n=WideCharToMultiByte(CP_UTF8,0,file,-1,NULL,0,NULL,NULL);char*p=n?malloc(n):NULL;
        if(p){WideCharToMultiByte(CP_UTF8,0,file,-1,p,n,NULL,NULL);select_disk(a,p);free(p);}
    }
}
#endif

static void draw(Launcher *a) {
    SDL_Renderer *r=a->renderer;
    for(int y=0;y<H;y++)fill(r,(SDL_Rect){0,y,W,1},(SDL_Color){11+y/100,17+y/55,32+y/35,255});
    fill(r,(SDL_Rect){0,0,W,5},gold);fill(r,(SDL_Rect){32,92,736,1},(SDL_Color){59,69,91,255});
    text(r,34,28,3,"PRINCE OF PERSIA",ink);text(r,36,62,2,"REMASTERED",gold);text(r,660,65,1,"V" REMASTERED_VERSION,muted);
    if(a->browse) {
        text(r,36,115,2,"CHOOSE YOUR DISK IMAGE",gold);
        button(a,(SDL_Rect){36,147,88,34},"UP",0,1);button(a,(SDL_Rect){136,147,100,34},"HOME",0,1);
        button(a,(SDL_Rect){248,147,100,34},"ROOT",0,1);button(a,(SDL_Rect){648,147,116,34},"CANCEL",0,1);
        const char *path=a->directory;size_t len=strlen(path);if(len>60)path+=len-60;text(r,38,199,2,path,muted);
        for(int i=0;i<7&&i+a->scroll<a->count;i++) {
            BrowserEntry *item=&a->files[i+a->scroll];int y=227+i*33;
            fill(r,(SDL_Rect){36,y,728,31},(SDL_Color){24,34+(i%2)*3,52+(i%2)*3,255});
            text(r,48,y+8,2,item->directory?"+":" ",item->directory?gold:green);
            char name[57];SDL_strlcpy(name,item->name,sizeof(name));text(r,72,y+8,2,name,item->directory?ink:green);
        }
        if(!a->count)text(r,44,240,2,"NO DISK IMAGES HERE. OPEN A FOLDER ABOVE.",muted);
        fill(r,(SDL_Rect){36,474,600,36},(SDL_Color){9,14,25,255});
        const char *input=a->path_input;len=strlen(input);if(len>47)input+=len-47;text(r,44,485,2,input,ink);
        button(a,(SDL_Rect){648,474,116,36},"OPEN",1,1);
        text(r,38,529,1,a->browser_message[0]?a->browser_message:"SCROLL TO BROWSE. TYPE OR PASTE A PATH. ENTER TO OPEN. ESC TO CANCEL.",a->browser_message[0]?red:muted);
    } else {
        text(r,36,116,2,"MACINTOSH GRAPHICS SETUP",gold);
        led(r,49,169,a->disk_found);text(r,70,162,2,a->disk_found?"DISK IMAGE FOUND":"NO DISK IMAGE SELECTED",ink);
        led(r,49,208,a->ready);text(r,70,201,2,a->ready?"GRAPHICS READY":a->worker?"PREPARING GRAPHICS...":"GRAPHICS NOT PREPARED",ink);
        fill(r,(SDL_Rect){36,244,728,50},(SDL_Color){9,15,27,255});
        char name[57];SDL_strlcpy(name,a->disk?filename(a->disk):"COPY POP1.HFS OR POP1.DSK BESIDE THE GAME",sizeof(name));
        text(r,50,262,2,name,a->disk?ink:muted);
        button(a,(SDL_Rect){36,311,224,42},"BROWSE FOR IMAGE",0,!a->worker);
        if(a->worker){int x=36+(SDL_GetTicks()/8)%660;fill(r,(SDL_Rect){36,373,728,4},(SDL_Color){38,50,72,255});fill(r,(SDL_Rect){x,373,68,4},gold);}
        wrapped(r,38,392,720,a->message,a->ready?green:muted);
        button(a,(SDL_Rect){36,481,224,44},"PLAY DOS MODE",0,!a->worker);
        button(a,(SDL_Rect){540,476,224,54},"PLAY",1,a->ready&&!a->worker);
        text(r,38,544,1,"YOUR DISK IS READ ONLY. GRAPHICS ARE PREPARED ONCE.",muted);
    }
    SDL_RenderPresent(r);
}

static int launch_engine(Launcher *a,int argc,char **argv,const char *graphics) {
#ifdef _WIN32
    const char *engine_name="runtime/prince-engine.exe",*visible="prince.exe";
#else
    const char *engine_name="runtime/prince-engine",*visible="prince";
#endif
    char *engine=join(a->base,engine_name),*argv0=join(a->base,visible);
    if(!engine||!argv0||!readable(engine)){free(engine);free(argv0);SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,REMASTERED_NAME,"The game executable is missing from the runtime folder. Extract the complete download and try again.",a->window);return 0;}
    char **args=calloc(argc+3,sizeof(*args));if(!args){free(engine);free(argv0);return 0;}
    args[0]=argv0;for(int i=1;i<argc;i++)args[i]=argv[i];
    char option[80];if(graphics){SDL_snprintf(option,sizeof(option),"graphics=%s",graphics);args[argc]=option;}
    int result=0;
#ifdef _WIN32
    size_t cap=1024;for(int i=0;args[i];i++)cap+=strlen(args[i])*2+4;
    char *command=calloc(cap,1);size_t at=0;
    if(command)for(int i=0;args[i];i++) {
        if(i)command[at++]=' ';
        command[at++]='"';const char *p=args[i];
        while(*p){unsigned slashes=0;while(*p=='\\'){slashes++;p++;}
            unsigned copies=(*p=='"'||!*p)?slashes*2:slashes;while(copies--)command[at++]='\\';
            if(*p=='"')command[at++]='\\';
            if(*p)command[at++]=*p++;
        }command[at++]='"';command[at]=0;
    }
    wchar_t *application=wide(engine),*line=command?wide(command):NULL,*cwd=wide(a->base);STARTUPINFOW start;PROCESS_INFORMATION process;SDL_zero(start);start.cb=sizeof(start);
    if(argc>1){start.dwFlags=STARTF_USESTDHANDLES;start.hStdInput=GetStdHandle(STD_INPUT_HANDLE);start.hStdOutput=GetStdHandle(STD_OUTPUT_HANDLE);start.hStdError=GetStdHandle(STD_ERROR_HANDLE);}
    if(application&&line&&cwd&&CreateProcessW(application,line,NULL,NULL,argc>1,0,NULL,cwd,&start,&process)){
        CloseHandle(process.hThread);
        if(argc>1){WaitForSingleObject(process.hProcess,INFINITE);DWORD exit_code=1;GetExitCodeProcess(process.hProcess,&exit_code);result=(int)exit_code+1;}
        else result=1;
        CloseHandle(process.hProcess);
    }
    free(application);free(line);free(cwd);free(command);
#else
    if(chdir(a->base)==0)execv(engine,args);
#endif
    if(!result)SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,REMASTERED_NAME,"The game could not start. Check that the complete download is extracted and the runtime executable has permission to run.",NULL);
    free(args);free(engine);free(argv0);return result;
}

int main(int argc,char **argv) {
    Launcher a;SDL_zero(a);a.running=1;a.base=SDL_GetBasePath();
    if(!a.base)return 1;
    /* Developer bridge used by Python fixture tools for archive inputs. */
    if(argc==3&&!strcmp(argv[1],"--mac-source")){
        unsigned char *bundle=NULL;size_t size=0;char error[512];
        int ok=mac_import_source_bundle(argv[2],&bundle,&size,error,sizeof(error));
#ifdef _WIN32
        _setmode(_fileno(stdout),_O_BINARY);
#endif
        if(ok)ok=fwrite(bundle,1,size,stdout)==size;
        else fprintf(stderr,"%s\n",error);
        free(bundle);SDL_free(a.base);return ok?0:1;
    }
    /* Developer/native importer entry; normal players use the setup window. */
    if(argc==4&&!strcmp(argv[1],"--import-mac")){char error[512];int ok=mac_import_disk(argv[2],argv[3],error,sizeof(error));if(!ok)fprintf(stderr,"%s\n",error);SDL_free(a.base);return ok?0:1;}
    /* Preserve --version, replay, screenshot and other command-line workflows. */
    if(argc>1){int result=launch_engine(&a,argc,argv,NULL);SDL_free(a.base);return result?result-1:1;}
    a.pack=join(a.base,"data/res_mac_native_full.pak");a.config=join(a.base,"data/mac-launcher.cfg");
    if(!a.pack||!a.config){free(a.pack);free(a.config);SDL_free(a.base);return 1;}
    /* A prepared installation goes directly to the game, without creating a
     * launcher window or reading the original disk image again. */
    if(mac_import_pack_status(a.pack)==1){int result=launch_engine(&a,1,argv,NULL);free(a.pack);free(a.config);SDL_free(a.base);return result?0:1;}
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER)){free(a.pack);free(a.config);SDL_free(a.base);return 1;}
    a.window=SDL_CreateWindow(REMASTERED_NAME " - Launcher",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,W,H,SDL_WINDOW_ALLOW_HIGHDPI|SDL_WINDOW_RESIZABLE);
    a.renderer=a.window?SDL_CreateRenderer(a.window,-1,SDL_RENDERER_SOFTWARE):NULL;
    if(!a.renderer){SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,REMASTERED_NAME,"Could not open the launcher window.",a.window);if(a.window)SDL_DestroyWindow(a.window);SDL_Quit();free(a.pack);free(a.config);SDL_free(a.base);return 1;}
    SDL_RenderSetLogicalSize(a.renderer,W,H);
    int status=mac_import_pack_status(a.pack);a.ready=status==1;
    SDL_strlcpy(a.message,a.ready?"Your Macintosh graphics are ready. Press Play to start.":"Copy your game disk image beside this launcher, or use Browse. Graphics will be prepared automatically.",sizeof(a.message));
    if(status<0)SDL_strlcpy(a.message,"The saved graphics file is damaged. Move data/res_mac_native_full.pak out of the game folder, then reopen the launcher.",sizeof(a.message));
    char *found=NULL;int images=mac_import_find_disk(a.base,&found);
    if(images==1&&found){if(status<0){a.disk=found;a.disk_found=1;}else{select_disk(&a,found);free(found);}}
    else {
        free(found);char saved[PATH_CAP]={0};SDL_RWops *f=SDL_RWFromFile(a.config,"rb");
        if(f){size_t n=SDL_RWread(f,saved,1,sizeof(saved)-1);saved[n]=0;SDL_RWclose(f);}
        if(readable(saved)){if(status<0){a.disk=SDL_strdup(saved);a.disk_found=1;}else select_disk(&a,saved);}
        else if(images>1)SDL_strlcpy(a.message,"Several disk images were found. Use Browse to choose your Macintosh colour game image.",sizeof(a.message));
    }
    int play=0;
    while(a.running) {
        if(a.worker&&SDL_AtomicGet(&a.job.done)) {
            SDL_WaitThread(a.worker,NULL);a.worker=NULL;a.ready=a.job.success||mac_import_pack_status(a.pack)==1;a.disk_found=readable(a.disk);
            SDL_strlcpy(a.message,a.ready?"Macintosh graphics ready. Starting the game...":a.job.error,sizeof(a.message));
            if(a.ready){play=1;a.running=0;}
        }
        SDL_Event event;
        while(a.running&&SDL_PollEvent(&event)) {
            if(event.type==SDL_QUIT)a.running=0;
            if(event.type==SDL_KEYDOWN&&event.key.keysym.sym==SDLK_ESCAPE){if(a.browse){a.browse=0;SDL_StopTextInput();}else a.running=0;}
            if(event.type==SDL_MOUSEBUTTONUP&&event.button.button==SDL_BUTTON_LEFT) {
                /* SDL_RenderSetLogicalSize already scales mouse events. */
                int x=event.button.x,y=event.button.y;
                if(!a.browse) {
                    if(!a.worker&&inside(x,y,(SDL_Rect){36,311,224,42}))browse(&a);
                    else if(!a.worker&&inside(x,y,(SDL_Rect){36,481,224,44})){play=2;a.running=0;}
                    else if(a.ready&&!a.worker&&inside(x,y,(SDL_Rect){540,476,224,54})){play=1;a.running=0;}
                }
#ifndef _WIN32
                else {
                    if(inside(x,y,(SDL_Rect){648,147,116,34})){a.browse=0;SDL_StopTextInput();}
                    else if(inside(x,y,(SDL_Rect){36,147,88,34})){char parent[PATH_CAP];SDL_strlcpy(parent,a.directory,sizeof(parent));size_t n=strlen(parent);if(n>1&&parent[n-1]=='/')parent[--n]=0;char*p=strrchr(parent,'/');if(p)p[1]=0;open_directory(&a,parent);}
                    else if(inside(x,y,(SDL_Rect){136,147,100,34}))browse(&a);
                    else if(inside(x,y,(SDL_Rect){248,147,100,34}))open_directory(&a,"/");
                    else if(inside(x,y,(SDL_Rect){648,474,116,36}))choose_path(&a);
                    else if(x>=36&&x<764&&y>=227&&y<458){int row=(y-227)/33+a.scroll;if(row<a.count){char path[PATH_CAP];SDL_snprintf(path,sizeof(path),"%s%s",a.directory,a.files[row].name);if(a.files[row].directory)open_directory(&a,path);else{a.browse=0;SDL_StopTextInput();select_disk(&a,path);}}}
                }
#endif
            }
#ifndef _WIN32
            if(a.browse) {
                if(event.type==SDL_MOUSEWHEEL){a.scroll-=event.wheel.y*3;if(a.scroll<0)a.scroll=0;int max=a.count>7?a.count-7:0;if(a.scroll>max)a.scroll=max;}
                if(event.type==SDL_TEXTINPUT)SDL_strlcat(a.path_input,event.text.text,sizeof(a.path_input));
                if(event.type==SDL_KEYDOWN) {
                    if(event.key.keysym.sym==SDLK_RETURN)choose_path(&a);
                    if(event.key.keysym.sym==SDLK_BACKSPACE){size_t n=strlen(a.path_input);if(n){do{n--;}while(n&&((unsigned char)a.path_input[n]&192)==128);a.path_input[n]=0;}}
                    if((event.key.keysym.mod&KMOD_CTRL)&&event.key.keysym.sym==SDLK_a)a.path_input[0]=0;
                    if((event.key.keysym.mod&KMOD_CTRL)&&event.key.keysym.sym==SDLK_v){char*clip=SDL_GetClipboardText();if(clip){SDL_strlcat(a.path_input,clip,sizeof(a.path_input));SDL_free(clip);}}
                }
            }
#endif
        }
        draw(&a);SDL_Delay(16);
    }
    if(a.worker)SDL_WaitThread(a.worker,NULL); /* Finish the atomic import before exit. */
    int result=0;
    if(play){SDL_DestroyRenderer(a.renderer);a.renderer=NULL;SDL_DestroyWindow(a.window);a.window=NULL;SDL_Quit();result=launch_engine(&a,1,argv,play==1?"mac_native_full":"dos")?0:1;}
#ifndef _WIN32
    clear_browser(&a);
#endif
    free(a.disk);free(a.pack);free(a.config);SDL_free(a.base);SDL_DestroyRenderer(a.renderer);SDL_DestroyWindow(a.window);SDL_Quit();return result;
}

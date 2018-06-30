#include <exception>
#include <functional>
#include <string>
#include <vector>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <SDL2/SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL2/SDL_opengles2.h>

#include "InputManager.h"

extern "C" {
#include "../config.h"
#include "yabause.h"
#include "vdp2.h"
#include "scsp.h"
#include "vidsoft.h"
#include "vidogl.h"
#include "peripheral.h"
#include "persdljoy.h"
#include "m68kcore.h"
#include "sh2core.h"
#include "sh2int.h"
#include "cdbase.h"
#include "cs2.h"
#include "debug.h"
#include "sndal.h"
#include "sndsdl.h"
#include "osdcore.h"
#include "ygl.h"

static char biospath[256] = "/home/pigaming/RetroPie/BIOS/saturn/bios.bin";
static char cdpath[256] = "/home/pigaming/RetroPie/roms/saturn/nights.cue";
static char buppath[256] = "./back.bin";
static char mpegpath[256] = "\0";
static char cartpath[256] = "\0";

#define LOG

M68K_struct * M68KCoreList[] = {
  &M68KDummy,
  #ifdef HAVE_C68K
  &M68KC68K,
  #endif
  #ifdef HAVE_Q68
  &M68KQ68,
  #endif
#ifdef HAVE_MUSASHI
  &M68KMusashi,
#endif
  NULL
};

SH2Interface_struct *SH2CoreList[] = {
  &SH2Interpreter,
  &SH2DebugInterpreter,
#ifdef SH2_DYNAREC
  &SH2Dynarec,
#endif
#if DYNAREC_DEVMIYAX
  &SH2Dyn,
  &SH2DynDebug,
#endif
  NULL
};

PerInterface_struct *PERCoreList[] = {
  &PERDummy,
  &PERSDLJoy,
  NULL
};

CDInterface *CDCoreList[] = {
  &DummyCD,
  &ISOCD,
  NULL
};

SoundInterface_struct *SNDCoreList[] = {
  &SNDDummy,
#ifdef HAVE_LIBSDL
  &SNDSDL,
#endif
  NULL
};

VideoInterface_struct *VIDCoreList[] = {
  &VIDDummy,
  &VIDOGL,
  NULL
};

/*
#ifdef YAB_PORT_OSD
#include "nanovg/nanovg_osdcore.h"
OSD_struct *OSDCoreList[] = {
  &OSDNnovg,
  NULL
};
#endif
*/

OSD_struct *OSDCoreList[] = {
  &OSDDummy,
  NULL
};

static SDL_Window* wnd;
static SDL_GLContext glc;

void DrawDebugInfo()
{
}

int g_EnagleFPS = 0;

void YuiErrorMsg(const char *string)
{
  LOG("%s",string);
}

void YuiSwapBuffers(void)
{
  SDL_GL_SwapWindow(wnd);
  //SetOSDToggle(1);
}

int YuiRevokeOGLOnThisThread(){
  LOG("revoke thread\n");
  SDL_GL_MakeCurrent(wnd,nullptr);
  return 0;
}

int YuiUseOGLOnThisThread(){
  LOG("use thread\n");
  SDL_GL_MakeCurrent(wnd,glc);
  return 0;
}

}

int g_resolution_mode = 0;
int g_keep_aspect_rate = 0;
int g_scsp_sync = 1;
int g_frame_skip = 0;
int g_emulated_bios = 1;
InputManager* inputmng;

int saveScreenshot( const char * filename );

int yabauseinit()
{
  int res;
  yabauseinit_struct yinit = {};
  void * padbits;
  inputmng = InputManager::getInstance();
  

  yinit.m68kcoretype = M68KCORE_MUSASHI;
  yinit.percoretype = PERCORE_DUMMY;
#ifdef SH2_DYNAREC
    yinit.sh2coretype = 2;
#else
  //yinit.sh2coretype = 0;
#endif
  yinit.sh2coretype = 3;
  //yinit.vidcoretype = VIDCORE_SOFT;
  yinit.vidcoretype = 1;
  yinit.sndcoretype = SNDCORE_SDL;
  //yinit.sndcoretype = SNDCORE_DUMMY;
  //yinit.cdcoretype = CDCORE_DEFAULT;
  yinit.cdcoretype = CDCORE_ISO;
  yinit.carttype = CART_NONE;
  yinit.regionid = 0;
  if( g_emulated_bios ){
    yinit.biospath = NULL;
  }else{
    yinit.biospath = biospath;
  }
  yinit.cdpath = cdpath;
  yinit.buppath = buppath;
  yinit.mpegpath = mpegpath;
  yinit.cartpath = cartpath;
  yinit.videoformattype = VIDEOFORMATTYPE_NTSC;
  yinit.frameskip = g_frame_skip;
  yinit.usethreads = 0;
  yinit.skip_load = 0;    
  yinit.video_filter_type = 0;
  yinit.polygon_generation_mode = PERSPECTIVE_CORRECTION; ////GPU_TESSERATION;
  yinit.use_new_scsp = 1;
  yinit.resolution_mode = g_resolution_mode;
  yinit.rotate_screen = 0;
  yinit.scsp_sync_count_per_frame = g_scsp_sync;
  yinit.extend_backup = 1;

    res = YabauseInit(&yinit);
    if( res == -1)
    {
        return -1;
    }
  inputmng->init();
#if 0
  PerPortReset();
  padbits = PerPadAdd(&PORTDATA1);
  PerSetKey(SDLK_UP, PERPAD_UP, padbits);
  PerSetKey(SDLK_RIGHT, PERPAD_RIGHT, padbits);
  PerSetKey(SDLK_DOWN, PERPAD_DOWN, padbits);
  PerSetKey(SDLK_LEFT, PERPAD_LEFT, padbits);
  PerSetKey(SDLK_q, PERPAD_RIGHT_TRIGGER, padbits);
  PerSetKey(SDLK_e, PERPAD_LEFT_TRIGGER, padbits);
  PerSetKey(SDLK_RETURN, PERPAD_START, padbits);
  PerSetKey(SDLK_z, PERPAD_A, padbits);
  PerSetKey(SDLK_x, PERPAD_B, padbits);
  PerSetKey(SDLK_c, PERPAD_C, padbits);
  PerSetKey(SDLK_a, PERPAD_X, padbits);
  PerSetKey(SDLK_s, PERPAD_Y, padbits);
  PerSetKey(SDLK_d, PERPAD_Z, padbits);
#endif
  //OSDInit(0);
  //OSDChangeCore(OSDCORE_NANOVG);
  
  LogStart();
  LogChangeOutput(DEBUG_CALLBACK, NULL);

  return 0;
}

using std::string;
#include "MenuScreen.h"

int main(int argc, char** argv)
{

  inputmng = InputManager::getInstance();
  MenuScreen * menu;

  // Inisialize home directory
  std::string home_dir = getenv("HOME");
  home_dir += "/.yabasanshiro";
  struct stat st = {0};
  if (stat(home_dir.c_str(), &st) == -1) {
    mkdir(home_dir.c_str(), 0700);
  }  
  home_dir += "backup.bin";
  strcpy( buppath, home_dir.c_str() );

  std::string current_exec_name = argv[0]; // Name of the current exec program
  std::vector<std::string> all_args;
  if (argc > 1) {
    all_args.assign(argv + 1, argv + argc);
    if( all_args[0] == "-h" || all_args[0] == "--h" ){
      printf("Usage:\n");
      printf("  -b STRING  --bios STRING                 bios file\n");
      printf("  -i STRING  --iso STRING                  iso/cue file\n");
      printf("  -r NUMBER  --resolution_mode NUMBER      0 .. Native, 1 .. 4x, 2 .. 2x, 3 .. Original\n");
      printf("  -a         --keep_aspect_rate\n");
      printf("  -s NUMBER  --scps_sync_per_frame NUMBER\n");
      printf("  -f         --frame_skip\n");    
      printf("  -v         --version\n");    
      exit(0);
    }
  }

  for( int i=0; i<all_args.size(); i++ ){
    string x = all_args[i];
		if(( x == "-b" || x == "--bios") && (i+1<all_args.size() ) ) {
      g_emulated_bios = 0;
      strncpy(biospath, all_args[i+1].c_str(), 256);
    }
		else if(( x == "-i" || x == "--iso") && (i+1<all_args.size() ) ) {
      strncpy(cdpath, all_args[i+1].c_str(), 256);
    }
		else if(( x == "-r" || x == "--resolution_mode") && (i+1<all_args.size() ) ) {
      g_resolution_mode = std::stoi( all_args[i+1] );
    }
		else if(( x == "-a" || x == "--keep_aspect_rate") ) {
      g_keep_aspect_rate = 1;
    }
		else if(( x == "-s" || x == "--g_scsp_sync")&& (i+1<all_args.size() ) ) {
      g_scsp_sync = std::stoi( all_args[i+1] );
    }
		else if(( x == "-f" || x == "--frame_skip") ) {
      g_frame_skip = 1;
    }
		else if(( x == "-v" || x == "--version") ) {
      printf("YabaSanshiro version %s(%s)\n",YAB_VERSION, GIT_SHA1 );
      return 0;
    }
	}

  if( SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO) != 0 ) {
    printf("Fail to init SDL Bye! (%s)", SDL_GetError() );
    return -1;
  }

  SDL_DisplayMode dsp;
  if( SDL_GetCurrentDisplayMode(0,&dsp) != 0 ){
    printf("Fail to SDL_GetCurrentDisplayMode Bye! (%s)", SDL_GetError() );
    return -1;
  }
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  SDL_GL_SetSwapInterval(0);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);


  int width = dsp.w;
  int height = dsp.h;
  wnd = SDL_CreateWindow("Yaba Snashiro", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP);
  if(wnd == nullptr ) {
    printf("Fail to SDL_CreateWindow Bye! (%s)", SDL_GetError() );
    return -1;
  }
  

  dsp.refresh_rate = 60;
  SDL_SetWindowDisplayMode(wnd,&dsp);
  SDL_GetWindowSize(wnd,&width,&height);
  SDL_SetWindowInputFocus(wnd);
  glc = SDL_GL_CreateContext(wnd);
  if(glc == nullptr ) {
    printf("Fail to SDL_GL_CreateContext Bye! (%s)", SDL_GetError() );
    return -1;
  }

  printf("context renderer string: \"%s\"\n", glGetString(GL_RENDERER));
  printf("context vendor string: \"%s\"\n", glGetString(GL_VENDOR));
  printf("version string: \"%s\"\n", glGetString(GL_VERSION));
  printf("Extentions: %s\n",glGetString(GL_EXTENSIONS));

  menu = new MenuScreen(wnd,width,height);

  if( yabauseinit() == -1 ) {
      printf("Fail to yabauseinit Bye! (%s)", SDL_GetError() );
      return -1;
  }

  if( g_keep_aspect_rate ){
    int originx = 0;
    int originy = 0;
    int specw = width;
    int spech = height;
    float specratio = (float)specw / (float)spech;
    int saturnw = 4;
    int saturnh = 3;
    float saturnraito = (float)saturnw/ (float)saturnh;
    float revraito = (float) saturnh/ (float)saturnw;
    if( specratio > saturnraito ){
            width = spech * saturnraito;
            height = spech;
            originx = (dsp.w - width)/2.0;
            originy = 0;
    }else{
        width = specw ;
        height = specw * revraito;
        originx = 0;
        originy = spech - height;
    }
    VIDCore->Resize(originx,originy,width,height,0);
  }else{
    VIDCore->Resize(0,0,width,height,0);
  }
  SDL_GL_MakeCurrent(wnd,nullptr);
  YabThreadSetCurrentThreadAffinityMask(0x00);

  Uint32 evToggleMenu = SDL_RegisterEvents(1);
  inputmng->setToggleMenuEventCode(evToggleMenu);

  Uint32  evResetMenu = SDL_RegisterEvents(1);
  menu->setResetMenuEventCode(evResetMenu);

  Uint32  evPadMenu = SDL_RegisterEvents(1);
  menu->setTogglePadModeMenuEventCode(evPadMenu);

  int padmode = 0;
  
  bool menu_show = false;

  while(true) {
    SDL_Event e;
    while(SDL_PollEvent(&e)) {
      if(e.type == SDL_QUIT){
        glClearColor(0.0,0.0,0.0,1.0);
        glClear(GL_COLOR_BUFFER_BIT);        
        SDL_GL_SwapWindow(wnd);
        YabauseDeInit();
        SDL_Quit();
        return 0;
      }

      else if(e.type == evToggleMenu){
        if( menu_show ){
          menu_show = false;
          inputmng->setMenuLayer(nullptr);
          SDL_GL_MakeCurrent(wnd,nullptr);
          VdpResume();
          SNDSDL.UnMuteAudio();          
        }else{
          menu_show = true;
          SNDSDL.MuteAudio();
          VdpRevoke();
          inputmng->setMenuLayer(menu);
          SDL_GL_MakeCurrent(wnd,glc);
          saveScreenshot("tmp.png");
          glUseProgram(0);
          glGetError();
          glBindBuffer(GL_ARRAY_BUFFER, 0);
          glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0);
          glDisableVertexAttribArray(0);
          glDisableVertexAttribArray(1);
          glDisableVertexAttribArray(2);
          glDisable(GL_DEPTH_TEST);
          glDisable(GL_SCISSOR_TEST);
          glDisable(GL_STENCIL_TEST);
          glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);   
          menu->setBackGroundImage( std::string("tmp.png") );
        }
      }

      else if(e.type == evResetMenu){
        YabauseReset();
        menu_show = false;
        inputmng->setMenuLayer(nullptr);
        SDL_GL_MakeCurrent(wnd,nullptr);
        VdpResume();
        SNDSDL.UnMuteAudio(); 
      }

      else if(e.type == evPadMenu ){
        if( padmode == 0 ){
          padmode = 1;
        }else{
          padmode = 0;
        }
        inputmng->setGamePadomode( 0, padmode );
        menu_show = false;
        inputmng->setMenuLayer(nullptr);
        SDL_GL_MakeCurrent(wnd,nullptr);
        VdpResume();
        SNDSDL.UnMuteAudio();         
      }

      inputmng->parseEvent(e);
      if( menu_show ){
        menu->onEvent( e );
      }
    }
    inputmng->handleJoyEvents();

    if( menu_show ){
      glClearColor(0.0f, 0.0f, 0.0f, 1);
      glClear(GL_COLOR_BUFFER_BIT);
      menu->drawAll();
      SDL_GL_SwapWindow(wnd);
    }else{
      YabauseExec(); // exec one frame
    }
  }
  //YabauseDeInit();
  SDL_Quit();
  return 0;
}

extern "C" {
#include "libpng/png.h"
}
#define YUI_LOG printf

int saveScreenshot( const char * filename ){
    
    int width;
    int height;
    unsigned char * buf = NULL;
    unsigned char * bufRGB = NULL;
    png_bytep * row_pointers = NULL;
    int quality = 100; // best
    FILE * outfile = NULL;
    int row_stride;
    int glerror;
    int u,v;
    int pmode;
    png_byte color_type;
    png_byte bit_depth; 
    png_structp png_ptr;
    png_infop info_ptr;
    int number_of_passes;
    int rtn = -1;
  
    SDL_GetWindowSize( wnd, &width, &height);
    buf = (unsigned char *)malloc(width*height*4);
    if( buf == NULL ) {
        YUI_LOG("not enough memory\n");
        goto FINISH;
    }

    glReadBuffer(GL_BACK);
    pmode = GL_RGBA;
    glGetError();
    glReadPixels(0, 0, width, height, pmode, GL_UNSIGNED_BYTE, buf);
    if( (glerror = glGetError()) != GL_NO_ERROR ){
        YUI_LOG("glReadPixels %04X\n",glerror);
         goto FINISH;
    }
	
	for( u = 3; u <width*height*4; u+=4 ){
		buf[u]=0xFF;
	}
    row_pointers = (png_byte**)malloc(sizeof(png_bytep) * height);
    for (v=0; v<height; v++)
        row_pointers[v] = (png_byte*)&buf[ (height-1-v) * width * 4];

    // save as png
    if ((outfile = fopen(filename, "wb")) == NULL) {
        YUI_LOG("can't open %s\n", filename);
        goto FINISH;
    }

    /* initialize stuff */
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (!png_ptr){
        YUI_LOG("[write_png_file] png_create_write_struct failed");
        goto FINISH;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr){
        YUI_LOG("[write_png_file] png_create_info_struct failed");
        goto FINISH;
    }

    if (setjmp(png_jmpbuf(png_ptr))){
        YUI_LOG("[write_png_file] Error during init_io");
        goto FINISH;
    }
    /* write header */
    png_init_io(png_ptr, outfile);
    
    if (setjmp(png_jmpbuf(png_ptr))){
        YUI_LOG("[write_png_file] Error during writing header");
        goto FINISH;
    }
    bit_depth = 8;
    color_type = PNG_COLOR_TYPE_RGB_ALPHA;
    png_set_IHDR(png_ptr, info_ptr, width, height,
        bit_depth, color_type, PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
    //png_set_gAMA(png_ptr, info_ptr, 1.0);
    {
        png_text text[3];
        int txt_fields = 0;
        char desc[256];
        
        time_t      gmt;
        png_time    mod_time;
        
        time(&gmt);
        png_convert_from_time_t(&mod_time, gmt);
        png_set_tIME(png_ptr, info_ptr, &mod_time);
    
        text[txt_fields].key = "Title";
        text[txt_fields].text = Cs2GetCurrentGmaecode();
        text[txt_fields].compression = PNG_TEXT_COMPRESSION_NONE;
        txt_fields++;

        sprintf( desc, "Yaba Sanshiro Version %s\n VENDER: %s\n RENDERER: %s\n VERSION %s\n",YAB_VERSION,glGetString(GL_VENDOR),glGetString(GL_RENDERER),glGetString(GL_VERSION));
        text[txt_fields].key ="Description";
        text[txt_fields].text=desc;
        text[txt_fields].compression = PNG_TEXT_COMPRESSION_NONE;
        txt_fields++;
        
        png_set_text(png_ptr, info_ptr, text,txt_fields);
    }       
    png_write_info(png_ptr, info_ptr);


    /* write bytes */
    if (setjmp(png_jmpbuf(png_ptr))){
        YUI_LOG("[write_png_file] Error during writing bytes");
        goto FINISH;
    }
    png_write_image(png_ptr, row_pointers);

    /* end write */
    if (setjmp(png_jmpbuf(png_ptr))){
        YUI_LOG("[write_png_file] Error during end of write");
        goto FINISH;
    }
    
    png_write_end(png_ptr, NULL);
    rtn = 0;
FINISH: 
    if(outfile) fclose(outfile);
    if(buf) free(buf);
    if(bufRGB) free(bufRGB);
    if(row_pointers) free(row_pointers);
    return rtn;
}

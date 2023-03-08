#include <android/log.h>
#include <android_native_app_glue.h>
#include <EGL/egl.h>
#include <GLES/gl.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO,"myName", __VA_ARGS__))

static EGLSurface surface;
static EGLDisplay disp;
static EGLContext ctx;

//create EGL surface & ctx. attach ctx to surface
static void initEGL(android_app* app)
{
 disp = eglGetDisplay(EGL_DEFAULT_DISPLAY);
 eglInitialize(disp, nullptr, nullptr);
 
 // desired attributes of FB configuration
 const EGLint attribs[] = {
     EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
     EGL_BLUE_SIZE, 8,
     EGL_GREEN_SIZE, 8,
     EGL_RED_SIZE, 8,
     EGL_NONE};
 EGLint numConfigs;
 
 // get num of config that match given attrib
 eglChooseConfig(disp, attribs, nullptr, 0, &numConfigs);
 EGLConfig configs[numConfigs];
 
 //get and store configs matching attr in configs
 eglChooseConfig(disp, attribs, configs, numConfigs, &numConfigs);
 size_t i = 0;
 EGLConfig config;
 
 //loop over each config and find one which has r=g=b=8bit
 for (; i < numConfigs; i++)
 {
  auto &cfg = configs[i];
  EGLint r, g, b, d;
  if (eglGetConfigAttrib(disp, cfg, EGL_RED_SIZE, &r) &&
      eglGetConfigAttrib(disp, cfg, EGL_GREEN_SIZE, &g) &&
      eglGetConfigAttrib(disp, cfg, EGL_BLUE_SIZE, &b) &&
      eglGetConfigAttrib(disp, cfg, EGL_DEPTH_SIZE, &d) &&
      r == 8 && g == 8 && b == 8 && d == 0)
  {
   LOGI("matchedConfig");
   config = configs[i];
   break;
  }
 }
 
 //if not found, use the first config
 if (i == numConfigs)
 {
  config = configs[0];
  LOGI("1stConfig");
 }
 //create surface & ctx using disp & config
 surface = eglCreateWindowSurface(disp, config, app->window, nullptr);
 ctx = eglCreateContext(disp, config, nullptr, nullptr);

//attach ctx to surface
 if (eglMakeCurrent(disp, surface, surface, ctx) == EGL_FALSE)
 {
  LOGI("unable to eglMakeCurrent");
  return;
 }
}

//init openGL
static void initGLES()
{
 glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
 glEnable(GL_CULL_FACE);
 glShadeModel(GL_SMOOTH);
 glDisable(GL_DEPTH_TEST);
}

//draw blank blue screen
static void drawFrame()
{
 if(disp == nullptr) return;
 glClearColor(0,0,1,1);
 glClear(GL_COLOR_BUFFER_BIT);
 eglSwapBuffers(disp,surface);
}

static void destroyDisp()
{
 if(disp != EGL_NO_DISPLAY)
 {
  //make current ctx to noContext
   eglMakeCurrent(disp,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);
   //destroy ctx
   if(ctx != EGL_NO_CONTEXT)
    eglDestroyContext(disp,ctx);
   //destroy surface
   if(surface != EGL_NO_SURFACE)
    eglDestroySurface(disp,surface);
   //destroy display
   eglTerminate(disp);
 }
}

//it executed when an activity event occurs
static void onActivityStateChange(android_app* app, int32_t cmd)
{
 switch (cmd)
 {
 case APP_CMD_INIT_WINDOW: // if window is created, call initDisplay & drawFrame
   if (app->window != nullptr)
   {
    initEGL(app);
    initGLES();
    drawFrame();
   }
   break;
 case APP_CMD_TERM_WINDOW: //if window needs to be terminated
  destroyDisp();
  break;
 default:
  break;
 }
}

void android_main(android_app* app)
{
 app->onAppCmd = onActivityStateChange; //register main cmd handler func
 while(true)
 {
  android_poll_source* src;
  int events;
  //check any event occurs(event in event queue)
  int ident = ALooper_pollAll(-1,nullptr,&events,(void**)&src);
  if(ident >= 0) //if occurs
  {
   if(src != nullptr)
     src->process(app,src); //run the onActivityStateChange
   
   if(app->destroyRequested != 0)
   {
    destroyDisp();
    return;
   }
  }
 }
}
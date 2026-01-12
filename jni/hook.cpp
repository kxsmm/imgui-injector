#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_android.h>
#include <Dobby/dobby.h>
#include <dlfcn.h>
#include <pthread.h>
#include <unistd.h>
#include <xdl.h>
#include <algorithm>
#include <Utils.h>
#include <Logger.h>
#include <font.h>
// 全局状态
static bool g_initialized = false;
static int g_surfaceWidth = 0, g_surfaceHeight = 0;
static int g_windowWidth = 0, g_windowHeight = 0;
static float g_uiScale = 1.0f;
static ANativeWindow* g_nativeWindow = nullptr;

// 原始函数指针
typedef EGLBoolean (*T_eglSwapBuffers)(EGLDisplay, EGLSurface);
typedef bool (*MotionEvent_copyFrom)(void*, void*, bool);
typedef int32_t (*ANativeWindow_GetSize)(ANativeWindow*);

static T_eglSwapBuffers orig_eglSwapBuffers = nullptr;
static MotionEvent_copyFrom orig_MotionEventCopy = nullptr;
static ANativeWindow_GetSize orig_GetWidth = nullptr;
static ANativeWindow_GetSize orig_GetHeight = nullptr;

//--------------------------------------------------
// 输入事件处理
//--------------------------------------------------
bool hook_MotionEventCopy(void* thiz, void* other, bool keepHistory) {
    bool ret = orig_MotionEventCopy(thiz, other, keepHistory);
    
    if (g_initialized) {
        AInputEvent* event = reinterpret_cast<AInputEvent*>(thiz);
        
        // 计算坐标转换比例
        float scaleX = (g_windowWidth != 0) ? 
            (((float)g_windowWidth / g_surfaceWidth)) : 1.0f;
        float scaleY = (g_windowHeight != 0) ? 
            (((float)g_windowHeight / g_surfaceHeight)) : 1.0f;

        ImGui_ImplAndroid_HandleTouchEvent(event, ImVec2(scaleX, scaleY));
    }
    return ret;
}

//--------------------------------------------------
// 窗口尺寸Hook
//--------------------------------------------------
int32_t hook_GetWidth(ANativeWindow* window) {
    g_nativeWindow = window;
    g_windowWidth = orig_GetWidth(window);
    return g_windowWidth;
}

int32_t hook_GetHeight(ANativeWindow* window) {
    g_windowHeight = orig_GetHeight(window);
    return g_windowHeight;
}

//--------------------------------------------------
// EGL交换缓冲Hook
//--------------------------------------------------
EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    // 获取表面尺寸
    eglQuerySurface(dpy, surface, EGL_WIDTH, &g_surfaceWidth);
    eglQuerySurface(dpy, surface, EGL_HEIGHT, &g_surfaceHeight);

    // 初始化ImGui
    if (!g_initialized && g_surfaceWidth > 0 && g_surfaceHeight > 0) {

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        
        ImFontConfig font_cfg;
       // font_cfg.SizePixels = 50.0f;
        io.Fonts->AddFontFromMemoryTTF((void *) OPPOSans_H, OPPOSans_H_size, 25.0f, NULL, io.Fonts->GetGlyphRangesChineseFull());
        io.Fonts->AddFontDefault(&font_cfg);
         ImGuiStyle& style = ImGui::GetStyle();
         style.ScaleAllSizes(3.2f); // 2倍放大

        ImGui_ImplOpenGL3_Init("#version 300 es");
   //     ImGui_ImplAndroid_Init(g_surfaceWidth, g_surfaceHeight);
        
        // 样式配置
        // ImGuiStyle& style = ImGui::GetStyle();
        // style.ScaleAllSizes(g_uiScale);
        //style.TouchExtraPadding = ImVec2(10 * g_uiScale, 10 * g_uiScale);
        style.FramePadding = ImVec2(12.0f, 9.0f);
        g_initialized = true;
    }

    // 更新显示尺寸
    if (g_initialized) {
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(g_surfaceWidth, g_surfaceHeight);
        
        // 开始新帧
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplAndroid_NewFrame(g_surfaceWidth,g_surfaceHeight);
        ImGui::NewFrame();

		ImGui::SetNextWindowSize(ImVec2(400, 400), ImGuiCond_FirstUseEver);
		ImGui::Begin("Mod Menu");
		ImGui::End();
        ImGui::EndFrame();
        glViewport(0, 0, g_surfaceWidth, g_surfaceHeight);
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    return orig_eglSwapBuffers(dpy, surface);
}

//--------------------------------------------------
// Hook初始化
//--------------------------------------------------
__attribute__((constructor)) void Init() {
    // Hook窗口尺寸获取
    while (!isLibraryLoaded("libandroid.so")) sleep(1);
    void* androidLib = dlopen("libandroid.so", RTLD_NOW);
    DobbyHook(dlsym(androidLib, "ANativeWindow_getWidth"), 
             (void*)hook_GetWidth, (void**)&orig_GetWidth);
    DobbyHook(dlsym(androidLib, "ANativeWindow_getHeight"), 
             (void*)hook_GetHeight, (void**)&orig_GetHeight);

    // Hook输入事件
    void* inputSym = DobbySymbolResolver("/system/lib/libinput.so", 
                                      "_ZN7android11MotionEvent8copyFromEPKS0_b");
    if (inputSym) {
        DobbyHook(inputSym, (void*)hook_MotionEventCopy, (void**)&orig_MotionEventCopy);
    }

    // Hook EGL交换缓冲
    void* eglLib = dlopen("libEGL.so", RTLD_NOW);
    DobbyHook(dlsym(eglLib, "eglSwapBuffers"), 
             (void*)hook_eglSwapBuffers, (void**)&orig_eglSwapBuffers);
}

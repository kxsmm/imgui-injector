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
#include <string.h>
#include <stdio.h>
#include <fstream>
#include <sstream>

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

extern "C" int luaopen_ImGui(lua_State* L);
extern "C" int luaopen_Mem(lua_State* L);
extern "C" void create_imdrawlist_metatable(lua_State* L);
// --------------------------------------------------------------
// 日志输出辅助函数 (print.D, print.I, print.W, print.E)
// --------------------------------------------------------------
static std::string lua_concat_args(lua_State* L) {
	int n = lua_gettop(L);
	lua_getglobal(L, "tostring");
	std::string output;
	for (int i = 1; i <= n; i++) {
		lua_pushvalue(L, -1);
		lua_pushvalue(L, i);
		lua_call(L, 1, 1);
		const char* s = lua_tostring(L, -1);
		if (s) {
			if (i > 1) output += "\t";
			output += s;
		}
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
	return output;
}

static int lua_print_debug(lua_State* L) {
	std::string msg = lua_concat_args(L);
	LOGD("%s", msg.c_str());
	return 0;
}
static int lua_print_info(lua_State* L) {
	std::string msg = lua_concat_args(L);
	LOGI("%s", msg.c_str());
	return 0;
}
static int lua_print_warn(lua_State* L) {
	std::string msg = lua_concat_args(L);
	LOGW("%s", msg.c_str());
	return 0;
}
static int lua_print_error(lua_State* L) {
	std::string msg = lua_concat_args(L);
	LOGE("%s", msg.c_str());
	return 0;
}

// --------------------------------------------------------------
// 退出游戏功能 (System.exit)
// --------------------------------------------------------------
static int lua_system_exit(lua_State* L) {
	int exit_code = luaL_optinteger(L, 1, 0);
	LOGI("Lua requested exit with code: %d", exit_code);
	_exit(exit_code);
	return 0;
}

// --------------------------------------------------------------
// 全局状态变量
// --------------------------------------------------------------
static bool g_initialized = false;
static int g_surfaceWidth = 0, g_surfaceHeight = 0;
static int g_windowWidth = 0, g_windowHeight = 0;
static ANativeWindow* g_nativeWindow = nullptr;

// Lua 状态机
static lua_State* g_luaState = nullptr;

// 窗口尺寸（可调整）
static ImVec2 g_windowSize = ImVec2(400, 400);
static const ImVec2 g_minWindowSize = ImVec2(200, 150);
static const ImVec2 g_maxWindowSize = ImVec2(800, 800);

// 原始函数指针
typedef EGLBoolean (*T_eglSwapBuffers)(EGLDisplay, EGLSurface);
typedef bool (*MotionEvent_copyFrom)(void*, void*, bool);
typedef int32_t (*ANativeWindow_GetSize)(ANativeWindow*);

static T_eglSwapBuffers orig_eglSwapBuffers = nullptr;
static MotionEvent_copyFrom orig_MotionEventCopy = nullptr;
static ANativeWindow_GetSize orig_GetWidth = nullptr;
static ANativeWindow_GetSize orig_GetHeight = nullptr;

// --------------------------------------------------------------
// Lua 辅助函数：加载并执行外部脚本
// --------------------------------------------------------------
static bool LoadAndRunLuaScript(const char* path) {
	if (!g_luaState) return false;
	std::ifstream file(path);
	if (!file.is_open()) {
		LOGD("Cannot open Lua script: %s", path);
		return false;
	}
	std::stringstream buffer;
	buffer << file.rdbuf();
	std::string content = buffer.str();
	file.close();
	if (luaL_loadstring(g_luaState, content.c_str()) != LUA_OK) {
		LOGE("Lua load error: %s", lua_tostring(g_luaState, -1));
		lua_pop(g_luaState, 1);
		return false;
	}
	if (lua_pcall(g_luaState, 0, LUA_MULTRET, 0) != LUA_OK) {
		LOGE("Lua run error: %s", lua_tostring(g_luaState, -1));
		lua_pop(g_luaState, 1);
		return false;
	}
	LOGI("Lua script executed successfully: %s", path);
	return true;
}

// --------------------------------------------------------------
// 调用 Lua 中的 OnImGui 函数
// --------------------------------------------------------------
static void CallLuaOnImGui() {
	if (!g_luaState) return;
	lua_getglobal(g_luaState, "OnImGui");
	if (lua_isfunction(g_luaState, -1)) {
		if (lua_pcall(g_luaState, 0, 0, 0) != LUA_OK) {
			LOGE("Lua OnImGui error: %s", lua_tostring(g_luaState, -1));
			lua_pop(g_luaState, 1);
		}
	} else {
		lua_pop(g_luaState, 1);
	}
}

// --------------------------------------------------------------
// 输入事件处理
// --------------------------------------------------------------
bool hook_MotionEventCopy(void* thiz, void* other, bool keepHistory) {
	bool ret = orig_MotionEventCopy(thiz, other, keepHistory);
	if (g_initialized) {
		AInputEvent* event = reinterpret_cast<AInputEvent*>(thiz);
		float scaleX = (g_windowWidth != 0) ? 
			(((float)g_windowWidth / g_surfaceWidth)) : 1.0f;
		float scaleY = (g_windowHeight != 0) ? 
			(((float)g_windowHeight / g_surfaceHeight)) : 1.0f;

		ImGui_ImplAndroid_HandleTouchEvent(event, ImVec2(scaleX, scaleY));
	}
	return ret;
}

// --------------------------------------------------------------
// 窗口尺寸Hook
// --------------------------------------------------------------
int32_t hook_GetWidth(ANativeWindow* window) {
	g_nativeWindow = window;
	g_windowWidth = orig_GetWidth(window);
	return g_windowWidth;
}

int32_t hook_GetHeight(ANativeWindow* window) {
	g_windowHeight = orig_GetHeight(window);
	return g_windowHeight;
}

// --------------------------------------------------------------
// EGL交换缓冲Hook
// --------------------------------------------------------------
EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
	eglQuerySurface(dpy, surface, EGL_WIDTH, &g_surfaceWidth);
	eglQuerySurface(dpy, surface, EGL_HEIGHT, &g_surfaceHeight);
	if (!g_initialized && g_surfaceWidth > 0 && g_surfaceHeight > 0) {
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		ImFontConfig font_cfg;
		io.Fonts->AddFontFromMemoryTTF((void *) OPPOSans_H, OPPOSans_H_size, 25.0f, NULL, io.Fonts->GetGlyphRangesChineseFull());
		io.Fonts->AddFontDefault(&font_cfg);
		io.ConfigWindowsResizeFromEdges = true;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		ImGuiStyle& style = ImGui::GetStyle();
		style.ScaleAllSizes(3.2f);
		style.FramePadding = ImVec2(20.0f, 11.0f);
		style.WindowPadding = ImVec2(20.0f, 20.0f);
		style.FrameBorderSize = 1.0f;
		style.WindowBorderSize = 1.0f;
		style.WindowRounding = 10.0f;
		io.ConfigWindowsResizeFromEdges = false;
		ImGui_ImplOpenGL3_Init("#version 300 es");
		g_luaState = luaL_newstate();
		if (g_luaState) {
			luaL_openlibs(g_luaState);
			lua_newtable(g_luaState);
			lua_pushcfunction(g_luaState, lua_print_debug); lua_setfield(g_luaState, -2, "D");
			lua_pushcfunction(g_luaState, lua_print_info);  lua_setfield(g_luaState, -2, "I");
			lua_pushcfunction(g_luaState, lua_print_warn);  lua_setfield(g_luaState, -2, "W");
			lua_pushcfunction(g_luaState, lua_print_error); lua_setfield(g_luaState, -2, "E");
			lua_setglobal(g_luaState, "print");
			// 注册 ImGui 模块
			create_imdrawlist_metatable(g_luaState);
			luaL_requiref(g_luaState, "ImGui", luaopen_ImGui, 1);
			lua_pop(g_luaState, 1);
			// 注册 Mem 模块
			luaL_requiref(g_luaState, "Mem", luaopen_Mem, 1);
			lua_pop(g_luaState, 1);
			// 注册 System 模块（包含 exit 函数）
			lua_newtable(g_luaState);
			lua_pushcfunction(g_luaState, lua_system_exit);
			lua_setfield(g_luaState, -2, "exit");
			lua_setglobal(g_luaState, "System");
			LoadAndRunLuaScript("/storage/emulated/0/Hook.lua");
		} else {
			LOGE("Failed to create Lua state");
		}
		g_initialized = true;
	}
	if (g_initialized) {
		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize = ImVec2(g_surfaceWidth, g_surfaceHeight);
		ImGui::SetNextWindowSizeConstraints(g_minWindowSize, g_maxWindowSize);
		ImGui::SetNextWindowPos(ImVec2(
			(g_surfaceWidth - g_windowSize.x) * 0.5f,
			(g_surfaceHeight - g_windowSize.y) * 0.5f
		), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(g_windowSize, ImGuiCond_FirstUseEver);
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplAndroid_NewFrame(g_surfaceWidth, g_surfaceHeight);
		ImGui::NewFrame();
		CallLuaOnImGui();
		ImGui::EndFrame();
		glViewport(0, 0, g_surfaceWidth, g_surfaceHeight);
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}
	return orig_eglSwapBuffers(dpy, surface);
}
// --------------------------------------------------------------
// Hook初始化
// --------------------------------------------------------------
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
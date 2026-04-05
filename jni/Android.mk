LOCAL_PATH := $(call my-dir)

# 预构建 Dobby 库
include $(CLEAR_VARS)
LOCAL_MODULE := libdobby
LOCAL_SRC_FILES := Dobby/$(TARGET_ARCH_ABI)/libdobby.a
include $(PREBUILT_STATIC_LIBRARY)

# 主注入库
include $(CLEAR_VARS)
LOCAL_MODULE := Hook
LOCAL_ARM_MODE := arm

# 源文件
LOCAL_SRC_FILES := \
    hook.cpp \
    lua_imgui.cpp \
    lua_mem.cpp \
    imGui/imGui.cpp \
    imGui/imGui_draw.cpp \
    imGui/imGui_tables.cpp \
    imGui/imGui_widgets.cpp \
    imGui/imGui_impl_opengl3.cpp \
    imGui/imGui_impl_android.cpp \
    imGui/imGui_demo.cpp \
    xdl/xdl.c \
    xdl/xdl_iterate.c \
    xdl/xdl_linker.c \
    xdl/xdl_lzma.c \
    xdl/xdl_util.c

LUA_FILES := $(wildcard $(LOCAL_PATH)/lua/*.c)
LUA_FILES := $(filter-out %/lua.c %/luac.c, $(LUA_FILES))
LOCAL_SRC_FILES += $(LUA_FILES:$(LOCAL_PATH)/%=%)

# 头文件搜索路径
LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/imGui \
    $(LOCAL_PATH)/xdl/include \
    $(LOCAL_PATH)/includes \
    $(LOCAL_PATH)/lua

# 编译标志
LOCAL_CFLAGS := -DIMGUI_IMPL_OPENGL_ES3 -fvisibility=hidden
LOCAL_CPPFLAGS := -std=c++17

# 链接库
LOCAL_LDLIBS := -lGLESv3 -lEGL -llog -landroid -lm
LOCAL_STATIC_LIBRARIES := libdobby

include $(BUILD_SHARED_LIBRARY)
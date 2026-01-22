LOCAL_PATH := $(call my-dir)

# 预构建 Dobby 库
include $(CLEAR_VARS)
LOCAL_MODULE := libdobby
LOCAL_SRC_FILES := Dobby/$(TARGET_ARCH_ABI)/libdobby.a
include $(PREBUILT_STATIC_LIBRARY)

LOCAL_CFLAGS := -w -s -Wno-error=format-security -fvisibility=default -fpermissive -fexceptions
LOCAL_CPPFLAGS := -w -s -Wno-error=format-security -fvisibility=default -Werror -std=c++17
LOCAL_CPPFLAGS += -Wno-error=c++17-narrowing -fpermissive -Wall -fexceptions
# 主注入库
include $(CLEAR_VARS)
LOCAL_MODULE := Hook
LOCAL_ARM_MODE := arm
# 源文件
LOCAL_SRC_FILES := \
    hook.cpp \
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
    xdl/xdl_util.c \
	Xhook/xhook.c \
    Xhook/xh_core.c \
    Xhook/xh_elf.c \
    Xhook/xh_jni.c \
    Xhook/xh_log.c \
    Xhook/xh_util.c \
    Xhook/xh_version.c \
    And64InlineHook/And64InlineHook.cpp \


LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/imGui \
    $(LOCAL_PATH)/xdl/include \
    $(LOCAL_PATH)/includes \
# 编译标志
LOCAL_CFLAGS := -DIMGUI_IMPL_OPENGL_ES3 -fvisibility=hidden
LOCAL_CPPFLAGS := -std=c++17

# 链接库
LOCAL_LDLIBS := -lGLESv3 -lEGL -llog -landroid
LOCAL_STATIC_LIBRARIES := libdobby

# 构建共享库
include $(BUILD_SHARED_LIBRARY)

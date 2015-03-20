LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

SDL_PATH := ../SDL2-2.0.3
SDL_TTF_PATH := ../SDL2_ttf-2.0.12
SDL_MIXER_PATH := ../SDL2_mixer-2.0.0
SDL_GFX_PATH := ../../../sdl2-gfx

LOCAL_CFLAGS += -DANDROID -fsigned-char

LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SDL_PATH)/include \
                    $(LOCAL_PATH)/$(SDL_TTF_PATH) \
                    $(LOCAL_PATH)/$(SDL_MIXER_PATH) \
                    $(LOCAL_PATH)/$(SDL_GFX_PATH)/include

LOCAL_SRC_FILES := $(SDL_PATH)/src/main/android/SDL_android_main.c \
                   entropy.c util.c \
                   SDL2_framerate.c SDL2_gfxPrimitives.c SDL2_imageFilter.c SDL2_rotozoom.c

LOCAL_SHARED_LIBRARIES := SDL2 SDL2_ttf SDL2_mixer 

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -llog

include $(BUILD_SHARED_LIBRARY)
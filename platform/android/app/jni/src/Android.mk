LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

SDL_PATH := ../SDL

MEG4_APP := $(LOCAL_PATH)/../../../../sdl

MEG4_LIB := $(LOCAL_PATH)/../../../../../src

LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SDL_PATH)/include $(MEG4_APP) $(MEG4_LIB)

# Add your application source files here...
LOCAL_SRC_FILES := $(MEG4_APP)/data.c $(MEG4_APP)/main.c \
  $(MEG4_LIB)/comp_asm.c \
  $(MEG4_LIB)/comp_bas.c \
  $(MEG4_LIB)/comp.c \
  $(MEG4_LIB)/comp_c.c \
  $(MEG4_LIB)/comp_lua.c \
  $(MEG4_LIB)/cons.c \
  $(MEG4_LIB)/cpu.c \
  $(MEG4_LIB)/data.c \
  $(MEG4_LIB)/dsp.c \
  $(MEG4_LIB)/gpu.c \
  $(MEG4_LIB)/inp.c \
  $(MEG4_LIB)/math.c \
  $(MEG4_LIB)/meg4.c \
  $(MEG4_LIB)/mem.c \
  $(MEG4_LIB)/editors/code.c \
  $(MEG4_LIB)/editors/debug.c \
  $(MEG4_LIB)/editors/font.c \
  $(MEG4_LIB)/editors/formats.c \
  $(MEG4_LIB)/editors/help.c \
  $(MEG4_LIB)/editors/hl.c \
  $(MEG4_LIB)/editors/load.c \
  $(MEG4_LIB)/editors/map.c \
  $(MEG4_LIB)/editors/menu.c \
  $(MEG4_LIB)/editors/music.c \
  $(MEG4_LIB)/editors/overlay.c \
  $(MEG4_LIB)/editors/save.c \
  $(MEG4_LIB)/editors/sound.c \
  $(MEG4_LIB)/editors/sprite.c \
  $(MEG4_LIB)/editors/textinp.c \
  $(MEG4_LIB)/editors/toolbox.c \
  $(MEG4_LIB)/editors/visual.c \
  $(MEG4_LIB)/editors/zip.c \
  $(MEG4_LIB)/lua/lapi.c \
  $(MEG4_LIB)/lua/lauxlib.c \
  $(MEG4_LIB)/lua/lbaselib.c \
  $(MEG4_LIB)/lua/lcode.c \
  $(MEG4_LIB)/lua/lcorolib.c \
  $(MEG4_LIB)/lua/lctype.c \
  $(MEG4_LIB)/lua/ldblib.c \
  $(MEG4_LIB)/lua/ldebug.c \
  $(MEG4_LIB)/lua/ldo.c \
  $(MEG4_LIB)/lua/ldump.c \
  $(MEG4_LIB)/lua/lfunc.c \
  $(MEG4_LIB)/lua/lgc.c \
  $(MEG4_LIB)/lua/linit.c \
  $(MEG4_LIB)/lua/llex.c \
  $(MEG4_LIB)/lua/lmathlib.c \
  $(MEG4_LIB)/lua/lmem.c \
  $(MEG4_LIB)/lua/lobject.c \
  $(MEG4_LIB)/lua/lopcodes.c \
  $(MEG4_LIB)/lua/lparser.c \
  $(MEG4_LIB)/lua/lstate.c \
  $(MEG4_LIB)/lua/lstring.c \
  $(MEG4_LIB)/lua/lstrlib.c \
  $(MEG4_LIB)/lua/ltable.c \
  $(MEG4_LIB)/lua/ltablib.c \
  $(MEG4_LIB)/lua/ltm.c \
  $(MEG4_LIB)/lua/lundump.c \
  $(MEG4_LIB)/lua/lutf8lib.c \
  $(MEG4_LIB)/lua/lvm.c \
  $(MEG4_LIB)/lua/lzio.c

LOCAL_SHARED_LIBRARIES := SDL3

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -lOpenSLES -llog -landroid

include $(BUILD_SHARED_LIBRARY)

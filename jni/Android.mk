#HAVE_OPENSSL := true

ifneq ($(TARGET_SIMULATOR),true)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	gogoc-jni.c

GOGOC_DIR := $(LOCAL_PATH)/../native/gogoc-1_2-RELEASE

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH) \
	$(GOGOC_DIR) \
	$(GOGOC_DIR)/gogoc-pal/defs \
	$(GOGOC_DIR)/gogoc-pal/platform/unix-common/inc \
	$(GOGOC_DIR)/gogoc-pal/platform/common/inc \
	$(GOGOC_DIR)/gogoc-tsp/platform/android \
	$(GOGOC_DIR)/gogoc-tsp/include

LOCAL_CFLAGS :=

LOCAL_LDFLAGS += -llog

LOCAL_MODULE := libgogocjni

LOCAL_STATIC_LIBRARIES += libgogoc

include $(BUILD_SHARED_LIBRARY)

include $(LOCAL_PATH)/../native/Android.mk

endif

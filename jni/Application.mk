# VpnService require 4.0 (api 14)
APP_PLATFORM=android-14
# TODO add mips
APP_ABI := armeabi x86
APP_PROJECT_PATH := $(call my-dir)/..
APP_BUILD_SCRIPT := $(APP_PROJECT_PATH)/jni/Android.mk
APP_MODULES      := libgogocjni

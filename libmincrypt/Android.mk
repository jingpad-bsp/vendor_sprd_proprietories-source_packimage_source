# Copyright 2008 The Android Open Source Project
#
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libmincrypt
LOCAL_SRC_FILES := dsa_sig.c p256.c p256_ec.c p256_ecdsa.c rsa.c sha.c sha256.c
LOCAL_CFLAGS := -Wall -Werror
# include headers mincrypt
LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/../include
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libmincrypt
LOCAL_SRC_FILES := dsa_sig.c p256.c p256_ec.c p256_ecdsa.c rsa.c sha.c sha256.c
LOCAL_CFLAGS := -Wall -Werror
# include headers mincrypt
LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/../include
include $(BUILD_HOST_STATIC_LIBRARY)

# Disable is for AndroidO bring-up dumpkey moved to bootable/recovery/tools/dumpkey already
#include $(LOCAL_PATH)/tools/Android.mk \
        $(LOCAL_PATH)/test/Android.mk

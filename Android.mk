LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := oxygine-freetype_static
LOCAL_MODULE_FILENAME := liboxygine-freetype

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../oxygine-framework/oxygine/src/ \
					$(LOCAL_PATH)/../SDL/include 
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../glm/

LOCAL_SRC_FILES :=  src/ResFontFT.cpp


LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/src/

include $(BUILD_STATIC_LIBRARY)
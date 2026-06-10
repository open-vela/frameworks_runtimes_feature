#
# Copyright (C) 2025 Xiaomi Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

include $(APPDIR)/Make.defs

CXXEXT     := .cpp
CXXFLAGS   += -std=c++17

# workaround for gcc-13 warning
GCC_VERSION := $(shell gcc -dumpversion)
ifeq ($(shell expr $(GCC_VERSION) \>= 13), 1)
	CFLAGS += --param=min-pagesize=0
	CXXFLAGS += --param=min-pagesize=0
endif

ifeq ($(CONFIG_FEATURE_FRAMEWORK),y)

# set some include path
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/runtimes/feature/modules/
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/runtimes/feature/modules/
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/runtimes/feature/modules/generated
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/runtimes/feature/modules/generated
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/runtimes/feature/modules/generated/jidl
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/runtimes/feature/modules/generated/jidl
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/runtimes/feature/modules/generated/aidl
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/runtimes/feature/modules/generated/aidl
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/runtimes/quickapp/src
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/runtimes/quickapp/src
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/runtimes/quickapp/src/framework
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/runtimes/quickapp/src/framework
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/external/rapidjson/rapidjson/include
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/runtimes/feature/include
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/runtimes/feature/src
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/runtimes/feature/src/backend/qjs
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/runtimes/quickapp/src/framework/dom-protobuf
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/runtimes/ash/include
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/runtimes/quickapp/inspector/include
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/runtimes/quickapp/shell/miwear
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/runtimes/quickapp/shell/vapp

ifeq ($(CONFIG_QUICKAPP_VAPP_XMS), y)
	CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/runtimes/quickapp/src/framework
	CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/runtimes/quickapp/src/jse
	CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/base/am/include/app
endif

# set some common srcs
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/modules/app_path.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/modules/net_utils.cpp

ifeq ($(findstring y,$(CONFIG_FEATURE_SYSTEM_CRYPTO)$(CONFIG_FEATURE_SYSTEM_CIPHER)), y)
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/modules/crypto_native.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/modules/crypto_utils.cpp
endif

# collect external features
ifeq ($(CONFIG_ACCOUNT_FEATURE),y)
	FEATURELIST += service_miaccount
endif

ifeq ($(CONFIG_MIJIA_CAMERA_CLIENT),y)
	FEATURELIST += system_internal_micamera
endif

ifeq ($(CONFIG_MEDIA),y)
	CFEATURELIST += system_volume
	CFEATURELIST += system_audio
	CFEATURELIST += system_media_session
	CFEATURELIST += system_record
endif

ifeq ($(CONFIG_QUICKAPP_FOLME_ANIMENGINE_ADAPTER),y)
	FEATURELIST += system_folme
endif

ifeq ($(CONFIG_BLUETOOTH_FEATURE), y)
	CFEATURELIST += system_bluetooth
	CFEATURELIST += system_bluetooth_bt

	ifeq ($(CONFIG_BLUETOOTH_A2DP_SINK), y)
		CFEATURELIST += system_bluetooth_bt_a2dpsink
	endif

	ifeq ($(CONFIG_BLUETOOTH_AVRCP_CONTROL), y)
		CFEATURELIST += system_bluetooth_bt_avrcpcontrol
	endif
else ifeq ($(CONFIG_BLUETOOTH_FEATURE_ASYNC), y)
	CFEATURELIST += system_bluetooth
	CFEATURELIST += system_bluetooth_ble
else
endif

ifeq ($(CONFIG_FEATURE_SYSTEM_APP), y)
	FEATURELIST += system_app
endif

ifeq ($(CONFIG_FEATURE_SYSTEM_ROUTER),y)
	FEATURELIST += system_router
endif

ifeq ($(CONFIG_NETWORK_DOWNLOADTASK_FEATURE),y)
	FEATURELIST += system_downloadtask
endif

ifeq ($(CONFIG_NETWORK_UPLOADTASK_FEATURE),y)
	FEATURELIST += system_uploadtask
endif

ifeq ($(CONFIG_MIPLAY_QAPP),y)
	FEATURELIST += service_miplay
endif

ifneq ($(CONFIG_LYRA_NEW_FEATURE), )
	ifneq ($(CONFIG_LYRA_NEW_FEATURE_TRANSFER), )
		FEATURELIST += system_internal_hyperchannel
	endif

	ifneq ($(CONFIG_LYRA_NEW_FEATURE_MESSAGE_CENTER), )
		FEATURELIST += system_internal_messagecenter
	endif

	ifneq ($(CONFIG_LYRA_NEW_FEATURE_NETWORKING), )
		FEATURELIST += system_internal_networking
	endif

endif

ifeq ($(CONFIG_SERVICE_AGENT_CLIENT),y)
	FEATURE_IGNORE_JIDL = 'utils'
	FEATURELIST += $(shell find $(APPDIR)/vendor/xiaomi/vela/service_agent/feature/jidl/ \
		-name '*.jidl' | grep -Ev $(FEATURE_IGNORE_JIDL) | xargs awk -F '^module|@' '{gsub(/\./, "_", $$2); print $$2}' | grep -v '^$$')
endif

ifeq ($(CONFIG_FEATURE_TEST_FEATURES), y)
	FEATURELIST += array_null
	FEATURELIST += callback_test
	FEATURELIST += const_object_test
	FEATURELIST += event_test
	FEATURELIST += interface_test
	FEATURELIST += jsonobject_test
	FEATURELIST += permissions_test
	FEATURELIST += promise_callback
	FEATURELIST += simple_test
endif

ifeq ($(CONFIG_FEATURE_TEST_JSFILE), y)
ifeq ($(CONFIG_FEATURE_UNIT_TEST), y)
	FEATURELIST += struct_test
	FEATURELIST += any_test
	FEATURELIST += function_test
endif
endif

ifeq ($(CONFIG_FEATURE_TEST_JSFILE), y)
	FEATURELIST += feat_test
endif

# add internal features
JIDL_C_PATH :=
JIDL_PATH :=
OUT_PATH :=
TEST_PATH :=

ifeq ($(CONFIG_FEATURE_SYSTEM_SENSOR), y)
	CXXSRCS 	:= $(CXXSRCS) $(APPDIR)/frameworks/runtimes/feature/modules/sensor_impl.cpp
	JIDL_PATH 	:= $(JIDL_PATH) $(APPDIR)/frameworks/runtimes/feature/modules/jidl/sensor.jidl
	OUT_PATH    += $(APPDIR)/frameworks/runtimes/feature/modules/generated/jidl
	FEATURELIST += system_sensor
endif

ifeq ($(CONFIG_FEATURE_SYSTEM_FILE),y)
	CXXSRCS 	:= $(CXXSRCS) $(APPDIR)/frameworks/runtimes/feature/modules/file_impl.cpp
	JIDL_PATH 	:= $(JIDL_PATH) $(APPDIR)/frameworks/runtimes/feature/modules/jidl/file.jidl
	OUT_PATH    += $(APPDIR)/frameworks/runtimes/feature/modules/generated/jidl
	FEATURELIST += system_file
endif

ifeq (0,y) # TODO reconstruct system_media
	CXXSRCS 	:= $(CXXSRCS) $(APPDIR)/frameworks/runtimes/feature/modules/media_impl.cpp
	CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/graphics/animengine/include
	CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/runtimes/quickapp/src/gui
	CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/runtimes/quickapp/src/framework
	JIDL_PATH 	:= $(JIDL_PATH) $(APPDIR)/frameworks/runtimes/feature/modules/jidl/media.jidl
	OUT_PATH    += $(APPDIR)/frameworks/runtimes/feature/modules/generated/jidl
	FEATURELIST += system_media
endif

ifeq ($(CONFIG_FEATURE_SYSTEM_REQUEST), y)
	CXXFLAGS 	+= ${INCDIR_PREFIX}$(APPDIR)/system/libuv/ext/include
	CFLAGS 		+= ${INCDIR_PREFIX}$(APPDIR)/system/libuv/ext/include
	JIDL_PATH 	:= $(JIDL_PATH) $(APPDIR)/frameworks/runtimes/feature/modules/jidl/request.jidl
	CXXSRCS 	:= $(CXXSRCS) $(APPDIR)/frameworks/runtimes/feature/modules/request_impl.cpp
	OUT_PATH    += $(APPDIR)/frameworks/runtimes/feature/modules/generated/jidl
	FEATURELIST += system_request
endif

ifeq ($(CONFIG_FEATURE_SYSTEM_ZIP),y)
	CXXFLAGS 	+= ${INCDIR_PREFIX}$(APPDIR)/system/zlib/zlib/contrib
	CXXFLAGS 	+= ${INCDIR_PREFIX}$(APPDIR)/system/zlib/zlib
	CFLAGS 		+= ${INCDIR_PREFIX}$(APPDIR)/system/zlib/zlib/contrib/minizip
	CFLAGS 		+= ${INCDIR_PREFIX}$(APPDIR)/system/zlib/zlib
	JIDL_PATH 	:= $(JIDL_PATH) $(APPDIR)/frameworks/runtimes/feature/modules/jidl/zip.jidl
	CXXSRCS 	:= $(CXXSRCS) $(APPDIR)/frameworks/runtimes/feature/modules/zip_impl.cpp
	OUT_PATH    += $(APPDIR)/frameworks/runtimes/feature/modules/generated/jidl
	FEATURELIST += system_zip
endif

ifeq ($(CONFIG_FEATURE_SYSTEM_ZLIB),y)
	JIDL_PATH 	:= $(JIDL_PATH) $(APPDIR)/frameworks/runtimes/feature/modules/jidl/system_zlib.jidl
	CXXSRCS 	:= $(CXXSRCS) $(APPDIR)/frameworks/runtimes/feature/modules/system_zlib_impl.cpp
	OUT_PATH    += $(APPDIR)/frameworks/runtimes/feature/modules/generated/jidl
	FEATURELIST += system_zlib
endif


ifeq ($(CONFIG_FEATURE_SYSTEM_CRYPTO),y)
	CXXSRCS 	:= $(CXXSRCS) $(APPDIR)/frameworks/runtimes/feature/modules/crypto_impl.cpp
	JIDL_PATH 	:= $(JIDL_PATH) $(APPDIR)/frameworks/runtimes/feature/modules/jidl/crypto.jidl
	OUT_PATH    += $(APPDIR)/frameworks/runtimes/feature/modules/generated/jidl
	FEATURELIST += system_crypto
endif

ifeq ($(CONFIG_FEATURE_SYSTEM_MQTTMESSAGE),y)
	CXXSRCS 	:= $(CXXSRCS) $(APPDIR)/frameworks/runtimes/feature/modules/mqttmessage_impl.cpp
	CSRCS 	:= $(CSRCS) $(APPDIR)/frameworks/runtimes/feature/modules/mqtt_message.pb-c.c
	JIDL_PATH 	:= $(JIDL_PATH) $(APPDIR)/frameworks/runtimes/feature/modules/jidl/mqttmessage.jidl
	OUT_PATH    += $(APPDIR)/frameworks/runtimes/feature/modules/generated/jidl
	FEATURELIST += system_mqttmessage
endif

ifeq ($(CONFIG_FEATURE_SYSTEM_CIPHER),y)
	CXXSRCS 	:= $(CXXSRCS) $(APPDIR)/frameworks/runtimes/feature/modules/cipher_impl.cpp
	JIDL_PATH 	:= $(JIDL_PATH) $(APPDIR)/frameworks/runtimes/feature/modules/jidl/cipher.jidl
	OUT_PATH    += $(APPDIR)/frameworks/runtimes/feature/modules/generated/jidl
	FEATURELIST += system_cipher
endif

ifeq ($(CONFIG_FEATURE_SYSTEM_VIBRATOR), y)
	CXXSRCS     := $(CXXSRCS) $(APPDIR)/frameworks/runtimes/feature/modules/vibrator_impl.cpp
	JIDL_PATH   := $(JIDL_PATH) $(APPDIR)/frameworks/runtimes/feature/modules/jidl/vibrator.jidl
	OUT_PATH    += $(APPDIR)/frameworks/runtimes/feature/modules/generated/jidl
	FEATURELIST += system_vibrator
endif

ifeq ($(CONFIG_QUICKAPP_TEST_FRAMEWORK), y)
	CXXSRCS     := $(CXXSRCS) $(APPDIR)/frameworks/runtimes/feature/modules/internal_test_impl.cpp
	JIDL_PATH 	:= $(JIDL_PATH) $(APPDIR)/frameworks/runtimes/feature/modules/jidl/internal_test.jidl
	OUT_PATH 	+= $(APPDIR)/frameworks/runtimes/feature/modules/generated/jidl/
	FEATURELIST += system_internal_test
endif

ifeq ($(CONFIG_GRAPHICS_INPUT_TOOL),y)
	CXXSRCS     := $(CXXSRCS) $(APPDIR)/frameworks/runtimes/feature/modules/input_impl.cpp
	JIDL_PATH   := $(JIDL_PATH) $(APPDIR)/frameworks/runtimes/feature/modules/jidl/input.jidl
	OUT_PATH    += $(APPDIR)/frameworks/runtimes/feature/modules/generated/jidl/
	FEATURELIST += system_test_input
endif

ifeq ($(CONFIG_FEATURE_SYSTEM_INTERNAL_POWER), y)
	CXXSRCS 	:= $(CXXSRCS) $(APPDIR)/frameworks/runtimes/feature/modules/power_impl.cpp
	JIDL_PATH 	:= $(JIDL_PATH) $(APPDIR)/frameworks/runtimes/feature/modules/jidl/power.jidl
	OUT_PATH    += $(APPDIR)/frameworks/runtimes/feature/modules/generated/jidl
	FEATURELIST += system_internal_power
endif

ifeq ($(CONFIG_FEATURE_SYSTEM_INTERNAL_ACTIVITY), y)
	CXXSRCS 	:= $(CXXSRCS) $(APPDIR)/frameworks/runtimes/feature/modules/activity_feature_impl.cpp
	JIDL_PATH 	:= $(JIDL_PATH) $(APPDIR)/frameworks/runtimes/feature/modules/jidl/activity.jidl
	OUT_PATH    += $(APPDIR)/frameworks/runtimes/feature/modules/generated/jidl
	FEATURELIST += system_internal_activity
endif

ifeq ($(CONFIG_FEATURE_SYSTEM_PACKAGEMANAGER), y)
	CXXSRCS 	:= $(CXXSRCS) $(APPDIR)/frameworks/runtimes/feature/modules/packagemanager_impl.cpp
	JIDL_PATH 	:= $(JIDL_PATH) $(APPDIR)/frameworks/runtimes/feature/modules/jidl/packagemanager.jidl
	OUT_PATH    += $(APPDIR)/frameworks/runtimes/feature/modules/generated/jidl
	FEATURELIST += system_packagemanager
endif

ifeq ($(CONFIG_FEATURE_SYSTEM_FETCH),y)
	CXXSRCS 	:= $(CXXSRCS) $(APPDIR)/frameworks/runtimes/feature/modules/fetch_impl.cpp
	JIDL_PATH 	:= $(JIDL_PATH) $(APPDIR)/frameworks/runtimes/feature/modules/jidl/fetch.jidl
	OUT_PATH    += $(APPDIR)/frameworks/runtimes/feature/modules/generated/jidl
	FEATURELIST += system_fetch
endif

ifeq ($(CONFIG_FEATURE_SYSTEM_PROMPT), y)
	CXXSRCS 	:= $(CXXSRCS) $(APPDIR)/frameworks/runtimes/feature/modules/prompt_impl.cpp
	CXXSRCS 	+= $(wildcard $(APPDIR)/frameworks/runtimes/feature/modules/prompt/*.cpp)
	JIDL_PATH 	:= $(JIDL_PATH) $(APPDIR)/frameworks/runtimes/feature/modules/jidl/prompt.jidl
	OUT_PATH    += $(APPDIR)/frameworks/runtimes/feature/modules/generated/jidl
	FEATURELIST += system_prompt
endif

ifeq ($(CONFIG_FEATURE_SYSTEM_MESSAGECHANNEL), y)
	AIDLSRCS_PATH 	:= $(APPDIR)/frameworks/runtimes/feature/modules/aidl
	AIDLSRCS 		+= $(shell find $(AIDLSRCS_PATH) -name *.aidl)
	AIDLFLAGS 		= --lang=cpp -I$(AIDLSRCS_PATH) -h$(APPDIR)/frameworks/runtimes/feature/modules/generated -o$(APPDIR)/frameworks/runtimes/feature/modules/generated
	CXXSRCS 		:= $(CXXSRCS) $(subst modules/aidl/,modules/generated/,$(patsubst %.aidl,%$(CXXEXT),$(AIDLSRCS)))
	CXXSRCS 		:= $(CXXSRCS) $(APPDIR)/frameworks/runtimes/feature/modules/message_transport.cpp
	CXXSRCS 		:= $(CXXSRCS) $(APPDIR)/frameworks/runtimes/feature/modules/message_channel_impl.cpp
	JIDL_PATH 		:= $(JIDL_PATH) $(APPDIR)/frameworks/runtimes/feature/modules/jidl/message_channel.jidl
	OUT_PATH    	+= $(APPDIR)/frameworks/runtimes/feature/modules/generated/jidl
	FEATURELIST 	+= system_messageChannel
endif

ifeq ($(CONFIG_FEATURE_SYSTEM_EXCHANGE),y)
	CXXSRCS 	:= $(CXXSRCS) $(APPDIR)/frameworks/runtimes/feature/modules/exchange_impl.cpp
	JIDL_PATH 	:= $(JIDL_PATH) $(APPDIR)/frameworks/runtimes/feature/modules/jidl/exchange.jidl
	OUT_PATH    += $(APPDIR)/frameworks/runtimes/feature/modules/generated/jidl
	FEATURELIST += system_exchange
endif

ifeq ($(CONFIG_RUST_FEATURE_SYSTEM_EXCHANGE),y)
	JIDL_C_PATH 	:= $(JIDL_C_PATH) $(APPDIR)/frameworks/runtimes/feature/rust/modules/system_exchange/jidl/system_exchange.jidl
	OUT_PATH    += $(APPDIR)/frameworks/runtimes/feature/modules/generated/jidl
	CFEATURELIST += system_exchange
endif

ifeq ($(CONFIG_FEATURE_SYSTEM_NETWORK),y)
	JIDL_C_PATH 	:= $(JIDL_C_PATH) $(APPDIR)/frameworks/runtimes/feature/rust/modules/system_network/jidl/system_network.jidl
	OUT_PATH    += $(APPDIR)/frameworks/runtimes/feature/modules/generated/jidl
	CFEATURELIST += system_network
endif

ifeq ($(CONFIG_FEATURE_SYSTEM_CONFIGURATION),y)
	CXXSRCS 	:= $(CXXSRCS) $(APPDIR)/frameworks/runtimes/feature/modules/configuration_impl.cpp
	JIDL_PATH 	:= $(JIDL_PATH) $(APPDIR)/frameworks/runtimes/feature/modules/jidl/configuration.jidl
	OUT_PATH    += $(APPDIR)/frameworks/runtimes/feature/modules/generated/jidl
	FEATURELIST += system_configuration
endif

ifeq ($(CONFIG_FEATURE_SYSTEM_DEVICE),y)
	CXXSRCS 	:= $(CXXSRCS) $(APPDIR)/frameworks/runtimes/feature/modules/device_impl.cpp
	JIDL_PATH 	:= $(JIDL_PATH) $(APPDIR)/frameworks/runtimes/feature/modules/jidl/device.jidl
	OUT_PATH    += $(APPDIR)/frameworks/runtimes/feature/modules/generated/jidl
	FEATURELIST += system_device
endif

ifneq ($(CONFIG_FEATURE_SYSTEM_BRIGHTNESS), )
	CXXSRCS     := $(CXXSRCS) $(APPDIR)/frameworks/runtimes/feature/modules/brightness_impl.cpp
	JIDL_PATH   := $(JIDL_PATH) $(APPDIR)/frameworks/runtimes/feature/modules/jidl/brightness.jidl
	OUT_PATH    += $(APPDIR)/frameworks/runtimes/feature/modules/generated/jidl
	FEATURELIST += system_brightness

	CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/external/android/system/core/libutils/include

    # If the CONFIG_SYSTEM_BRIGHTNESS_SERVICE macro is enabled, add the paths to the binder-related header files.
	ifneq ($(CONFIG_SYSTEM_BRIGHTNESS_SERVICE),)
        CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/runtimes/services/brightness/include
        CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/external/android/frameworks/native/libs/binder/include
    endif
endif

ifeq ($(CONFIG_FEATURE_SYSTEM_EVENT),y)
	CXXSRCS     := $(CXXSRCS) $(APPDIR)/frameworks/runtimes/feature/modules/event_impl.cpp
	CXXSRCS     := $(CXXSRCS) $(APPDIR)/frameworks/runtimes/feature/modules/event/topic.cpp
	CXXSRCS     := $(CXXSRCS) $(APPDIR)/frameworks/runtimes/feature/modules/event/event_context.cpp
	CXXSRCS     := $(CXXSRCS) $(APPDIR)/frameworks/runtimes/feature/modules/event/topics/user_topic.cpp
	CXXSRCS     := $(CXXSRCS) $(APPDIR)/frameworks/runtimes/feature/modules/event/topics/battery_topic.cpp

	JIDL_PATH   := $(JIDL_PATH) $(APPDIR)/frameworks/runtimes/feature/modules/jidl/event.jidl
	OUT_PATH    += $(APPDIR)/frameworks/runtimes/feature/modules/generated/jidl
	
	CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/topics/include/system/
	CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/topics/include/system/
	CXXFLAGS := $(CXXFLAGS) ${INCDIR_PREFIX}$(APPDIR)/frameworks/runtimes/feature/modules
	CFLAGS := $(CFLAGS) ${INCDIR_PREFIX}$(APPDIR)/frameworks/runtimes/feature/modules
	FEATURELIST += system_event
endif

ifeq ($(CONFIG_FEATURE_SYSTEM_EVENT_TEST),y)
	CXXSRCS     := $(CXXSRCS) $(APPDIR)/frameworks/runtimes/feature/modules/event_test_impl.cpp
	JIDL_PATH   := $(JIDL_PATH) $(APPDIR)/frameworks/runtimes/feature/modules/jidl/event_test.jidl
	OUT_PATH 	+= $(APPDIR)/frameworks/runtimes/feature/modules/generated/jidl
	CXXFLAGS := $(CXXFLAGS) ${INCDIR_PREFIX}$(APPDIR)/frameworks/runtimes/feature/modules
	CFLAGS := $(CFLAGS) ${INCDIR_PREFIX}$(APPDIR)/frameworks/runtimes/feature/modules
	FEATURELIST += system_event_test
endif

ifeq ($(CONFIG_FEATURE_SYSTEM_INTERCONNECT),y)
	CXXSRCS     += $(APPDIR)/frameworks/runtimes/feature/modules/interconnect_impl.cpp
	JIDL_PATH   += $(APPDIR)/frameworks/runtimes/feature/modules/jidl/interconnect.jidl
	OUT_PATH    += $(APPDIR)/frameworks/runtimes/feature/modules/generated/jidl
	FEATURELIST += system_interconnect
endif

ifeq ($(CONFIG_FEATURE_SYSTEM_STORAGE),y)
	CXXFLAGS 	+= ${INCDIR_PREFIX}$(APPDIR)/external/unqlite/unqlite
	CXXSRCS 	:= $(CXXSRCS) $(APPDIR)/frameworks/runtimes/feature/modules/storage_impl.cpp
	JIDL_PATH 	:= $(JIDL_PATH) $(APPDIR)/frameworks/runtimes/feature/modules/jidl/storage.jidl
	OUT_PATH    += $(APPDIR)/frameworks/runtimes/feature/modules/generated/jidl
	FEATURELIST += system_storage
endif

ifeq ($(CONFIG_FEATURE_SERVICE_WECHAT),y)
	CXXSRCS     := $(CXXSRCS) $(APPDIR)/frameworks/runtimes/feature/modules/wechat_impl.cpp
	JIDL_PATH   := $(JIDL_PATH) $(APPDIR)/frameworks/runtimes/feature/modules/jidl/wechat.jidl
	OUT_PATH    += $(APPDIR)/frameworks/runtimes/feature/modules/generated/jidl
	FEATURELIST += service_wechat
endif

ifeq ($(CONFIG_FEATURE_LOCALE),y)
	CXXSRCS 	:= $(CXXSRCS) $(APPDIR)/frameworks/runtimes/feature/modules/locale_impl.cpp
	JIDL_PATH 	:= $(JIDL_PATH) $(APPDIR)/frameworks/runtimes/feature/modules/jidl/feature_locale.jidl
	OUT_PATH 	+= $(APPDIR)/frameworks/runtimes/feature/modules/generated/jidl
	FEATURELIST += locale
endif

ifeq ($(CONFIG_FEATURE_ERROR),y)
	CXXSRCS 	:= $(CXXSRCS) $(APPDIR)/frameworks/runtimes/feature/modules/error_impl.cpp
	JIDL_PATH 	:= $(JIDL_PATH) $(APPDIR)/frameworks/runtimes/feature/modules/jidl/error.jidl
	OUT_PATH 	+= $(APPDIR)/frameworks/runtimes/feature/modules/generated/jidl
	FEATURELIST += Error
endif

ifeq ($(CONFIG_FEATURE_SYSTEM_BATTERY),y)
	CXXSRCS 	:= $(CXXSRCS) $(APPDIR)/frameworks/runtimes/feature/modules/battery_impl.cpp
	JIDL_PATH 	:= $(JIDL_PATH) $(APPDIR)/frameworks/runtimes/feature/modules/jidl/battery.jidl
	OUT_PATH 	+= $(APPDIR)/frameworks/runtimes/feature/modules/generated/jidl
	FEATURELIST += system_battery
endif

ifeq ($(CONFIG_FEATURE_SYSTEM_GEOLOCATION),y)
	CXXSRCS 	:= $(CXXSRCS) $(APPDIR)/frameworks/runtimes/feature/modules/geolocation_impl.cpp
	JIDL_PATH 	:= $(JIDL_PATH) $(APPDIR)/frameworks/runtimes/feature/modules/jidl/geolocation.jidl
	OUT_PATH 	+= $(APPDIR)/frameworks/runtimes/feature/modules/generated/jidl
	FEATURELIST += system_geolocation
endif

ifeq ($(CONFIG_FEATURE_SERVICE_HEALTH),y)
	CXXSRCS 	:= $(CXXSRCS) $(APPDIR)/frameworks/runtimes/feature/modules/health_impl.cpp
	JIDL_PATH 	:= $(JIDL_PATH) $(APPDIR)/frameworks/runtimes/feature/modules/jidl/health.jidl
	OUT_PATH 	+= $(APPDIR)/frameworks/runtimes/feature/modules/generated/jidl
	FEATURELIST += service_health
endif

ifeq ($(CONFIG_FEATURE_INPUT_METHOD_CONTROLLER),y)
        CXXSRCS         := $(CXXSRCS) $(APPDIR)/frameworks/runtimes/feature/modules/imc_impl.cpp
        JIDL_PATH       := $(JIDL_PATH) $(APPDIR)/frameworks/runtimes/feature/modules/jidl/imc.jidl
        OUT_PATH        += $(APPDIR)/frameworks/runtimes/feature/modules/generated/jidl
        FEATURELIST += system_imc
endif


ifeq ($(CONFIG_AI_MODULE),y)
	CXXFLAGS 	+= ${INCDIR_PREFIX}$(APPDIR)/netutils/cjson/cJSON
	CXXFLAGS 	+= ${INCDIR_PREFIX}$(APPDIR)/frameworks/ai/src/asr
	CXXFLAGS 	+= ${INCDIR_PREFIX}$(APPDIR)/frameworks/ai/include
	CXXSRCS     := $(CXXSRCS) $(APPDIR)/frameworks/runtimes/feature/modules/ai/src/ai_speech_impl.cpp
	CXXSRCS     := $(CXXSRCS) $(APPDIR)/frameworks/runtimes/feature/modules/ai/src/asr_impl.cpp
	JIDL_PATH   := $(JIDL_PATH) $(APPDIR)/frameworks/runtimes/feature/modules/ai/jidl/system_ai_speech.jidl
	OUT_PATH    += $(APPDIR)/frameworks/runtimes/feature/modules/generated/jidl
	FEATURELIST += system_ai_speech
endif

# for feature registry
PDATLIST = $(strip $(call RWILDCARD, registry, *.pdat))

CXXSRCS += $(strip $(foreach i, $(shell seq 1 $(words $(JIDL_PATH))),\
	$(eval jidl_path=$(word $(i), $(JIDL_PATH)))\
	$(eval out_path=$(word $(i), $(OUT_PATH)))\
	$(eval file_name=$(strip $(basename $(notdir $(word $(i), $(JIDL_PATH))) .jidl)))\
	$(out_path)/$(file_name).cpp\
))

CSRCS += $(strip $(foreach i, $(shell seq 1 $(words $(JIDL_C_PATH))),\
	$(eval jidl_path=$(word $(i), $(JIDL_C_PATH)))\
	$(eval out_path=$(word $(i), $(OUT_PATH)))\
	$(eval file_name=$(strip $(basename $(notdir $(word $(i), $(JIDL_C_PATH))) .jidl)))\
	$(out_path)/$(file_name).c\
))

CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/modules/ajs_features_registry.cpp

include ${APPDIR}/frameworks/runtimes/feature/modules/Module.mk

ASRCS := $(wildcard $(ASRCS))
CSRCS := $(wildcard $(CSRCS))
CXXSRCS := $(wildcard $(CXXSRCS))
MAINSRC := $(wildcard $(MAINSRC))
ifeq ($(CONFIG_FEATURE_SYSTEM_VELACLAW),y)
    CXXSRCS         := $(CXXSRCS) $(APPDIR)/frameworks/runtimes/feature/modules/velaclaw_impl.cpp
    CXXSRCS         := $(CXXSRCS) $(APPDIR)/frameworks/runtimes/feature/modules/generated/jidl/velaclaw.cpp
    CSRCS           := $(CSRCS) $(APPDIR)/packages/ai_agent/src/quickapp/velaclaw_quickapp_bridge.c
    JIDL_PATH       := $(JIDL_PATH) $(APPDIR)/frameworks/runtimes/feature/modules/jidl/velaclaw.jidl
    OUT_PATH    += $(APPDIR)/frameworks/runtimes/feature/modules/generated/jidl
    CFLAGS          += ${INCDIR_PREFIX}$(APPDIR)/packages/ai_agent/include
    CFLAGS          += ${INCDIR_PREFIX}$(APPDIR)/packages/ai_agent/src/quickapp
    CXXFLAGS        += ${INCDIR_PREFIX}$(APPDIR)/packages/ai_agent/include
    CXXFLAGS        += ${INCDIR_PREFIX}$(APPDIR)/packages/ai_agent/src/quickapp
    FEATURELIST += system_velaclaw
endif

NOEXPORTSRCS = $(ASRCS)$(CSRCS)$(CXXSRCS)$(MAINSRC)

ifneq ($(NOEXPORTSRCS),)
BIN := $(APPDIR)/staging/libfeature_impls.a
endif

clean::
	-rm -rf $(APPDIR)/frameworks/runtimes/feature/modules/generated/jidl/*.h
	-rm -rf $(APPDIR)/frameworks/runtimes/feature/modules/generated/jidl/*.cpp
	-rm -rf $(APPDIR)/frameworks/runtimes/feature/modules/generated/jidl/*.o
	-rm -rf $(APPDIR)/frameworks/runtimes/feature/modules/generated/message_transport/*

distclean::
	-rm -rf $(APPDIR)/frameworks/runtimes/feature/modules/generated/jidl/*.h
	-rm -rf $(APPDIR)/frameworks/runtimes/feature/modules/generated/jidl/*.cpp
	-rm -rf $(APPDIR)/frameworks/runtimes/feature/modules/generated/jidl/*.o
	-rm -rf $(APPDIR)/frameworks/runtimes/feature/modules/registry/*.pdat
	-rm -rf $(APPDIR)/frameworks/runtimes/feature/modules/generated/message_transport/*

clean_context::
	$(call DELFILE, $(PDATLIST))

endif # CONFIG_FEATURE_FRAMEWORK

include $(APPDIR)/Application.mk

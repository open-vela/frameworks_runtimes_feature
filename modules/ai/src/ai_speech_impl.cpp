#include "system_ai_speech.h"

#define TAG "[ai_impl]"
#define AI_LOG_DEBUG(fmt, ...) \
    FEATURE_LOG_DEBUG(TAG fmt, ##__VA_ARGS__)

#define AI_LOG_INFO(fmt, ...) FEATURE_LOG_INFO(TAG fmt, ##__VA_ARGS__)

#define AI_LOG_ERROR(fmt, ...) \
    FEATURE_LOG_ERROR(TAG fmt, ##__VA_ARGS__)

void system_ai_speech_onRegister(const char* feature_name)
{
    AI_LOG_DEBUG("system_ai_speech_onRegister");
}

void system_ai_speech_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    AI_LOG_DEBUG("system_ai_speech_onCreate");
}

void system_ai_speech_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    AI_LOG_DEBUG("system_ai_speech_onRequired");
}

void system_ai_speech_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    AI_LOG_DEBUG("system_ai_speech_onDetached");
}

void system_ai_speech_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    AI_LOG_DEBUG("system_ai_speech_onDestroy");
}

void system_ai_speech_onUnregister(const char* feature_name)
{
    AI_LOG_DEBUG("system_ai_speech_onUnregister");
}
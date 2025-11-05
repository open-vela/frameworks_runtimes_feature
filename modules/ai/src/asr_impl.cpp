#include "ai_asr.h"
#include "ai_asr_internal.h"
#include "ai_util.h"
#include "system_ai_speech.h"

#include <memory>
#include <string>

#define TAG "[asr_impl]"
#define ASR_LOG_DEBUG(fmt, ...) \
    FEATURE_LOG_DEBUG(TAG fmt, ##__VA_ARGS__)

#define ASR_LOG_INFO(fmt, ...) FEATURE_LOG_INFO(TAG fmt, ##__VA_ARGS__)

#define ASR_LOG_ERROR(fmt, ...) \
    FEATURE_LOG_ERROR(TAG fmt, ##__VA_ARGS__)

using namespace ai;

namespace asr {
class Asr {
public:
    enum {
        kCommonError = 200,
        kEngineInternalError = 10006,
        kEngineServiceError = 10007
    };
    struct asr_info {
        const char* engine_type;
        const char* config;
        const char* lang;
        const char** translations;
        int timeout;
        int tran_count;
    };
    Asr(FeatureInterfaceHandle feature, asr_handle_t asr);
    ~Asr();
    void Start();
    void Finish();
    void Cancel();
    static bool CheckLang(const char* lang);
    static std::unique_ptr<asr::Asr> Create(FeatureInterfaceHandle feature, const asr_info& info, bool need_check = true);
    static const int kDefaultTimeout = 1000;
    static const int ASR_VERSION = 1;

private:
    void OnMessage(const char* result, int reformation, char** translations, char** lang);
    void OnError(int code = asr_error_t::asr_error_unknown);
    void OnFinished();
    int ConvertErrorCode(int code, std::string& text);
    static void EventCallback(asr_event_t event, const asr_result_t* result, void* cookie);
    FeatureInterfaceHandle feature_;
    asr_handle_t asr_;
};

Asr::Asr(FeatureInterfaceHandle feature, asr_handle_t asr)
    : feature_(feature)
    , asr_(asr)
{
    ai_asr_set_listener(asr_, &Asr::EventCallback, this);
}

Asr::~Asr()
{
    ai_asr_set_listener(asr_, nullptr, nullptr);
    ai_asr_finish(asr_);
    if (ai_asr_close(asr_))
        assert(0);
}
bool Asr::CheckLang(const char* lang)
{
    static const char* valid_langs[] = { "auto", "zh", "en", "ja" };
    static const int valid_langs_count = sizeof(valid_langs) / sizeof(valid_langs[0]);

    if (lang == nullptr) {
        return true;
    }

    for (int i = 0; i < valid_langs_count; ++i) {
        if (strcmp(lang, valid_langs[i]) == 0) {
            return true;
        }
    }

    return false;
}

void Asr::OnMessage(const char* result, int reformation, char** translations, char** lang)
{
    if (FeatureGetEventCallbackCountByName(feature_, "onMessage") <= 0)
        return;
    system_ai_speech_AsrResult res = { 0 };
    res.text = result;
    res.reformation = reformation;
    int size = 0;
    while (translations && translations[size]) {
        size++;
    }
    if (size) {
        FtArray tran_array = { 0 };
        tran_array._size = size;
        system_ai_speech_AsrTranslationResult* tran_res_ref[size] = { 0 };
        system_ai_speech_AsrTranslationResult tran_res[size] = { 0 };
        for (int i = 0; i < size; i++) {
            tran_res[i].text = translations[i];
            tran_res[i].lang = lang[i];
            tran_res_ref[i] = &tran_res[i];
        }
        tran_array._element = tran_res_ref;
        res.translations = &tran_array;
        FeatureEmitEventByName(feature_, "onMessage", &res);
        return;
    }
    FeatureEmitEventByName(feature_, "onMessage", &res);
}

int Asr::ConvertErrorCode(int code, std::string& text)
{
    switch (code) {
    case asr_error_t::asr_error_failed:
    case asr_error_t::asr_error_media:
    case asr_error_t::asr_error_destroyed:
        text = "engine internal error";
        return kEngineInternalError;
    case asr_error_t::asr_error_network:
        text = "network error";
        return kEngineServiceError;
    default:
        text = "unknown error";
        return kCommonError;
    }
}

void Asr::OnError(int code)
{
    if (FeatureGetEventCallbackCountByName(feature_, "onError") <= 0)
        return;
    std::string text;
    int code_ = ConvertErrorCode(code, text);
    // system_ai_speech_SpeechRecognition_emit_onError(feature_, text.c_str(), code);
    FeatureEmitEventByName(feature_, "onError", text.c_str(), code_);
}

void Asr::OnFinished()
{
    if (FeatureGetEventCallbackCountByName(feature_, "onFinished") <= 0)
        return;
    // system_ai_speech_SpeechRecognition_emit_onFinished(feature_);
    FeatureEmitEventByName(feature_, "onFinished");
}

void Asr::EventCallback(asr_event_t event, const asr_result_t* result, void* cookie)
{
    if (!cookie) {
        ASR_LOG_ERROR("EventCallback: cookie is null");
        return;
    }
    Asr* asr = reinterpret_cast<Asr*>(cookie);
    switch (event) {
    case asr_event_start:
        ASR_LOG_DEBUG("asr_event_start");
        break;
    case asr_event_result:
        if (result && result->data.result) {
            ASR_LOG_INFO("asr_event_result: %s", result->data.result);
            asr->OnMessage(result->data.result, result->data.sentence_end,
                result->data.translation, result->data.lang);
        }
        break;
    case asr_event_complete:
        ASR_LOG_DEBUG("asr_event_complete");
        asr->OnFinished();
        break;
    case asr_event_error:
        if (result) {
            ASR_LOG_DEBUG("asr_event_error: %d", result->error_code);
            asr->OnError(result->error_code);
        }
        break;
    case asr_event_cancel:
        ASR_LOG_INFO("asr_event_cancel");
        break;
    case asr_event_closed:
        ASR_LOG_INFO("asr_event_closed");
        break;
    case asr_event_loss_focus:
        ASR_LOG_INFO("asr_event_loss_focus");
        asr->OnError(asr_error_t::asr_error_media);
        break;
    default:
        ASR_LOG_DEBUG("asr_event_unknown");
        break;
    }
}

void Asr::Start()
{
    ai_asr_start(asr_, nullptr);
}

void Asr::Finish()
{
    ai_asr_finish(asr_);
}

void Asr::Cancel()
{
    ai_asr_cancel(asr_);
}

std::unique_ptr<asr::Asr> Asr::Create(FeatureInterfaceHandle feature, const asr_info& info, bool need_check)
{
    asr_init_params_t init_param { 0 };
    ai_auth_t ai_auth { 0 };
    auto loop = FeatureGetUVLoop(FeatureGetManagerHandleFromInstance(feature));
    init_param.version = ASR_VERSION;
    init_param.loop = loop;
    init_param.language = info.lang;
    init_param.slience_timeout = info.timeout;
    const char* translations_buf[info.tran_count + 1] = { 0 };
    if (info.tran_count > 0)
        memcpy((void*)translations_buf, info.translations, info.tran_count * sizeof(char*));
    init_param.translations = translations_buf;
    asr_handle_t asr_handle = nullptr;
    if (!need_check) {
        asr_handle = ai_asr_create_engine(&init_param);
    } else {
        cJSON* root = cJSON_Parse(info.config);
        if (!root) {
            ASR_LOG_ERROR("parse config json failed");
            return nullptr;
        }

        if (!ai::setupAuthFromConfig(root, info.engine_type, ai_auth)) {
            cJSON_Delete(root);
            return nullptr;
        }
        asr_handle = ai_asr_create_engine_with_auth(&init_param, &ai_auth);
        cJSON_Delete(root);
        free((void*)ai_auth.auth);
    }
    if (!asr_handle) {
        ASR_LOG_ERROR("ai_asr_create_engine_with_auth failed");
        return nullptr;
    }
    return std::make_unique<Asr>(feature, asr_handle);
}
} // namespace asr

FeatureInterfaceHandle system_ai_speech_wrap_createAsr(FeatureInstanceHandle handle, AppendData adata, system_ai_speech_AsrParams* para)
{
    if (!para) {
        ASR_LOG_ERROR("para is null");
        FeatureThrowError(handle, "param error");
        return nullptr;
    }
    if (!para->auth || !para->auth->config) {
        ASR_LOG_ERROR("auth or config is null");
        FeatureThrowError(handle, "auth error");
        return nullptr;
    }

    if (para->config) {
        if (para->config->endVadTime < 0) {
            ASR_LOG_ERROR("endVadTime is invalid");
            return nullptr;
        }
        if (!asr::Asr::CheckLang(para->config->lang)) {
            ASR_LOG_ERROR("lang is invalid");
            return nullptr;
        }
    }
    if (!ai::checkEngine(para->auth->type)) {
        ASR_LOG_ERROR("engine_type is invalid");
        return nullptr;
    }

    bool need_check = true;
    FeatureInterfaceHandle interface = system_ai_speech_createAsr_instance(handle);
    if (ai::whitelistVerifi(FeatureGetPackageName(FeatureGetProtoHandle(handle)))) {
        need_check = false;
    }

    asr::Asr::asr_info info { 0 };
    info.timeout = para->config ? para->config->endVadTime : asr::Asr::kDefaultTimeout;
    info.lang = para->config ? para->config->lang : "auto";
    info.engine_type = para->auth->type ? para->auth->type : "volcengine";
    info.config = FeatureGetJsonString(para->auth->config);
    info.translations = para->config && para->config->translations ? (const char**)para->config->translations->_element : nullptr;
    info.tran_count = para->config && para->config->translations ? para->config->translations->_size : 0;
    auto asr = asr::Asr::Create(interface, info, need_check);
    if (!asr) {
        ASR_LOG_ERROR("create asr failed");
        FeatureFreeInstanceHandle(interface);
        return nullptr;
    }
    FeatureSetObjectData(interface, asr.release());
    return interface;
}

void system_ai_speech_SpeechRecognition_interface_Asr_finalize(FeatureInterfaceHandle handle)
{
    std::unique_ptr<asr::Asr> asr(reinterpret_cast<asr::Asr*>(FeatureGetObjectData(handle)));
    FeatureSetObjectData(handle, nullptr);
}

void system_ai_speech_SpeechRecognition_interface_Asr_start(
    FeatureInterfaceHandle handle, AppendData adata)
{
    auto asr = reinterpret_cast<asr::Asr*>(FeatureGetObjectData(handle));
    // 默认值
    asr->Start();
}

void system_ai_speech_SpeechRecognition_interface_Asr_cancel(
    FeatureInterfaceHandle handle, AppendData adata)
{
    auto asr = reinterpret_cast<asr::Asr*>(FeatureGetObjectData(handle));
    asr->Cancel();
}

void system_ai_speech_SpeechRecognition_interface_Asr_finish(
    FeatureInterfaceHandle handle, AppendData adata)
{
    auto asr = reinterpret_cast<asr::Asr*>(FeatureGetObjectData(handle));
    asr->Finish();
}

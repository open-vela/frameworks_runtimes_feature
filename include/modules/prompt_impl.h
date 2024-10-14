#ifndef FEATURE_PROMPT_IMPL_H_
#define FEATURE_PROMPT_IMPL_H_

#include <stdint.h>

struct PromptManager;

typedef void (*promptInit)(void* feature, void* data);
typedef void (*promptUninit)(void* feature);
typedef void (*promptShowToast)(void* feature, const char* msg, int32_t duration);
typedef void (*promptCleanOnDetached)(void* feature);

typedef void (*successCb)(void* feature, int32_t success, int index);
typedef void (*cancelCb)(void* feature, int32_t cancel);
typedef void (*completeCb)(void* feature, int32_t complete);

typedef struct PromptDialogParams {
    void* handle;
    char* title;
    char* msg;
    char* buttons;
    bool autocancel;
    int32_t success;
    int32_t cancel;
    int32_t complete;
    successCb success_cb;
    cancelCb cancel_cb;
    completeCb complete_cb;
} PromptDialogParams;

void prompt_dialog_free(PromptDialogParams* params);

typedef void (*promptShowDialog)(PromptDialogParams* params);

struct PromptInterfaceHandler {
    void promptInterfaceInit(void* user_data);
    promptInit init;
    promptUninit uninit;
    promptShowToast show_toast;
    promptShowDialog show_dialog;
    promptCleanOnDetached cleanup;
    PromptManager* pm;
    void* data;
};

#endif

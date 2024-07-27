/*
 * Copyright (C) 2023 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * 	 http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "builtin_console.h"

#include "feature.h"
#include "feature_log.h"

#ifndef countof
#define countof(x) (sizeof(x) / sizeof((x)[0]))
#endif

namespace builtin {
static JSValue jsPrint(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv,
    int magic)
{
    const char* str;
    for (int i = 0; i < argc; ++i) {
        str = JS_ToCString(ctx, argv[i]);
        if (!str) {
            return JS_EXCEPTION;
        }
        printf("[jidl_console] %s\n", str);
        JS_FreeCString(ctx, str);
    }
    return JS_UNDEFINED;
}

static const JSCFunctionListEntry aiot_core_funcs[] = {
    JS_CFUNC_MAGIC_DEF("debug", 1, jsPrint, FEATURE_LOG_LEVEL_DEBUG),
    JS_CFUNC_MAGIC_DEF("log", 1, jsPrint, FEATURE_LOG_LEVEL_INFO),
    JS_CFUNC_MAGIC_DEF("info", 1, jsPrint, FEATURE_LOG_LEVEL_INFO),
    JS_CFUNC_MAGIC_DEF("warn", 1, jsPrint, FEATURE_LOG_LEVEL_WARNING),
    JS_CFUNC_MAGIC_DEF("error", 1, jsPrint, FEATURE_LOG_LEVEL_ERROR),
    JS_CFUNC_MAGIC_DEF("assert", 1, jsPrint, FEATURE_LOG_LEVEL_ALERT),
};

static int js_aiot_core_init(JSContext* ctx, JSModuleDef* m)
{
    return JS_SetModuleExportList(ctx, m, aiot_core_funcs, countof(aiot_core_funcs));
}

static JSModuleDef* js_init_module_aiot_core(JSContext* ctx, const char* module_name)
{
    JSModuleDef* m;
    m = JS_NewCModule(ctx, module_name, js_aiot_core_init);
    if (!m)
        return nullptr;

    JS_AddModuleExportList(ctx, m, aiot_core_funcs, countof(aiot_core_funcs));
    return m;
}

static void evalFileModule(JSContext* ctx, const char* filename, const char* content)
{
    int eval_flags = JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY;
    int size = strlen(content);
    if (!content || size <= 0) {
        printf("Error: pass error file content");
        return;
    }

    feature_value_t func_val = JS_Eval(ctx, content, size, filename, eval_flags);
    if (JS_IsException(func_val)) {
        printf("could read js file module: '%s'", filename);
        goto fail;
    }

    if (JS_VALUE_GET_TAG(func_val) == JS_TAG_MODULE) {
        if (JS_ResolveModule(ctx, func_val) < 0) {
            goto fail;
        }

        func_val = JS_EvalFunction(ctx, func_val);
        if (JS_IsException(func_val)) {
            printf("Fail to eval file module: %s", filename);
            goto fail;
        }
    }

    JS_FreeValue(ctx, func_val);
    return;

fail:
    feature_dump_error(ctx);
    JS_FreeValue(ctx, func_val);
    return;
}

void addConsoleModule(JSContext* ctx, const char* filename, const char* content)
{
    js_init_module_aiot_core(ctx, "@aiot");
    evalFileModule(ctx, filename, content);
}

} // namespace builtin

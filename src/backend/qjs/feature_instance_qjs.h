
/*
 * Copyright (C) 2023 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __FEATURE_INSTANCE_QJS_H__
#define __FEATURE_INSTANCE_QJS_H__

#include "feature.h"
#include "feature_common.h"
#include "feature_instance.h"
// clang-format off
#include "callback_manager_qjs.h"
#include "callback_manager.h"
#include "feature_event_manager.h"
// clang-format on
#include "promise_manager.h"

#include <map>
#include <memory>

namespace feature_framework {

class FeaturePrototype;
class PromiseManager;

typedef struct WeakRef {
    ft_value_t ft_value;
    struct weakref_list_node link;
} WeakRef;

class FeatureInstanceQjs : public FeatureInstance,
                           public PromiseManager,
                           public CallbackManager<JSContext*, JSValue, FeatureInstanceQjs>,
                           public EventManager<JSContext*, JSValue, FeatureInstanceQjs> {
public:
    FeatureInstanceQjs(FeaturePrototype* proto);

    FeatureInstanceQjs(FeaturePrototype* module_proto, VTable* vtable);

    virtual ~FeatureInstanceQjs();

    void setVmObject(feature_value_t vm_object);

    feature_value_t getVmObject() const;

    feature_value_t getFeatureJsvalue(ft_value_t ft_value);

    virtual bool removeCallback(FtCallbackId cid);

    virtual int resolvePromise(FtPromiseId pid, va_list& ap);

    virtual int rejectPromise(FtPromiseId pid, int code, const char* msg);

    virtual int getPromiseType(FtPromiseId pid);

    virtual int invokeCallback(FtCallbackId cid, va_list& ap);

    virtual int invokeCallbackCount(FtCallbackId cid, va_list& ap, int count);

    virtual bool emitEvent(FtEventId cid, va_list& ap);

    virtual void setEventChangeListener(FeatureEventChangeListener listener);

    virtual FtEventId getEventId(const char* name);

    virtual const char* getEventName(FtEventId eid);

    virtual int getEventCallbackCount(FtEventId eid);

    virtual void throwError(const char* msg);

    bool checkCallback(FtCallbackId cid);

    void markValues(feature_runtime_ref rt, feature_mark_func mark_func);

    bool initWeakRef(feature_value_t feature_object);

    void freeWeakRef();

    JSContext* getContext();

    int doInvokeCallback(const FeatureType* param_types, feature_value_t callback, va_list& ap, int fixed_argc, int rest_argc);

    virtual void initialize();

    feature_value_t dupTarget();

    virtual void onDetached();

    void onDumpMemory(FeatureMemoryDump* dump, void* userdata) override;

private:
    feature_value_t vm_object_;
    feature_value_t target_;
    WeakRef weak_self_;
};

}
#endif // __FEATURE_INSTANCE_QJS_H__

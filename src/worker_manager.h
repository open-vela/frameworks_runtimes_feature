
#pragma once
#include "feature_types.h"
#include <list>
#include <uv.h>

namespace feature_framework {

enum FeatureWorkerState {
    FEATURE_WORKER_INITED, // worker未提交
    FEATURE_WORKER_PENDING, // worker等待中
    FEATURE_WORKER_RUNNING,
    FEATURE_WORKER_INVALID, // worker处于无效状态
    FEATURE_WORKER_RESOLVED,
    FEATURE_WORKER_REJECTED,
    FEATURE_WORKER_FINISHED, // worker已经完成
};

using do_work_fn = void (*)(FeatureWorkerHandle);
using do_after_worker_fn = void (*)(FeatureWorkerHandle);
using do_free_fn = void (*)(void*);

typedef struct _FeatureWorker {
    uv_work_t worker; // worker
    void* user_data; // user data
    do_work_fn do_work;
    do_after_worker_fn do_after_worker;
    do_free_fn do_free;
    FtPromiseId pid;
    int status;
} FeatureWorker;

class WorkerManager {
private:
    FeatureInstanceHandle _handle;
    std::list<FeatureWorker*> _pendingTasks;
    uv_loop_t* _loop = nullptr;

public:
    WorkerManager() = default;
    bool init(uv_loop_t* loop)
    {
        _loop = loop;
        return true;
    }
    FeatureWorker* create(FtPromiseId pid, size_t buf_size, do_work_fn do_work, do_after_worker_fn do_after_worker, do_free_fn free);
    bool commit(FeatureWorker* pWorker);
    bool resolve(FeatureWorker* pWorker, FeatureWorkerResult result);
    bool reject(FeatureWorker* pWorker, int error_code, const char* error_msg);
    bool checkValid(FeatureWorker* pWorker);
    bool cancel(FeatureWorker* pWorker);
};
}
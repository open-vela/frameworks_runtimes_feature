#include "worker_manager.h"
#include "feature_exports.h"
#include "feature_log.h"
#include <algorithm>
#include <cstdlib>
#include <uv.h>

namespace feature_framework {

FeatureWorker* WorkerManager::create(FtPromiseId pid, size_t buf_size, do_work_fn do_work, do_after_worker_fn do_after_worker, do_free_fn free)
{
    auto pWorker = new FeatureWorker();
    pWorker->do_work = do_work;
    pWorker->do_after_worker = do_after_worker;
    pWorker->do_free = free;
    pWorker->status = FEATURE_WORKER_INITED;
    pWorker->user_data = buf_size ? malloc(buf_size) : nullptr;
    pWorker->pid = pid;
    _pendingTasks.push_back(pWorker);
    return pWorker;
}

bool WorkerManager::commit(FeatureWorker* pWorker)
{
    pWorker->worker.data = pWorker;
    int ret = uv_queue_work(
        _loop, &pWorker->worker, [](uv_work_t* req) {
        auto pWorker1 = static_cast<FeatureWorker*>(req->data);
        pWorker1->status = FEATURE_WORKER_RUNNING;
        pWorker1->do_work((FeatureWorkerHandle)pWorker1); }, [](uv_work_t* req, int status) {
        auto pWorker2 = static_cast<FeatureWorker*>(req->data);
        pWorker2->status = status;
        pWorker2->do_after_worker((FeatureWorkerHandle)pWorker2);
        // if need free user_data
        if (pWorker2->user_data) {
            if(pWorker2->do_free) {
                pWorker2->do_free(pWorker2->user_data);
            } else {
                free(pWorker2->user_data);
            }
        } });
    if (ret) {
        FEATURE_LOG_ERROR("commit worker failed %d:%s", ret, uv_strerror(ret));
        return false;
    }
    return true;
}

bool WorkerManager::resolve(FeatureWorker* pWorker, FeatureWorkerResult result)
{
    return FeaturePromiseResolve(_handle, pWorker->pid, result.ival);
}

bool WorkerManager::reject(FeatureWorker* pWorker, int error_code, const char* error_msg)
{
    return FeaturePromiseReject(_handle, pWorker->pid, error_code, error_msg);
}

bool WorkerManager::checkValid(FeatureWorker* pWorker)
{
    auto pos = std::find_if(_pendingTasks.begin(), _pendingTasks.end(), [pWorker](const FeatureWorker* ref) {
        return ref == pWorker;
    });
    return pos != _pendingTasks.end();
}

bool WorkerManager::cancel(FeatureWorker* pWorker)
{
    auto pos = std::find(_pendingTasks.begin(), _pendingTasks.end(), pWorker);
    if (pos != _pendingTasks.end()) {
        return false;
    }
    uv_cancel((uv_req_t*)&pWorker->worker);
    return true;
}

}
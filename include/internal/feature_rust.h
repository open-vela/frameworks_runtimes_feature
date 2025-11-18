#ifndef __FEATURE_RUST_H__
#define __FEATURE_RUST_H__

#ifdef __cplusplus
extern "C" {
#endif

void* init_vdk_async_runtime(void* uvloop_ptr);
void close_vdk_async_runtime(void* runtime_ptr);

#ifdef __cplusplus
}
#endif

#endif // __FEATURE_RUST_H__

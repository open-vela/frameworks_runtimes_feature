// Copyright 2023 Xiaomi, Inc. All rights reserved.

#include "miplay_1_0.h"
#include "uv.h"
#include <thread>
#include <sstream>
#include <string>

static const char* file_tag = "[jidl_feature] miplay_1_0_impl";
static uv_loop_t* loop = nullptr;
static struct{
    std::string state;
    std::string artist;
    std::string title;
    std::string duration;
    std::string position;
} gMediainfo;
static FeatureInstanceHandle gFeature;
static FtCallbackId gMediainfoCb;

void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    printf("%s::%s()\n", file_tag,  __FUNCTION__);
    buf->base = (char*) malloc(suggested_size);
    buf->len = suggested_size;
}

void on_close(uv_handle_t *handle) {
    printf("%s::%s()\n", file_tag,  __FUNCTION__);
    if (handle != NULL)
    {
        free(handle);
    }
}

void read_cb(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
    printf("%s::%s()\n", file_tag,  __FUNCTION__);
    if (nread > 0) {
        printf("%s::read_cb: recv buf %s\n", file_tag,  buf->base);
        // std::string revc_str(buf->base);
        std::istringstream iss(buf->base);
        char split = '+';
        std::getline(iss, gMediainfo.state, split);
        printf("%s::read_cb: recv state %s\n", file_tag,  gMediainfo.state.c_str());
        std::getline(iss, gMediainfo.title, split);
        printf("%s::read_cb: recv title %s\n", file_tag,  gMediainfo.title.c_str());
        std::getline(iss, gMediainfo.artist, split);
        printf("%s::read_cb: recv artist %s\n", file_tag,  gMediainfo.artist.c_str());
        std::getline(iss, gMediainfo.duration, split);
        printf("%s::read_cb: recv duration %s\n", file_tag,  gMediainfo.duration.c_str());
        std::getline(iss, gMediainfo.position, split);
        printf("%s::read_cb: recv position %s\n", file_tag,  gMediainfo.position.c_str());
        if (!FeatureInvokeCallback(gFeature, gMediainfoCb, atoi(gMediainfo.state.c_str()), gMediainfo.artist.c_str(), gMediainfo.title.c_str(), atoi(gMediainfo.duration.c_str()), atoi(gMediainfo.position.c_str()))) {
            FEATURE_LOG_ERROR("invoke failed !");
            return;
        }
        return;
    }
    if (nread < 0) {
        if (nread != UV_EOF)
            fprintf(stderr, "Read error %s\n", uv_err_name(nread));
        uv_close((uv_handle_t*) client, on_close);
    }

    if (buf->base != NULL) {
        free(buf->base);
    }
}

void on_new_connection(uv_stream_t *server, int status) {
    printf("%s::%s()\n", file_tag,  __FUNCTION__);
    if (status < 0) {
        fprintf(stderr, "New connection error %s\n", uv_strerror(status));
        // error!
        return;
    }

    uv_tcp_t *client = (uv_tcp_t*) malloc(sizeof(uv_tcp_t));
    uv_tcp_init(loop, client);
    if (uv_accept(server, (uv_stream_t*) client) == 0) {
        uv_read_start((uv_stream_t*) client, alloc_buffer, read_cb);
        printf("uv_read_start\n");
    }
    else {
        uv_close((uv_handle_t*) client, NULL);
    }
}

int runloop()
{
    printf("%s::%s()\n", file_tag,  __FUNCTION__);

    loop = (uv_loop_t*) malloc(sizeof(uv_loop_t));
    memset(loop, 0, sizeof(uv_loop_t));
    uv_loop_init(loop);

    uv_tcp_t server;
    uv_tcp_init(loop, &server);

    sockaddr_in addr;

    uv_ip4_addr("0.0.0.0", 7979, &addr);

    uv_tcp_bind(&server, (const struct sockaddr*)&addr, 0);
    int r = uv_listen((uv_stream_t*) &server, 16, on_new_connection);
    if (r) {
        fprintf(stderr, "Listen error %s\n", uv_strerror(r));
        return 1;
    }
    return uv_run(loop, UV_RUN_DEFAULT);
}

// FeatureCallbacks to be implemented
void Miplay_onRegister(const char* feature_name)
{
    printf("%s::%s()\n", file_tag,  __FUNCTION__);
}

void Miplay_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    printf("%s::%s()\n", file_tag,  __FUNCTION__);
}

void Miplay_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    printf("%s::%s()\n", file_tag,  __FUNCTION__);
}

void Miplay_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    printf("%s::%s()\n", file_tag,  __FUNCTION__);
}

void Miplay_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    printf("%s::%s()\n", file_tag,  __FUNCTION__);
    uv_loop_close(loop);
    free(loop);
    loop = nullptr;
    FeatureRemoveCallback(gFeature, gMediainfoCb);
    gFeature = nullptr;
    gMediainfoCb = 0;
}

void Miplay_onUnregister(const char* feature_name)
{
    printf("%s::%s()\n", file_tag,  __FUNCTION__);
}

// Function wrappers to be implemented
void Miplay_wrap_init(FeatureInstanceHandle feature, AppendData data, FtCallbackId cb)
{
    printf("%s::%s()\n", file_tag,  __FUNCTION__);
    gFeature = feature;
    gMediainfoCb = cb;
    std::thread t1(runloop);
    t1.detach();
    printf("%s::%s()  END\n", file_tag,  __FUNCTION__);
}

void Miplay_wrap_uninit(FeatureInstanceHandle feature, AppendData data)
{
    FeatureRemoveCallback(gFeature, gMediainfoCb);
    gFeature = nullptr;
    gMediainfoCb = 0;
    printf("%s::%s()\n", file_tag,  __FUNCTION__);
}
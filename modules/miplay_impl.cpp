// Copyright 2023 Xiaomi, Inc. All rights reserved.

#include "miplay.h"
#include "uv.h"
#include <thread>
#include <sstream>
#include <string>
#include <spawn.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MIPLAY_QAPP_THREAD_STACK_SIZE 14336
static const char* file_tag = "[jidl_feature] miplay_impl";
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

static void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    printf("%s::%s()\n", file_tag,  __FUNCTION__);
    buf->base = (char*) malloc(suggested_size);
    buf->len = suggested_size;
}

static void on_close(uv_handle_t *handle) {
    printf("%s::%s()\n", file_tag,  __FUNCTION__);
    if (handle != NULL)
    {
        free(handle);
    }
}

static void read_cb(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
    printf("%s::%s()\n", file_tag,  __FUNCTION__);
    if (nread > 0) {
        printf("%s::read_cb: recv buf %s\n", file_tag,  buf->base);
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

static void on_new_connection(uv_stream_t *server, int status) {
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

void* runloop(void* arg)
{
    loop = (uv_loop_t*) malloc(sizeof(uv_loop_t));
    memset(loop, 0, sizeof(uv_loop_t));
    uv_loop_init(loop);

    uv_tcp_t server;
    uv_tcp_init(loop, &server);

    sockaddr_in addr;
    uv_ip4_addr("0.0.0.0", 7979, &addr);

    uv_tcp_bind(&server, (const struct sockaddr*)&addr, 0);
    int r = uv_listen((uv_stream_t*)&server, 16, on_new_connection);
    if (r) {
        fprintf(stderr, "Listen error %s\n", uv_strerror(r));
        return nullptr;
    }
    uv_run(loop, UV_RUN_DEFAULT);
    return nullptr;
}

// FeatureCallbacks to be implemented
void service_miplay_onRegister(const char* feature_name)
{
    printf("%s::%s()\n", file_tag,  __FUNCTION__);
}

void service_miplay_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    printf("%s::%s()\n", file_tag,  __FUNCTION__);
}

void service_miplay_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    printf("%s::%s()\n", file_tag,  __FUNCTION__);
}

void service_miplay_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    printf("%s::%s()\n", file_tag,  __FUNCTION__);
}

void service_miplay_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    printf("%s::%s()\n", file_tag,  __FUNCTION__);
    loop = nullptr;
    FeatureRemoveCallback(gFeature, gMediainfoCb);
    gFeature = nullptr;
    gMediainfoCb = 0;
}

void service_miplay_onUnregister(const char* feature_name)
{
    printf("%s::%s()\n", file_tag,  __FUNCTION__);
}

// Function wrappers to be implemented
void service_miplay_wrap_init(FeatureInstanceHandle feature, AppendData data, FtCallbackId cb)
{
    printf("%s::%s()\n", file_tag,  __FUNCTION__);
    gFeature = feature;
    gMediainfoCb = cb;

    pthread_attr_t thread_attr; 
    pthread_t thread_info; 
    if (pthread_attr_init(&thread_attr) != 0) {
        return;
    }
    pthread_attr_setstacksize(&thread_attr, MIPLAY_QAPP_THREAD_STACK_SIZE);
    if (pthread_create(&thread_info, &thread_attr, runloop, NULL) != 0) {
        return;
    }
    if (pthread_setname_np(thread_info, "quickapp_miplay") != 0) {
        return;
    }
    pthread_detach(thread_info);
}

void service_miplay_wrap_uninit(FeatureInstanceHandle feature, AppendData data)
{
    FeatureRemoveCallback(gFeature, gMediainfoCb);
    gFeature = nullptr;
    gMediainfoCb = 0;
    printf("%s::%s()\n", file_tag,  __FUNCTION__);
}

static void miplay_ctrl_task(char* args[])
{
    const char* command = "reverseCtrl";
    char *const vapp_argv[] = {const_cast<char*>(command), args[0], args[1], nullptr };
    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);    
    pid_t pid;
    if (posix_spawn(&pid, command, &actions, NULL, vapp_argv, environ) != 0) {
        // Error occurred while spawning the process
        FEATURE_LOG_ERROR("posix_spawn %s error\n", command);
        return;
    }

    int status;
    if (waitpid(pid, &status, 0) == -1) {
        FEATURE_LOG_ERROR("waitpid error %d\n", status);
        return;
    }

    if (WIFEXITED(status)) {
        int exitStatus = WEXITSTATUS(status);
        FEATURE_LOG_WARN("Child process exited with status:%d", exitStatus);
    }

    return;
}

void service_miplay_wrap_ctrlcmd(FeatureInstanceHandle feature, AppendData data, FtString cmd)
{
    FEATURE_LOG_WARN("%s app click callback cmd:%s..\n", __FUNCTION__, cmd);
    char* args[3] = {nullptr};
    
    if (strcmp(cmd, "pause") == 0) {
        args[0] = const_cast<char*>("ctrl");
        args[1] = const_cast<char*>("pause");
    } else if (strcmp(cmd, "resume") == 0) {
        args[0] = const_cast<char*>("ctrl");
        args[1] = const_cast<char*>("resume");
    } else if (strcmp(cmd, "next") == 0) {
        args[0] = const_cast<char*>("ctrl");
        args[1] = const_cast<char*>("next");
    } else if (strcmp(cmd, "prev") == 0) {
        args[0] = const_cast<char*>("ctrl");
        args[1] = const_cast<char*>("prev");
    } else if (strcmp(cmd, "stop") == 0) {
        args[0] = const_cast<char*>("ctrl");
        args[1] = const_cast<char*>("stop");
    }
    miplay_ctrl_task(args);
}

void service_miplay_wrap_volumeCtrl(FeatureInstanceHandle feature, AppendData append_data, FtInt type)
{
    FEATURE_LOG_WARN("%s volumeCtrl click callback cmd:%d..\n", __FUNCTION__, type);
    char* args[3] = {nullptr};

    if (type == 1) {
        args[0] = const_cast<char*>("volumeCtrl");
        args[1] = const_cast<char*>("up");
    } else {
        args[0] = const_cast<char*>("volumeCtrl");
        args[1] = const_cast<char*>("down");
    }
    miplay_ctrl_task(args);
}

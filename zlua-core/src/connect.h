#pragma once

#include "uv.h"
#include "zlua.h"

#include <thread>

struct conn_t {
    uv_loop_t* loop;
    uv_tcp_t server;
    uv_tcp_t client;

    char* buf;
    size_t maxsize;
    size_t size;

    bool runing;
    HANDLE thread;
    DWORD threadId;

    int head;
};

typedef struct {
    uv_write_t req;
    uv_buf_t buf;
    uv_stream_t* stream;
} write_req_t;

conn_t* conn_new();
int conn_listen(conn_t* conn, const char* host, int port, const char* err);
int conn_send(conn_t* conn, int cmd, const char* data, int len);
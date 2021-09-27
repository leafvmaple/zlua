#pragma once

#include "uv.h"
#include "zlua.h"

#include <thread>

typedef struct {
    uv_write_t req;
    uv_buf_t buf;
    uv_stream_t* stream;
} WriteReq;

class Connection
{
public:
    Connection();

    int Listen(const char* host, int port, char* err);
    int Accept(uv_stream_t* server, int status);

    int Send(int cmd, const char* data, int len);
    int Receive(const char* data, size_t len);

    int Run();

    uv_tcp_t server_tcp_;
    uv_tcp_t client_tcp_;

    uv_loop_t* loop_;

    char* buf_;
    size_t max_size_;
    size_t size_;

    bool runing_;
    HANDLE thread_;
    DWORD thread_id_;

    int head_;
};
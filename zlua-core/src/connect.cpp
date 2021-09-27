#include "document.h"
#include "rapidjson.h"
#include "encodings.h"

#include "zlua.h"
#include "connect.h"
#include "uv.h"
#include "json.h"
#include "event.h"

#include <assert.h>
#include <set>
#include <vector>
#include <processthreadsapi.h>

void OnEchoAlloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
    buf->base = static_cast<char*>(malloc(suggested_size));
    buf->len = suggested_size;
}

void OnAfterRead(uv_stream_t* client, ssize_t nread, const uv_buf_t* buf) {
    auto conn = static_cast<Connection*>(client->data);
    if (!conn)
        return;

    if (nread < 0) {
        delete buf->base;
        uv_close(reinterpret_cast<uv_handle_t*>(client), nullptr);
        return;
    }

    if (nread == 0) {
        delete buf->base;
        return;
    }

    if (!conn->Receive(buf->base, nread))
        return;
}

void OnAfterAsync(uv_handle_t* handle) {
    delete handle;
}

void OnAfterWrite(uv_write_t* req, int status) {
    const auto* write_req = reinterpret_cast<WriteReq*>(req);
    delete write_req->buf.base;
    delete write_req;
}

void OnAsyncWrite(uv_async_t* async) {
    auto* write_req = (WriteReq*)async->data;
    uv_write(&write_req->req, write_req->stream, &write_req->buf, 1, OnAfterWrite);
    uv_close((uv_handle_t*)async, OnAfterAsync);
}

void OnNewConnection(uv_stream_t* server, int status) {
    Connection* conn = reinterpret_cast<Connection*>(server->data);
    if (conn)
        conn->Accept(server, status);
}

DWORD OnThread(void* ptr) {
    Connection* conn = static_cast<Connection*>(ptr);
    if (conn)
        conn->Run();

    return 0;
}

Connection::Connection() :
    buf_(nullptr),
    max_size_(0),
    size_(0),
    runing_(false),
    thread_(nullptr),
    thread_id_(0),
    head_(0)
{
    loop_ = uv_loop_new();
}

int Connection::Listen(const char* host, int port, char* err)
{
    sockaddr_in addr;

    server_tcp_.data = this;

    uv_tcp_init(loop_, &server_tcp_);
    uv_ip4_addr(host, port, &addr);
    uv_tcp_bind(&server_tcp_, reinterpret_cast<const struct sockaddr*>(&addr), 0);
    const int r = uv_listen(reinterpret_cast<uv_stream_t*>(&server_tcp_), SOMAXCONN, OnNewConnection);
    if (r) {
        if (err) {
            strcpy(err, uv_strerror(r));
        }
        return false;
    }

    thread_ = CreateThread(NULL, 0, &OnThread, (void*)this, 0, &thread_id_);

    return true;
}

int Connection::Accept(uv_stream_t* server, int status)
{
    client_tcp_.data = this;

    uv_tcp_init(loop_, &client_tcp_);
    uv_accept(server, reinterpret_cast<uv_stream_t*>(&client_tcp_));
    uv_read_start(reinterpret_cast<uv_stream_t*>(&client_tcp_), OnEchoAlloc, OnAfterRead);
}

int Connection::Send(int cmd, const char* data, int len)
{
#ifdef EMMY
    auto* writeReq = new WriteReq;
    char cmdValue[100];
    const int l1 = sprintf(cmdValue, "%d\n", cmd);
    const size_t newLen = len + l1 + 1;
    char* newData = new char[newLen];

    memcpy(newData, cmdValue, l1);
    memcpy(newData + l1, data, len);
    newData[newLen - 1] = '\n';
    writeReq->buf = uv_buf_init(newData, newLen);
    writeReq->stream = (uv_stream_t*)&client_tcp_;;

    auto* async = new uv_async_t;
    async->data = writeReq;
#else

    auto* write_req = new WriteReq;
    char* buf = new char[len];
    if (!buf)
        return 0;

    memcpy(buf, data, len);
    write_req->buf = uv_buf_init(buf, len);
    write_req->stream = (uv_stream_t*)&client_tcp_;

    // thread safe:
    auto* async = new uv_async_t;
    async->data = write_req;

#endif
    uv_async_init(loop_, async, OnAsyncWrite);
    uv_async_send(async);

    return true;
}

int Connection::Receive(const char* data, size_t len)
{
#ifdef EMMY
    if (conn->maxsize < conn->size + len) {
        conn->buf = static_cast<char*>(realloc(conn->buf, conn->size + len));
        if (!conn->buf)
            return 0;
        conn->maxsize = conn->size + len;
    }
    memcpy(conn->buf + conn->size, data, len);
    conn->size += len;

    size_t pos = 0;
    while (true) {
        size_t start = pos;
        for (size_t i = pos; i < conn->size; i++) {
            if (data[i] == '\n') {
                pos = i + 1;
                break;
            }
        }
        if (start != pos) {
            if (!conn->head) {
                // skip
            }
            else {
                EventHandle(conn, conn->buf + start, pos - start);
            }
            conn->head = !conn->head;
        }
        else break;
    }

    if (pos > 0) {
        memcpy(conn->buf, conn->buf + pos, conn->size - pos);
        conn->size -= pos;
    }
#else
    const char* end = data + len;
    while (data < end) {
        int len = strlen(data);
        EventHandle(this, data, len);
        data += len + 1;
    }
#endif

    return true;
}

int Connection::Run()
{
    runing_ = true;
    while (runing_) {
        uv_run(loop_, UV_RUN_DEFAULT);
    }
}

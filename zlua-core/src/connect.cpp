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

int conn_receive(conn_t* conn, const char* data, size_t len);

void on_echo_alloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
    buf->base = static_cast<char*>(malloc(suggested_size));
    buf->len = suggested_size;
}

void on_after_read(uv_stream_t* client, ssize_t nread, const uv_buf_t* buf) {
    auto conn = static_cast<conn_t*>(client->data);
    if (!conn)
        return;

    if (nread < 0) {
        free(buf->base);
        uv_close(reinterpret_cast<uv_handle_t*>(client), nullptr);
        return;
    }

    if (nread == 0) {
        free(buf->base);
        return;
    }

    if (!conn_receive(conn, buf->base, nread))
        return;
}

static void on_after_write(uv_write_t* req, int status) {
    const auto* write_req = reinterpret_cast<write_req_t*>(req);
    free(write_req->buf.base);
    delete write_req;
}

static void on_after_async(uv_handle_t* h) {
    delete h;
}

void on_async_write(uv_async_t* async) {
    auto* write_req = (write_req_t*)async->data;
    uv_write(&write_req->req, write_req->stream, &write_req->buf, 1, on_after_write);
    uv_close((uv_handle_t*)async, on_after_async);
}

void on_new_connection(uv_stream_t* server, int status) {
    conn_t* conn = reinterpret_cast<conn_t*>(server->data);
    conn->client.data = conn;

    uv_tcp_init(conn->loop, &conn->client);
    uv_accept(server, reinterpret_cast<uv_stream_t*>(&conn->client));
    uv_read_start(reinterpret_cast<uv_stream_t*>(&conn->client), on_echo_alloc, on_after_read);
}


conn_t* conn_new() {
    conn_t* conn = (conn_t*)malloc(sizeof(conn_t));
    if (!conn)
        return nullptr;
    memset(conn, 0, sizeof(conn_t));
    conn->loop = uv_loop_new();

    return conn;
}

int conn_receive(conn_t* conn, const char* data, size_t len) {
    event_handle(conn, data, len);

    return true;
}

DWORD conn_thread(void* ptr) {
    conn_t* conn = static_cast<conn_t*>(ptr);
    if (!conn)
        return 0;

    conn->runing = true;
    while (conn->runing) {
        uv_run(conn->loop, UV_RUN_DEFAULT);
    }

    return 0;
}

int conn_listen(conn_t* conn, const char* host, int port, char* err) {
    sockaddr_in addr;

    conn->server.data = conn;

    uv_tcp_init(conn->loop, &conn->server);
    uv_ip4_addr(host, port, &addr);
    uv_tcp_bind(&conn->server, reinterpret_cast<const struct sockaddr*>(&addr), 0);
    const int r = uv_listen(reinterpret_cast<uv_stream_t*>(&conn->server), SOMAXCONN, on_new_connection);
    if (r) {
        if (err) {
            strcpy(err, uv_strerror(r));
        }
        return false;
    }
    
    conn->thread = CreateThread(NULL, 0, &conn_thread, (void*)conn, 0, &conn->threadId);

    return true;
}

int conn_send(conn_t* conn, int cmd, const char* data, int len)
{
    uv_stream_t* client = (uv_stream_t*)&conn->client;

    auto* write_req = new write_req_t;
    char* buf = static_cast<char*>(malloc(len));
    if (!buf)
        return 0;

    memcpy(buf, data, len);
    write_req->buf = uv_buf_init(buf, len);
    write_req->stream = client;

    // thread safe:
    auto* async = new uv_async_t;
    async->data = write_req;
    uv_async_init(conn->loop, async, on_async_write);
    uv_async_send(async);

    return true;
}

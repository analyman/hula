#include "../include/kserver.h"
#include "../include/logger.h"

#include <uv.h>

#include <stdlib.h>

#include <tuple>


namespace KProxyServer {

void ServerToNetConnection::tcp_read_callback(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) //{
{
    if(nread < 0) {
        // TODO
    } else if (nread == 0) { // EOF
        // TODO
    }
    ServerToNetConnection* _this = (ServerToNetConnection*)uv_handle_get_data((uv_handle_t*)stream);
    _this->realy_back(*buf);
    return;
} //}
void ServerToNetConnection::tcp_connect_callback(uv_connect_t* req, int status) //{
{
    ServerToNetConnection* _this = (ServerToNetConnection*)uv_req_get_data((uv_req_t*)req);

    delete req;

    if(status < 0) {
        // TODO
    }

    uv_read_start((uv_stream_t*)_this->mp_tcp, 
            ServerToNetConnection::tcp_alloc_callback,
            ServerToNetConnection::tcp_read_callback
            );
} //}
void ServerToNetConnection::tcp_alloc_callback(uv_handle_t* req, size_t suggested_size, uv_buf_t* buf) //{
{
    buf->base = (char*)malloc(suggested_size);
    buf->len  = suggested_size;
    return;
} //}

ServerToNetConnection::ServerToNetConnection(const sockaddr* addr, ConnectionId id, ClientConnectionProxy* p, uv_read_cb rcb) //{
{
    Logger::debug("ServerToNetConnectio::construtor() called new proxy connection");
    this->m_connectionWrapper = p;
    this->m_read_callback = rcb;
    this->id = id;
    this->used_buffer_size = 0;
    this->mp_tcp = new uv_tcp_t();

    uv_handle_set_data((uv_handle_t*)this->mp_tcp, this);

    uv_connect_t* p_req = new uv_connect_t();
    uv_req_set_data((uv_req_t*)p_req, this);

    uv_tcp_connect(p_req, this->mp_tcp, addr, 
                   ServerToNetConnection::tcp_connect_callback);
} //}

int ServerToNetConnection::write(uv_buf_t bufs[], unsigned int nbufs, uv_write_cb cb) //{
{
    Logger::debug("ServerToNetConnection::write() called");
    using data_type = std::tuple<decltype(this), decltype(cb)>;

    uv_write_t* p_req = new uv_write_t();
    uv_req_set_data((uv_req_t*)p_req, new data_type(this, cb));

    for(int i=0;i<nbufs;i++)
        this->used_buffer_size += bufs[i].len;

    return uv_write(p_req, (uv_stream_t*)this->mp_tcp, bufs, nbufs, 
            [](uv_write_t* req, int status) -> void {
                data_type* x = static_cast<data_type*>(uv_req_get_data((uv_req_t*)req));
                ServerToNetConnection* _this = std::get<0>(*x);
                uv_write_cb cb = std::get<1>(*x);
                delete x;
                
                for(int i=0;i<req->nbufs;i++)
                    _this->used_buffer_size -= req->bufs[i].len;

                cb(req, status);
            });
} //}

ServerToNetConnection::~ServerToNetConnection() //{
{
    Logger::debug("ServerToNetConnection:deconstructor() called");
    delete this->mp_tcp;
    this->mp_tcp = nullptr;
} //}

}

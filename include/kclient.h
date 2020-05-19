#pragma once

#include <uv.h>

#include <string>
#include <vector>
#include <tuple>
#include <map>
#include <unordered_set>

#include "../include/robuf.h"
#include "../include/utils.h"
#include "../include/events.h"
#include "../include/config_file.h"
#include "../include/dlinkedlist.hpp"
#include "../include/socks5.h"

namespace KProxyClient {

class Server;
class ConnectionProxy;
class ClientProxy;
class RelayConnection;

enum ConnectionState {
    CONNECTION_INIT = 0,
    CONNECTION_OPEN,
    CONNECTION_CLOSING,
    CONNECTION_CLOSED
};

/**
 * @class Socks5Auth */
class Socks5Auth //{
{
    public:
        using finish_cb = void (*)(int status, Socks5Auth* self_ref, const std::string& addr, uint16_t port, uv_tcp_t* tcp, void* data);
        enum SOCKS5_STAGE {
            SOCKS5_ERROR = 0,
            SOCKS5_INIT,
            SOCKS5_ID,
            SOCKS5_METHOD,
            SOCKS5_FINISH
        };

    private:
        finish_cb m_cb;
        bool m_client_read_start;

        SOCKS5_STAGE  m_state;
        uv_loop_t*    mp_loop;
        uv_tcp_t*     mp_client;
        ClientConfig* mp_config;
        ROBuf         m_remain;
        void*         m_data;

        std::string   m_servername;
        uint16_t      m_port;

        void dispatch_data(ROBuf buf);

        static void read_callback(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);

        static void write_callback_hello(uv_write_t* req, int status);
        static void write_callback_id(uv_write_t* req, int status);
        static void write_callback_reply(uv_write_t* req, int status);

        void return_to_server();

        void __send_selection_method(socks5_authentication_method method);
        void __send_auth_status(uint8_t status);
        void __send_reply(uint8_t reply);

    public:
        Socks5Auth(uv_tcp_t* client, ClientConfig* config, finish_cb cb, void* data);
}; //}

/**
 * @class Server a socks5 proxy server */
class Server: EventEmitter //{
{
    private:
        uint32_t bind_addr;
        uint16_t bind_port;

        // uv data structures
        uv_loop_t* mp_uv_loop;
        uv_tcp_t*  mp_uv_tcp;

        ClientConfig* m_config;

        std::unordered_set<Socks5Auth*> m_auths;
        std::unordered_map<RelayConnection*, bool> m_relay;

        ConnectionState m_state;

        static void on_connection(uv_stream_t* stream, int status);

        static void on_authentication(int status, Socks5Auth* self_ref, 
                const std::string& addr, uint16_t port, 
                uv_tcp_t* con, void* data);

        static void on_config_load(int error, void* data);
        int __listen();

        void dispath_base_on_addr(const std::string&, uint16_t, uv_tcp_t*);
        void dispath_bypass(const std::string&, uint16_t, uv_tcp_t*);

    public:
        Server(const Server&) = delete;
        Server(Server&& s) = delete;
        Server& operator=(const Server&) = delete;
        Server& operator=(const Server&&) = delete;

        Server(uv_loop_t* loop, const std::string& config_file);

        int listen();
        void close();

        void close_relay(RelayConnection* relay);
}; //}

/**
 * @class represent a socks5 connection */
class ClientConnection: EventEmitter //{
{
    private:
        uv_tcp_t* mp_tcp;
        Server* mp_server;
        ConnectionProxy* mp_proxy;
        uint8_t m_id;

        ConnectionState m_state;

        void socks5_authenticate();

    public:
        ClientConnection(const sockaddr* addr, Server* p);

        ClientConnection(const ClientConnection&) = delete;
        ClientConnection(ClientConnection&& a) = delete; 
        ClientConnection& operator=(ClientConnection&) = delete;
        ClientConnection& operator=(ClientConnection&&) = delete;

        int write(ROBuf buf);
        void close();
}; //}

/**
 * @class ConnectionProxy Multiplexing a single ssl/tls connection to multiple tcp connection */
class ConnectionProxy: EventEmitter //{
{
    private:
        std::map<uint8_t, ClientProxy*> m_map;
        Server* m_server;
        uv_tcp_t* m_connection;

        ConnectionState m_state;

        void tsl_handshake();
        void client_autheticate();

    public:
        ConnectionProxy(Server* server);

        ConnectionProxy(const ConnectionProxy&) = delete;
        ConnectionProxy(ConnectionProxy&& a) = delete;
        ConnectionProxy& operator=(ConnectionProxy&&);
        ConnectionProxy& operator=(const ConnectionProxy&) = delete;

        int write(uint8_t id,ROBuf buf);
        void close();

        inline size_t getConnectionNumbers() {return this->m_map.size();}
}; //}

/**
 * @class RelayConnection direct relay connection between webserver and client */
class RelayConnection: EventEmitter //{
{
    private:
        uv_loop_t* mp_loop;
        uv_tcp_t*  mp_tcp_client;
        uv_tcp_t*  mp_tcp_server;

        bool m_client_start_read;
        bool m_server_start_read;

        std::string m_server;
        uint16_t    m_port; // network bytes order -- big-endian

        size_t m_in_buffer;
        size_t m_out_buffer;

        Server* m_kserver;
        bool m_error;

        static void getaddrinfo_cb(uv_getaddrinfo_t* req, int status, struct addrinfo* res);
        static void connect_server_cb(uv_connect_t* req, int status);

        static void client_read_cb(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
        static void server_read_cb(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);

        static void server_write_cb(uv_write_t* req, int status);
        static void client_write_cb(uv_write_t* req, int status);

        void __connect_to(const sockaddr* addr);
        void __start_relay();

        void __relay_client_to_server();
        void __relay_server_to_client();

    public:
        RelayConnection(Server* kserver, uv_loop_t* loop, uv_tcp_t* tcp_client, const std::string& server, uint16_t port);
        void run();
        void close();
}; //}

}


#include "../include/kserver.h"
#include "../include/kserver_multiplexing.h"
#include "../include/callback_data.h"
#include "../include/ObjectFactory.hpp"
#include "../include/config.h"

NS_PROXY_SERVER_START

/** constructor of ClientConnectionProxy */
ClientConnectionProxy::ClientConnectionProxy(Server* server): m_remains() //{
{
    __logger->debug("call %s", FUNCNAME);
    this->mp_server = server;
    this->in_authentication = true;
} //}

/** [static] As name suggested
 *  callback to EBStreamAbstraction::_write(...) in @ClientConnectionProxy::_write(...) */
void ClientConnectionProxy::write_to_user_stream_cb(EventEmitter* obj, ROBuf buf, int status, void* ptr)//{
{
    __logger->debug("call %s", FUNCNAME);
    CBD::ClientConnectionProxy$_write$_write* msg = 
        dynamic_cast<decltype(msg)>(static_cast<CBD::SBase*>(ptr));
    assert(msg);
    auto _this = msg->_this;
    auto cb    = msg->_cb;
    auto data  = msg->_data;
    auto run   = msg->CanRun();
    delete msg;
    if(!run) {
        cb(ROBuf(), data, -1, false);
    } else {
        _this->remove_callback(msg);
        cb(buf, data, status, true);
    }
    return;
} //}

/** dispatch data read from user tcp connection base on current state */
void ClientConnectionProxy::dispatch_new_data(ROBuf buf) //{
{
    __logger->debug("call %s", FUNCNAME);
    if(this->in_authentication) {
        this->dispatch_authentication_data(buf);
    } else {
        this->dispatch_packet_data(buf);
    }
} //}
void ClientConnectionProxy::dispatch_authentication_data(ROBuf buf) //{
{
    __logger->debug("call %s", FUNCNAME);
    assert(this->in_authentication && "????????");
    this->m_remains = this->m_remains + buf;
    if(this->m_remains.size() < 2) return;
    uint8_t username_len = this->m_remains.base()[0];
    if(this->m_remains.size() < username_len + 3) return;
    uint8_t password_len = this->m_remains.base()[username_len + 1];
    if(this->m_remains.size() < username_len + password_len + 2) return;

    char* username = (char*)malloc(username_len + 1);
    char* password = (char*)malloc(password_len + 1);
    memcpy(username, this->m_remains.base() + 1, username_len);
    memcpy(password, this->m_remains.base() + 2 + username_len, password_len);
    username[username_len] = '\0';
    password[password_len] = '\0';
    __logger->info("Authentication: [username: %s, password: %s]", username, password);

    bool authenticate_pass = this->mp_server->m_config->validateUser(username, password);

    free(username);
    free(password);

    this->in_authentication = false;
    if(authenticate_pass) {
        __logger->info("Authentication success");
        this->m_remains = this->m_remains + (username_len + password_len + 2);
        this->_write(ROBuf((char*)"\xff\x00", 2), nullptr, nullptr);
    } else {
        __logger->warn("Authentication fail");
        this->_write(ROBuf((char*)"\x00\x00", 2), nullptr, nullptr);
        this->close();
    }
} //}
void ClientConnectionProxy::dispatch_packet_data(ROBuf buf) //{
{
    __logger->debug("call %s", FUNCNAME);
    auto result = decode_all_packet(this->m_remains, buf);
    bool noerror;
    std::vector<std::tuple<ROBuf, PACKET_OPCODE, uint8_t>> x;
    std::tie(noerror, x, this->m_remains) = result;

    if(!noerror) {
        // packet error, give up TODO
        this->close();
        return;
    }

    for(auto& z: x) {
        ROBuf a;
        PACKET_OPCODE b;
        uint8_t id;
        std::tie(a, b, id) = z;

        switch(b) {
            case PACKET_OP_NEW:
                this->dispatch_new(id, a);
                break;
            case PACKET_OP_REG:
                if(this->m_map.find(id) != this->m_map.end()) {
                    this->dispatch_reg(id, a);
                } else {
                    this->close();
                    __logger->warn("ClientConnectionProxy: unexpected id REG");
                }
                break;
            case PACKET_OP_CLOSE:
                if(this->m_map.find(id) != this->m_map.end())
                    this->dispatch_close(id, a);
                else
                    __logger->warn("ClientConnectionProxy: unexpected id CLOSE");
                break;
            case PACKET_OP_RESERVED:
            default:
                logger->warn("unexcepted packet which use reserved opcode");
                this->close();
                break;
        }
    }
} //}

/* dispatch data base on opcode in packet
 * the request has format like |    address:port   | 
 *                             |      buf.size()   | */
void ClientConnectionProxy::dispatch_new(uint8_t id, ROBuf buf) //{
{
    __logger->debug("call %s", FUNCNAME);
    assert(id < 1 << 6);
    if(this->m_map.find(id) != this->m_map.end()) {
        logger->warn("ClientConnectionProxy::dispatch_new(): bad connection id");
        this->close();
        return;
    }

    if(buf.size() < 4) {
        logger->warn("ClientConnectionProxy::dispatch_new(): bad new connection request");
        this->close();
        return;
    }

    uint16_t port;
    memcpy(&port, buf.base() + buf.size() - 2, 2);
    port = k_ntohs(port);

    char* addr = (char*)malloc(buf.size() - 2 + 1);
    memcpy(addr, buf.base(), buf.size() - 2);
    addr[buf.size() - 2] = '\0';

    this->m_wait_connect.insert(id);
    auto newcon = Factory::ToNetAbstraction(this, this->newUnderlyStream(), id, std::string(addr), port);
    this->m_map[id] = newcon;

    free(addr);

    return;
} //}
void ClientConnectionProxy::dispatch_reg(uint8_t id, ROBuf buf) //{
{
    __logger->debug("call %s", FUNCNAME);
    assert(this->m_map.find(id) != this->m_map.end());
    this->m_map[id]->PushData(buf);
} //}
void ClientConnectionProxy::dispatch_close(uint8_t id, ROBuf buf) //{
{
    __logger->debug("call %s", FUNCNAME);
    assert(this->m_map.find(id) != this->m_map.end());
    ToNetAbstraction* s = this->m_map[id];
    if(buf.size() > 0) {
        char* reason = (char*)malloc(buf.size() + 1);
        memcpy(reason, buf.base(), buf.size());
        reason[buf.size()] = '\0';
        logger->info("ClientConnectionProxy: close connection because %s", reason);
        free(reason);
    }
    s->close();
} //}

/** user authentication routine  */
void ClientConnectionProxy::start_relay() //{
{
    __logger->debug("call %s", FUNCNAME);
    return;
} //}

/** wrapper of uv_write() */
int ClientConnectionProxy::_write(ROBuf buf, WriteCallback cb, void* data) //{
{
    __logger->debug("call %s", FUNCNAME);
    auto ptr = new CBD::ClientConnectionProxy$_write$_write(this, cb, data);
    this->add_callback(ptr);
    this->EBStreamAbstraction::_write(buf, ClientConnectionProxy::write_to_user_stream_cb, ptr);
    return 0;
} //}

/** implement abstract method */
void ClientConnectionProxy::read_callback(ROBuf buf, int status) //{
{
    if(status < 0) {
        this->close();
        return;
    }
    this->dispatch_new_data(buf);
} //}

/** send a packet to close a connection */
int ClientConnectionProxy::close_connection(uint8_t id) //{
{
    __logger->debug("call %s", FUNCNAME);
    assert(id < SINGLE_PROXY_MAX_CONNECTION);
    assert(this->m_map.find(id) != this->m_map.end());
    char reason[] = "close";
    auto b = encode_packet(PACKET_OPCODE::PACKET_OP_CLOSE, id, ROBuf(reason, strlen(reason)));
    int ret = this->_write(b, nullptr, nullptr);
    this->m_map.erase(this->m_map.find(id));
    return ret;
} //}
/** send a packet to inform state of new connection, accept new connection */
int ClientConnectionProxy::accept_connection(uint8_t id) //{
{
    __logger->debug("call %s", FUNCNAME);
    assert(id < SINGLE_PROXY_MAX_CONNECTION);
    assert(this->m_map.find(id) != this->m_map.end());
    char reason = NEW_CONNECTION_REPLY::SUCCESS;
    auto b = encode_packet(PACKET_OPCODE::PACKET_OP_CONNECT, id, ROBuf(&reason, 1));
    return this->_write(b, nullptr, nullptr);
} //}
/** send a packet to inform state of new connection, reject new connection by #reason */
int ClientConnectionProxy::reject_connection(uint8_t id, NEW_CONNECTION_REPLY reason) //{
{
    __logger->debug("call %s", FUNCNAME);
    assert(id < SINGLE_PROXY_MAX_CONNECTION);
    assert(this->m_map.find(id) != this->m_map.end());
    auto b = encode_packet(PACKET_OPCODE::PACKET_OP_REJECT, id, ROBuf(&reason, 1));
    return this->_write(b, nullptr, nullptr);
} //}

/** delete a conneciton */
void ClientConnectionProxy::remove_connection(uint8_t id, ToNetAbstraction* con) //{
{
    __logger->debug("call %s", FUNCNAME);
    assert(this->m_map.find(id) != this->m_map.end());
    assert(this->m_map[id] == con);
    this->m_map.erase(this->m_map.find(id));
    delete con;
} //}

/** close this object */
void ClientConnectionProxy::close() //{
{
    auto m = this->m_map;
    for(auto& x: m)
        x.second->close();

    this->mp_server->remove_proxy(this);
} //}

/** wrapper of _write() FIXME */
int ClientConnectionProxy::write(uint8_t id, ROBuf buf, WriteCallback cb, void* data) //{
{
    __logger->debug("call %s", FUNCNAME);
    auto m = encode_packet(PACKET_OPCODE::PACKET_OP_REG, id, buf);
    return this->_write(m, cb, data);
} //}

NS_PROXY_SERVER_END
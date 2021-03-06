#include "../include/StreamProvider_KProxyMultiplexer.h"
#include "../include/config.h"
#include "../include/kpacket.h"

#include <random>
static std::default_random_engine random_engine;
static std::uniform_int_distribution<size_t> random_dist;


#define DEBUG(all...) __logger->debug(all)


KProxyMultiplexerStreamProvider::KProxyMultiplexerStreamProvider(): m_remain(), m_allocator(), m_client_wait_connection(), m_next_id(SINGLE_MULTIPLEXER_MAX_CONNECTION) {}

struct __multiplexer_state: public CallbackPointer {
    KProxyMultiplexerStreamProvider* _this;
    inline __multiplexer_state(decltype(_this)_this): _this(_this) {}
};

KProxyMultiplexerStreamProvider::StreamId 
    KProxyMultiplexerStreamProvider::createStreamID(bool* runlock, void (*freef)(void*), EBStreamAbstraction* stream) //{
{
    DEBUG("call %s", FUNCNAME);
    if(this->m_allocator.size() == SINGLE_MULTIPLEXER_MAX_CONNECTION)
        return StreamId(nullptr);

    uint8_t i = 0;
    if(this->m_next_id != SINGLE_MULTIPLEXER_MAX_CONNECTION) {
        i = this->m_next_id;
        this->m_next_id = SINGLE_MULTIPLEXER_MAX_CONNECTION;
    } else {
        std::vector<uint8_t> all__;
        for(;i<SINGLE_MULTIPLEXER_MAX_CONNECTION;i++) {
            if(this->m_allocator.find(i) == this->m_allocator.end()) {
                all__.push_back(i);
            }
        }
        assert(all__.size() > 0 && "what ???");
        size_t k = random_dist(random_engine) % all__.size();
        i = all__[k];
    }

    auto id = StreamId(new __KMStreamID(runlock, freef, i, stream));
    this->m_allocator[i] = id;
    return id;
} //}


struct __multiplexer_writecb_state: public __multiplexer_state {
//    KProxyMultiplexerStreamProvider* _this;  STUPID ME
    KProxyMultiplexerStreamProvider::WriteCallback _cb;
    void* _data;
    inline __multiplexer_writecb_state(KProxyMultiplexerStreamProvider* _this, decltype(_cb) cb, void* data): 
        __multiplexer_state(_this), _cb(cb), _data(data) {}
};
void KProxyMultiplexerStreamProvider::write_header(ROBuf header) //{
{
    DEBUG("call %s", FUNCNAME);
    auto ptr1 = new __multiplexer_state(this);
    this->add_callback(ptr1);
    this->prm_write(header, header_write_callback, ptr1);
} //}
void KProxyMultiplexerStreamProvider::write_buffer(ROBuf buf, WriteCallback cb, void* data) //{
{
    DEBUG("call %s", FUNCNAME);
    auto ptr2 = new __multiplexer_writecb_state(this, cb, data);
    this->add_callback(ptr2);
    this->prm_write(buf, buffer_write_callback, ptr2);
} //}


#define GETKID() \
    DEBUG("call %s", FUNCNAME); \
    const __KMStreamID* kid = dynamic_cast<decltype(kid)>(id.get()); \
    assert(kid); \
    assert(this->m_allocator.find(kid->m_id)->second == id)

#define GETKID__() \
    DEBUG("call %s", FUNCNAME); \
    __KMStreamID* kid = dynamic_cast<decltype(kid)>(id.get()); \
    assert(kid); \
    assert(this->m_allocator.find(kid->m_id)->second == id)


struct __new_connection_write_state: public CallbackPointer {
    KProxyMultiplexerStreamProvider* _this;
    uint8_t                          _id;
    __new_connection_write_state(decltype(_this) _this, uint8_t id):
        _this(_this), _id(id) {}
};
using __new_connection_timeout_state = __new_connection_write_state;
void KProxyMultiplexerStreamProvider::connect(StreamId id, struct sockaddr* addr, ConnectCallback cb, void* data) //{
{
    GETKID();
    auto s = k_sockaddr_to_str(addr);
    if(s.first.size() == 0) {
        cb(-1, data);
        return;
    }
    this->connect(id, s.first, s.second, cb, data);
}//}
void KProxyMultiplexerStreamProvider::connect(StreamId id, uint32_t ipv4,    uint16_t port, ConnectCallback cb, void* data) //{
{
    static char addr_save[100];
    GETKID();
    uint32_t n_addr = k_htonl(ipv4);
    if(!k_inet_ntop(AF_INET, &n_addr, addr_save, sizeof(addr_save))) {
        cb(-1, data);
        assert(false);
        return;
    }
    this->connect(id, addr_save, port, cb, data);
}//}
void KProxyMultiplexerStreamProvider::connect(StreamId id, uint8_t ipv6[16], uint16_t port, ConnectCallback cb, void* data) //{
{
    static char addr_save[100];
    GETKID();
    if(!k_inet_ntop(AF_INET, ipv6, addr_save, sizeof(addr_save))) {
        cb(-1, data);
        assert(false);
        return;
    }
    this->connect(id, addr_save, port, cb, data);
}//}
void KProxyMultiplexerStreamProvider::connect(StreamId id, const std::string& addr, uint16_t port, ConnectCallback cb, void* data) //{
{
    GETKID();
    ROBuf buf = ROBuf(addr.size() + 2);
    memcpy(buf.__base(), addr.c_str(), addr.size());
    *(uint16_t*)(&buf.__base()[addr.size()]) = k_htons(port);
    ROBuf send_buf = encode_packet(PACKET_OP_CREATE_CONNECTION, kid->m_id, buf);
    DEBUG("create new connection, opcode: %s, id: %d", packet_opcode_name(PACKET_OP_CREATE_CONNECTION), kid->m_id);

    assert(this->m_client_wait_connection.find(kid->m_id) == this->m_client_wait_connection.end());
    this->m_client_wait_connection[kid->m_id] = __connect_state {cb, data};

    auto write_ptr = new __new_connection_write_state(this, kid->m_id);
    this->add_callback(write_ptr);
    this->write_buffer(send_buf, new_connection_write_callback, write_ptr);

    auto timeout_ptr = new __new_connection_timeout_state(this, kid->m_id);
    this->add_callback(timeout_ptr);

    this->timeout(new_connection_timeout_callback, timeout_ptr, NEW_CONNECTION_TIMEOUT);
}//}
/** [static] */
void KProxyMultiplexerStreamProvider::new_connection_write_callback(ROBuf buf, int status, void* data) //{
{
    DEBUG("call %s", FUNCNAME);
    __new_connection_write_state* msg = 
        dynamic_cast<decltype(msg)>(static_cast<CallbackPointer*>(data));
    assert(msg);

    auto _this = msg->_this;
    auto id = msg->_id;
    auto run = msg->CanRun();
    delete msg;

    if(!run) return;
    _this->remove_callback(msg);

    if(status < 0) {
        if(_this->m_client_wait_connection.find(id) == _this->m_client_wait_connection.end())
            return;
        auto state = _this->m_client_wait_connection[id];
        _this->m_client_wait_connection.erase(_this->m_client_wait_connection.find(id));
        state.cb(-1, state.data);
        return;
    }
} //}
/** [static] */
void KProxyMultiplexerStreamProvider::new_connection_timeout_callback(void* data) //{
{
    DEBUG("call %s", FUNCNAME);
    __new_connection_write_state* msg = 
        dynamic_cast<decltype(msg)>(static_cast<CallbackPointer*>(data));
    assert(msg);

    auto _this = msg->_this;
    auto id = msg->_id;
    auto run = msg->CanRun();
    delete msg;

    if(!run) return;
    _this->remove_callback(msg);

    if(_this->m_client_wait_connection.find(id) == _this->m_client_wait_connection.end())
        return;

    auto state = _this->m_client_wait_connection[id];
    auto _cb = state.cb;
    auto _data = state.data;
    _this->m_client_wait_connection.erase(_this->m_client_wait_connection.find(id));
    _cb(-1, _data);
    return;
} //}

void KProxyMultiplexerStreamProvider::getaddrinfo(StreamId id, const std::string& addr, GetAddrInfoCallback cb, void* data) //{
{
    GETKID();
    assert(false && "not implement");
}//}
void KProxyMultiplexerStreamProvider::getaddrinfoIPv4(StreamId id, const std::string& addr, GetAddrInfoIPv4Callback cb, void* data) //{
{
    GETKID();
    assert(false && "not implement");
}//}
void KProxyMultiplexerStreamProvider::getaddrinfoIPv6(StreamId id, const std::string& addr, GetAddrInfoIPv6Callback cb, void* data) //{
{
    GETKID();
    assert(false && "not implement");
}//}

void KProxyMultiplexerStreamProvider::write(StreamId id, ROBuf buf, WriteCallback cb, void* data) //{
{
    GETKID();
    ROBuf header = encode_packet_header(PACKET_OPCODE::PACKET_OP_WRITE, kid->m_id, buf.size());
    this->write_header(header);
    this->write_buffer(buf, cb, data);
}//}
/** [static] */
void KProxyMultiplexerStreamProvider::header_write_callback(ROBuf buf, int status, void* data) //{
{
    DEBUG("call %s", FUNCNAME);
    __multiplexer_state* msg = 
        dynamic_cast<decltype(msg)>(static_cast<CallbackPointer*>(data));
    assert(msg);
    auto _this = msg->_this;
    auto run   = msg->CanRun();
    delete msg;

    if(!run) return;
    _this->remove_callback(msg);

    if(status < 0) _this->prm_error_handle();
} //}
/** [static] */
void KProxyMultiplexerStreamProvider::buffer_write_callback(ROBuf buf, int status, void* data) //{
{
    DEBUG("call %s", FUNCNAME);
    __multiplexer_writecb_state* msg = 
        dynamic_cast<decltype(msg)>(static_cast<CallbackPointer*>(data));
    assert(msg);
    auto _this = msg->_this;
    auto _cb   = msg->_cb;
    auto _data = msg->_data;
    auto run   = msg->CanRun();
    delete msg;

    if(!run) {
        if(_cb != nullptr) _cb(ROBuf(), -1, _data);
        return;
    }
    _this->remove_callback(msg);

    if(status < 0) {
        if(_cb != nullptr) _cb(ROBuf(), -1, _data);
        _this->prm_error_handle();
        return;
    }

    if(_cb != nullptr) _cb(buf, 0, _data);
} //}

void KProxyMultiplexerStreamProvider::send_zero_packet(StreamId id, uint8_t opcode) //{
{
    GETKID();
    DEBUG("send packet with {opcode: %s, description: %s, id: %d}", 
                    packet_opcode_name((PACKET_OPCODE)opcode), packet_opcode_description((PACKET_OPCODE)opcode), kid->m_id);
    ROBuf header = encode_packet_header((PACKET_OPCODE)opcode, kid->m_id, 0);
    this->write_header(header);
}//}

void KProxyMultiplexerStreamProvider::startRead(StreamId id) //{
{
    this->send_zero_packet(id, PACKET_OPCODE::PACKET_OP_START_READ);
}//}
void KProxyMultiplexerStreamProvider::stopRead (StreamId id) //{
{
    this->send_zero_packet(id, PACKET_OPCODE::PACKET_OP_STOP_READ);
}//}
void KProxyMultiplexerStreamProvider::closeOtherEnd(StreamId id) //{
{
    this->send_zero_packet(id, PACKET_OPCODE::PACKET_OP_CLOSE_CONNECTION);
}//}

void KProxyMultiplexerStreamProvider::accept_connection(StreamId id) //{
{
    this->send_zero_packet(id, PACKET_OPCODE::PACKET_OP_ACCEPT_CONNECTION);
}//}
void KProxyMultiplexerStreamProvider::reject_connection(StreamId id, uint8_t reason) //{
{
    this->send_zero_packet(id, PACKET_OPCODE::PACKET_OP_REJECT_CONNECTION);
}//}

void KProxyMultiplexerStreamProvider::end(StreamId id, EndCallback cb, void* data) //{
{
    this->send_zero_packet(id, PACKET_OPCODE::PACKET_OP_END_CONNECTION);
}//}

void KProxyMultiplexerStreamProvider::closeStream(StreamId id) //{
{
    GETKID();
    this->finish(kid->m_stream);

    if(this->m_client_wait_connection.find(kid->m_id) != this->m_client_wait_connection.end()) { // client side
        auto state = this->m_client_wait_connection[kid->m_id];
        this->m_client_wait_connection.erase(this->m_client_wait_connection.find(kid->m_id));
        state.cb(-1, state.data);
    }
    this->closeOtherEnd(id);
    this->m_allocator.erase(this->m_allocator.find(kid->m_id));
    if(this->m_server_wait_connection.find(kid->m_id) != this->m_server_wait_connection.end())
        this->CreateConnectionFail(id, 0x01);
}//}
void KProxyMultiplexerStreamProvider::timeout(TimeoutCallback cb, void* data, int time_ms) //{
{
    DEBUG("call %s", FUNCNAME);
    this->prm_timeout(cb, data, time_ms);
}//}


void KProxyMultiplexerStreamProvider::registerReadCallback(StreamId id, RegisterReadCallback cb) //{
{
    DEBUG("call %s", FUNCNAME);
    __KMStreamID* kid = dynamic_cast<decltype(kid)>(id.get()); assert(kid);
    assert(this->m_allocator.find(kid->m_id) != this->m_allocator.end());
    assert(this->m_allocator.find(kid->m_id)->second == id);
    kid->m_read_callback = cb;
} //}
void KProxyMultiplexerStreamProvider::registerErrorCallback(StreamId id, RegisterErrorCallback cb) //{
{
    DEBUG("call %s", FUNCNAME);
    __KMStreamID* kid = dynamic_cast<decltype(kid)>(id.get()); assert(kid);
    assert(this->m_allocator.find(kid->m_id)->second == id);
    kid->m_error_callback = cb;
} //}
void KProxyMultiplexerStreamProvider::registerCloseCallback(StreamId id, RegisterCloseCallback cb) //{
{
    DEBUG("call %s", FUNCNAME);
    __KMStreamID* kid = dynamic_cast<decltype(kid)>(id.get()); assert(kid);
    assert(this->m_allocator.find(kid->m_id)->second == id);
    kid->m_close_callback = cb;
} //}
void KProxyMultiplexerStreamProvider::registerEndCallback(StreamId id, RegisterEndCallback cb) //{
{
    DEBUG("call %s", FUNCNAME);
    __KMStreamID* kid = dynamic_cast<decltype(kid)>(id.get()); assert(kid);
    assert(this->m_allocator.find(kid->m_id)->second == id);
    kid->m_end_callback = cb;
} //}
void KProxyMultiplexerStreamProvider::registerShouldStartWriteCallback(StreamId id, RegisterShouldStartWriteCallback cb) //{
{
    DEBUG("call %s", FUNCNAME);
    __KMStreamID* kid = dynamic_cast<decltype(kid)>(id.get()); assert(kid);
    assert(this->m_allocator.find(kid->m_id)->second == id);
    kid->m_start_read_callback = cb;
} //}
void KProxyMultiplexerStreamProvider::registerShouldStopWriteCallback(StreamId id, RegisterShouldStopWriteCallback cb) //{
{
    DEBUG("call %s", FUNCNAME);
    __KMStreamID* kid = dynamic_cast<decltype(kid)>(id.get()); assert(kid);
    assert(this->m_allocator.find(kid->m_id)->second == id);
    kid->m_stop_read_callback = cb;
} //}


void KProxyMultiplexerStreamProvider::dispatch_data(ROBuf buf) //{
{
    DEBUG("call %s", FUNCNAME);
    std::tuple<bool, std::vector<std::tuple<ROBuf, PACKET_OPCODE, uint8_t>>, ROBuf> mm = 
        decode_all_packet(this->m_remain, buf);
    bool noerror;
    std::vector<std::tuple<ROBuf, PACKET_OPCODE, uint8_t>> packets;
    std::tie(noerror, packets, this->m_remain) = mm;

    if(noerror == false) {
        __logger->warn("packet error");
        this->prm_error_handle();
        return;
    }

    auto checker = new ObjectChecker();
    this->SetChecker(checker);

    for(auto& p: packets) {
        ROBuf frame;
        PACKET_OPCODE opcode;
        uint8_t id;
        std::tie(frame, opcode, id) = p;

        DEBUG("multiplexer: 0x%lx\nopcode: %s -> %s\nid: %d", this, 
                        packet_opcode_name(opcode), packet_opcode_description(opcode), id);

        if(!checker->exist()) {
            __logger->warn("object checker failure");
            break;
        }

        if(this->m_allocator.find(id) == this->m_allocator.end()) {
            if(opcode == PACKET_OP_WRITE          || 
               opcode == PACKET_OP_END_CONNECTION ||
               opcode == PACKET_OP_START_READ     ||
               opcode == PACKET_OP_STOP_READ) {
                __logger->warn("bad id");
                continue;
            }
        }

        __KMStreamID* id_info = nullptr;
        if(this->m_allocator.find(id) != this->m_allocator.end()) {
            id_info = dynamic_cast<decltype(id_info)>(this->m_allocator[id].get());
            assert(id_info);
        }

        switch (opcode) {
            case PACKET_OP_WRITE:
                if(id_info == nullptr) break;
                id_info->m_read_callback(id_info->m_stream, frame);
                break;
            case PACKET_OP_END_CONNECTION:
                if(id_info == nullptr) break;
                id_info->m_end_callback(id_info->m_stream);
                break;
            case PACKET_OP_START_READ:
                if(id_info == nullptr) break;
                id_info->m_start_read_callback(id_info->m_stream);
                break;
            case PACKET_OP_STOP_READ:
                if(id_info == nullptr) break;
                id_info->m_stop_read_callback(id_info->m_stream);
                break;
            case PACKET_OP_CLOSE_CONNECTION:
                if(id_info == nullptr) break;
                id_info->m_close_callback(id_info->m_stream);
                break;
            case PACKET_OP_ACCEPT_CONNECTION:
                if(this->m_client_wait_connection.find(id) == this->m_client_wait_connection.end()) {
                    __logger->warn("KProxyMultiplexerStreamProvider： unexpected ACCEPT_CONNECTION");
                } else {
                    auto ff = this->m_client_wait_connection.find(id);
                    auto state = ff->second;
                    this->m_client_wait_connection.erase(ff);
                    state.cb(0, state.data);
                }
                break;
            case PACKET_OP_REJECT_CONNECTION:
                if(this->m_client_wait_connection.find(id) == this->m_client_wait_connection.end()) {
                    __logger->warn("KProxyMultiplexerStreamProvider： unexpected REJECT_CONNECTION");
                } else {
                    auto ff = this->m_client_wait_connection.find(id);
                    auto state = ff->second;
                    this->m_client_wait_connection.erase(ff);
                    state.cb(-1, state.data);
                }
                break;
            case PACKET_OP_CREATE_CONNECTION:
                this->create_new_connection(frame ,id);
                break;
            case PACKET_OP_RESERVED:
            default:
                __logger->warn("KProxyMultiplexerStreamProvider： unexpected OPCODE");
                break;
        }
    }

    if(checker->exist()) this->cleanChecker(checker);
    delete checker;
} //}

void KProxyMultiplexerStreamProvider::prm_read_callback(ROBuf buf) //{
{
    DEBUG("call %s", FUNCNAME);
    this->dispatch_data(buf);
} //}

void KProxyMultiplexerStreamProvider::lock_nextID_with(typename decltype(m_allocator)::key_type new_id) //{
{
    DEBUG("call %s", FUNCNAME);
    assert(this->m_allocator.find(new_id) == this->m_allocator.end());
    assert(this->m_next_id == SINGLE_MULTIPLEXER_MAX_CONNECTION);
    this->m_next_id = new_id;
} //}

#include "ObjectFactory.h"
void KProxyMultiplexerStreamProvider::create_new_connection(ROBuf buf, uint8_t nid) //{
{
    DEBUG("call %s", FUNCNAME);
    if(this->m_allocator.find(nid) != this->m_allocator.end()) {
        __logger->warn("bad NEW_CONNECTION opcode, which already allocated to other stream, %d, total=%d", 
                        nid, this->m_allocator.size());
        this->reject_connection(this->m_allocator[nid], 0x02); // TODO
        __KMStreamID* id_info = dynamic_cast<decltype(id_info)>(this->m_allocator[nid].get());
        assert(id_info);
        id_info->m_close_callback(id_info->m_stream);
        return;
    }

    if(buf.size() < 4) {
        logger->warn("bad new connection request");
        this->reject_connection(this->m_allocator[nid], 0x03); // TODO
        return;
    }

    this->lock_nextID_with(nid);
    EBStreamObject* proxy_obj = Factory::createKProxyMultiplexerStreamObject(PROXY_MAX_BUFFER_SIZE, this);

    uint16_t port;
    memcpy(&port, buf.base() + buf.size() - 2, 2);
    port = k_ntohs(port);

    char* addr = (char*)malloc(buf.size() - 2 + 1);
    memcpy(addr, buf.base(), buf.size() - 2);
    addr[buf.size() - 2] = '\0';

    this->m_server_wait_connection.insert(nid);
    this->CreateNewConnection(proxy_obj, this->m_allocator[nid], addr, port);

    free(addr);
    return;
} //}

void KProxyMultiplexerStreamProvider::changeOwner(StreamId id, EBStreamAbstraction* obj) //{
{
    GETKID__();
    kid->m_stream = obj;
} //}

void KProxyMultiplexerStreamProvider::CreateConnectionSuccess(StreamId id) //{
{
    GETKID();
    assert(this->m_server_wait_connection.find(kid->m_id) != this->m_server_wait_connection.end());
    this->m_server_wait_connection.erase(this->m_server_wait_connection.find(kid->m_id));
    this->accept_connection(id);
} //}
void KProxyMultiplexerStreamProvider::CreateConnectionFail(StreamId id, uint8_t reason) //{
{
    GETKID();
    assert(this->m_server_wait_connection.find(kid->m_id) != this->m_server_wait_connection.end()); // FIXME assert false
    this->m_server_wait_connection.erase(this->m_server_wait_connection.find(kid->m_id));
    this->reject_connection(id, reason);
} //}

bool KProxyMultiplexerStreamProvider::__full() {return this->m_allocator.size() >= (SINGLE_MULTIPLEXER_MAX_CONNECTION / 2);}
uint8_t KProxyMultiplexerStreamProvider::__getConnectionNumbers() {return this->m_allocator.size();}


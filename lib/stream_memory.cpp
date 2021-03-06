#include "../include/stream_memory.h"

#include <iostream>

#include <assert.h>
#include <stdlib.h>


void EBMemStream::_write(ROBuf buf, WriteCallback cb, void* data) //{
{
    if(this->m_state == CONNECTION_STATE::UNINITIAL ||
       this->m_state == CONNECTION_STATE::CLOSED ||
       this->m_state == CONNECTION_STATE::GIVEUP ||
       this->m_state == CONNECTION_STATE::LISTENNING) {
        assert(false && "bad function call");
    }
    std::cout << "write: " << buf << std::endl;
    this->m_write_buffer = this->m_write_buffer + buf;
    cb(buf, 0, data);
    return;
} //}

bool EBMemStream::bind(struct sockaddr* addr) //{
{
    return true;
} //}
bool EBMemStream::listen() //{
{
    assert(this->m_state == CONNECTION_STATE::INITIAL);
    this->m_state = CONNECTION_STATE::LISTENNING;
    return true;
} //}

bool EBMemStream::connect(struct sockaddr* addr, ConnectCallback cb, void* data) //{
{
    assert(this->m_state == CONNECTION_STATE::INITIAL);
    this->m_state = CONNECTION_STATE::CONNECT;
    cb(0, data);
    return true;
} //}
bool EBMemStream::connect(uint32_t ipv4,              uint16_t port, ConnectCallback cb, void* data) //{
{
    return false;
} //}
bool EBMemStream::connect(uint8_t  ipv6[16],          uint16_t port, ConnectCallback cb, void* data) //{
{
    return false;
} //}
bool EBMemStream::connect(const std::string& domname, uint16_t port, ConnectCallback cb, void* data) //{
{
    return false;
} //}
bool EBMemStream::connectINet6(const std::string& domname, uint16_t port, ConnectCallback cb, void* data) //{
{
    return false;
} //}

void EBMemStream::start_read() //{
{
    assert(this->m_stream_read == false);
    this->m_stream_read = true;
} //}
void EBMemStream::stop_read() //{
{
    assert(this->m_stream_read == true);
    this->m_stream_read = false;
} //}
bool EBMemStream::in_read() //{
{
    return this->m_stream_read;
} //}

static void __freeaddrinfo(struct addrinfo* addr) {if(addr != nullptr) free(addr);}
void EBMemStream::getaddrinfo (const char* hostname, GetAddrInfoCallback cb, void* data) //{
{
    cb(nullptr, __freeaddrinfo, 0, data);
} //}
void EBMemStream::getaddrinfoipv4(const char* hostname, GetAddrInfoIPv4Callback cb, void* data) //{
{
    cb(0, -1, data);
} //}
void EBMemStream::getaddrinfoipv6(const char* hostname, GetAddrInfoIPv6Callback cb, void* data) //{
{
    cb(0, -1, data);
} //}

EBStreamAbstraction::UNST EBMemStream::newUnderlyStream() //{
{
    return UNST(new __UnderlyingStreamMem(this->getType()));
} //}
void  EBMemStream::releaseUnderlyStream(UNST) //{
{
    return;
} //}
bool  EBMemStream::accept(UNST stream) //{
{
    assert(false && "unimplemented");
    return true;
} //}

EBMemStream::EBMemStream() {
    this->m_state = CONNECTION_STATE::INITIAL;
    this->m_stream_read = false;
}
EBMemStream::~EBMemStream() {}

void EBMemStream::shutdown(ShutdownCallback cb, void* data) //{
{
    assert(this->m_shutdown == false);
    this->m_shutdown = true;
    cb(0, data);
} //}

EBStreamAbstraction::UNST EBMemStream::transfer()      {
    assert(this->m_state != CONNECTION_STATE::GIVEUP);
    this->m_state = CONNECTION_STATE::GIVEUP;
    return nullptr;
}
void  EBMemStream::regain(UNST)   {
    assert(this->m_state == CONNECTION_STATE::UNINITIAL ||
           this->m_state == CONNECTION_STATE::GIVEUP);
    this->m_state = CONNECTION_STATE::CONNECT;
    return;
}

void  EBMemStream::reply(ROBuf buf) {this->read_callback(buf, 0);}
ROBuf EBMemStream::buffer()         {return this->m_write_buffer;}

bool EBMemStream::hasStreamObject() {return true;}
void EBMemStream::release() {}

StreamType EBMemStream::getType() {return StreamType::Memory;}

bool EBMemStream::timeout(TimeoutCallback cb, void* data, int time_ms) {return false;}


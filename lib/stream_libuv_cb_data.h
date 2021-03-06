#pragma once

#include "../include/stream_libuv.h"


struct UVARG {
    inline virtual ~UVARG() {}
};

struct EBStreamUV$_write$uv_write: public UVARG {
    EBStreamUV*               _this;
    ROBuf                     _buf;
    uv_buf_t*                 _uv_buf;
    EBStreamUV::WriteCallback _cb;
    void*                     _data;
    inline EBStreamUV$_write$uv_write(EBStreamUV* _this, ROBuf buf, uv_buf_t* uv_buf, EBStreamUV::WriteCallback cb, void* data): 
        _this(_this), _buf(buf), _uv_buf(uv_buf), _cb(cb), _data(data) {}
};

struct EBStreamUV$connect$uv_connect: public UVARG {
    EBStreamUV*               _this;
    EBStreamUV::ConnectCallback _cb;
    void*                     _data;
    inline EBStreamUV$connect$uv_connect(EBStreamUV* _this, EBStreamUV::ConnectCallback cb, void* data):
        _this(_this), _cb(cb), _data(data) {}
};

struct EBStreamUV$getaddrinfo$uv_getaddrinfo: public UVARG {
    EBStreamUV*                     _this;
    EBStreamUV::GetAddrInfoCallback _cb;
    void*                           _data;
    inline EBStreamUV$getaddrinfo$uv_getaddrinfo(EBStreamUV* _this, EBStreamUV::GetAddrInfoCallback cb, void* data):
        _this(_this), _cb(cb), _data(data) {}
};


#include "../include/stream_object_libuv.h"
#include "../include/config.h"



EBStreamObject* EBStreamObjectUV::NewStreamObject(UNST stream) //{
{
    return new EBStreamObjectUV(stream, NEW_STREAM_OBJECT_BUFFER_SIZE);
} //}

bool EBStreamObjectUV::accept(EBStreamObject* streamo) //{
{
    EBStreamObjectUV* other = dynamic_cast<decltype(other)>(streamo);
    assert(other);
    return this->accept(other);
} //}


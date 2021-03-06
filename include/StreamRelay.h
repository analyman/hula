#pragma once

#include "StreamObject.h"
#include "object_manager.h"

/** @class relay traffic between two StreamObject */
class StreamRelay: virtual protected CallbackManager //{
{
    private:
        EBStreamObject *mp_stream_a, *mp_stream_b;
        bool m_a_start_read, m_b_start_read;
        bool m_a_end, m_b_end;

        EventEmitter::EventListener m_a_drain_listener_reg, m_b_drain_listener_reg;

        void register_b_listener();
        void register_a_listener();
        static void a_data_listener(EventEmitter* obj, const std::string& event, EventArgs::Base* args);
        static void b_data_listener(EventEmitter* obj, const std::string& event, EventArgs::Base* args);

        static void a_drain_listener(EventEmitter* obj, const std::string& event, EventArgs::Base* args);
        static void b_drain_listener(EventEmitter* obj, const std::string& event, EventArgs::Base* args);

        static void a_shouldstartwrite_listener(EventEmitter* obj, const std::string& event, EventArgs::Base* args);
        static void b_shouldstartwrite_listener(EventEmitter* obj, const std::string& event, EventArgs::Base* args);

        static void a_shouldstopwrite_listener(EventEmitter* obj, const std::string& event, EventArgs::Base* args);
        static void b_shouldstopwrite_listener(EventEmitter* obj, const std::string& event, EventArgs::Base* args);

        static void a_end_listener(EventEmitter* obj, const std::string& event, EventArgs::Base* args);
        static void b_end_listener(EventEmitter* obj, const std::string& event, EventArgs::Base* args);

        static void a_error_listener(EventEmitter* obj, const std::string& event, EventArgs::Base* args);
        static void b_error_listener(EventEmitter* obj, const std::string& event, EventArgs::Base* args);

        int m_a_random = 0;
        int m_b_random = 0;
        void __relay_a_to_b();
        void __relay_b_to_a();
        void __stop_a_to_b();
        void __stop_b_to_a();

        static void __wait_a_start_read(void* data);
        static void __wait_b_start_read(void* data);


    protected:
        virtual void __close() = 0;
        void start_relay();

        void setStreamA(EBStreamObject*);
        void setStreamB(EBStreamObject*);
        EBStreamObject* StreamA();
        EBStreamObject* StreamB();

    public:
        StreamRelay();
        virtual ~StreamRelay();
}; //}


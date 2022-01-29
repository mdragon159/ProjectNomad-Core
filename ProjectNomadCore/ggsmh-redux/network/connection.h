#pragma once

#include "ggsmh-redux/poll.h"
#include "connection_msg.h"
#include "ggsmh-redux/ggponet.h"
#include "ggsmh-redux/ring_buffer.h"
#include "ggsmh-redux/types.h"

#define MAX_CONNECTION_ENDPOINTS     16

static const int MAX_CONNECTION_PACKET_SIZE = 4096;

class Connection : public IPollSink {
public:
    struct Stats {
        int bytes_sent;
        int packets_sent;
        float kbps_sent;
    };

    struct Callbacks {
        virtual ~Callbacks() { }
        virtual void OnMsg(int player_id, ConnectionMsg* msg, int len) = 0;
    };


protected:
    void Log(const char* fmt, ...) {
        char buf[1024];
        size_t offset;
        va_list args;

        strcpy_s(buf, "connection | ");
        offset = strlen(buf);
        va_start(args, fmt);
        vsnprintf(buf + offset, ARRAY_SIZE(buf) - offset - 1, fmt, args);
        buf[ARRAY_SIZE(buf)-1] = '\0';
        ::Log(buf);
        va_end(args);
    }

public:
    Connection() {}
    ~Connection(void) {}
    
    void Init(Poll* poll, Callbacks* callbacks, GGPOConnection* ggpo_connection) {
        _callbacks = callbacks;
        _ggpo_connection = ggpo_connection;
        _poll = poll;
        _poll->RegisterLoop(this);
    }

    void SendTo(char* buffer, int len, int flags, int player_num) {
        _ggpo_connection->send_to(_ggpo_connection->instance, buffer, len, flags, player_num);
    }

    virtual bool OnLoopPoll(void* cookie) {
        uint8          recv_buf[MAX_CONNECTION_PACKET_SIZE];

        for (;;) {
            int player_id = -1;
            int len = _ggpo_connection->receive_from(_ggpo_connection->instance, (char*)recv_buf, MAX_CONNECTION_PACKET_SIZE, 0, &player_id);

            // TODO: handle len == 0... indicates a disconnect.

            if (len == -1) {
                break;
            } else if (len > 0) {
                Log("recvfrom returned (len:%d  from player: %d).\n", len, player_id );
                ConnectionMsg *msg = (ConnectionMsg *)recv_buf;
                _callbacks->OnMsg(player_id, msg, len);
            } 
        }
        return true;
    }

protected:
    // Network transmission information
    GGPOConnection* _ggpo_connection;
    // state management
    Callbacks* _callbacks;
    Poll* _poll;
};
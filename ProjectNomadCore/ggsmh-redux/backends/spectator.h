#pragma once

#include "ggsmh-redux/types.h"
#include "ggsmh-redux/poll.h"
#include "ggsmh-redux/sync.h"
#include "backend.h"
#include "ggsmh-redux/timesync.h"
#include "ggsmh-redux/network/connection_proto.h"

#define SPECTATOR_FRAME_BUFFER_SIZE    64

class SpectatorBackend : public IQuarkBackend, IPollSink, Connection::Callbacks {
public:
    SpectatorBackend(GGPOSessionCallbacks* cb, const char* gamename, GGPOConnection* ggpo_connection, int num_players,
    int input_size, int connection_id) {
        
        _callbacks = *cb;
        _synchronizing = true;

        for (int i = 0; i < ARRAY_SIZE(_inputs); i++) {
            _inputs[i].frame = -1;
        }

        /*
        * Initialize the CONNECTION port
        */
        _connection.Init(&_poll, this, ggpo_connection);

        /*
        * Init the host endpoint
        */
        _host.Init(&_connection, _poll, 0, connection_id, nullptr);
        _host.Synchronize();

        /*
        * Preload the ROM
        */
        _callbacks.begin_game(_callbacks.instance, gamename);
    }
    virtual ~SpectatorBackend() {}


public:
    virtual GGPOErrorCode DoPoll(int timeout) {
        _poll.Pump(0);

        PollConnectionProtocolEvents();
        return GGPO_OK;
    }
    
    virtual GGPOErrorCode AddPlayer(GGPOPlayer* player, GGPOPlayerHandle* handle) {
        return GGPO_ERRORCODE_UNSUPPORTED;
    }
    
    virtual GGPOErrorCode AddLocalInput(GGPOPlayerHandle player, void* values, int size) {
        return GGPO_OK;
    }
    
    virtual GGPOErrorCode SyncInput(void* values, int size, int* disconnect_flags) {
        // Wait until we've started to return inputs.
        if (_synchronizing) {
            return GGPO_ERRORCODE_NOT_SYNCHRONIZED;
        }

        GameInput& input = _inputs[_next_input_to_send % SPECTATOR_FRAME_BUFFER_SIZE];
        if (input.frame < _next_input_to_send) {
            // Haven't received the input from the host yet.  Wait
            return GGPO_ERRORCODE_PREDICTION_THRESHOLD;
        }
        if (input.frame > _next_input_to_send) {
            // The host is way way way far ahead of the spectator.  How'd this
            // happen?  Anyway, the input we need is gone forever.
            return GGPO_ERRORCODE_GENERAL_FAILURE;
        }

        ASSERT(size >= _input_size * _num_players);
        memcpy(values, input.bits, _input_size * _num_players);
        if (disconnect_flags) {
            *disconnect_flags = 0; // xxx: should get them from the host!
        }
        _next_input_to_send++;

        return GGPO_OK;
    }
    
    virtual GGPOErrorCode IncrementFrame() {
        Log("End of frame (%d)...\n", _next_input_to_send - 1);
        DoPoll(0);
        PollConnectionProtocolEvents();

        return GGPO_OK;
    }
    
    virtual GGPOErrorCode DisconnectPlayer(GGPOPlayerHandle handle) { return GGPO_ERRORCODE_UNSUPPORTED; }

    virtual GGPOErrorCode GetNetworkStats(GGPONetworkStats* stats, GGPOPlayerHandle handle) {
        return GGPO_ERRORCODE_UNSUPPORTED;
    }

    virtual GGPOErrorCode SetFrameDelay(GGPOPlayerHandle player, int delay) { return GGPO_ERRORCODE_UNSUPPORTED; }
    virtual GGPOErrorCode SetDisconnectTimeout(int timeout) { return GGPO_ERRORCODE_UNSUPPORTED; }
    virtual GGPOErrorCode SetDisconnectNotifyStart(int timeout) { return GGPO_ERRORCODE_UNSUPPORTED; }

    virtual void OnMsg(int player_id, ConnectionMsg* msg, int len) {
        if (_host.HandlesMsg(player_id, msg)) {
            _host.OnMsg(msg, len);
        }
    }


protected:
    void PollConnectionProtocolEvents() {
        ConnectionProtocol::Event evt;
        while (_host.GetEvent(evt)) {
            OnConnectionProtocolEvent(evt);
        }
    }
    
    void CheckInitialSync();

    void OnConnectionProtocolEvent(ConnectionProtocol::Event& e) {
        GGPOEvent info;

        switch (evt.type) {
            case ConnectionProtocol::Event::Connected:
                info.code = GGPO_EVENTCODE_CONNECTED_TO_PEER;
            info.u.connected.player = 0;
            _callbacks.on_event(_callbacks.instance, &info);
            break;
            case ConnectionProtocol::Event::Synchronizing:
                info.code = GGPO_EVENTCODE_SYNCHRONIZING_WITH_PEER;
            info.u.synchronizing.player = 0;
            info.u.synchronizing.count = evt.u.synchronizing.count;
            info.u.synchronizing.total = evt.u.synchronizing.total;
            _callbacks.on_event(_callbacks.instance, &info);
            break;
            case ConnectionProtocol::Event::Synchronzied:
                if (_synchronizing) {
                    info.code = GGPO_EVENTCODE_SYNCHRONIZED_WITH_PEER;
                    info.u.synchronized.player = 0;
                    _callbacks.on_event(_callbacks.instance, &info);

                    info.code = GGPO_EVENTCODE_RUNNING;
                    _callbacks.on_event(_callbacks.instance, &info);
                    _synchronizing = false;
                }
            break;

            case ConnectionProtocol::Event::NetworkInterrupted:
                info.code = GGPO_EVENTCODE_CONNECTION_INTERRUPTED;
            info.u.connection_interrupted.player = 0;
            info.u.connection_interrupted.disconnect_timeout = evt.u.network_interrupted.disconnect_timeout;
            _callbacks.on_event(_callbacks.instance, &info);
            break;

            case ConnectionProtocol::Event::NetworkResumed:
                info.code = GGPO_EVENTCODE_CONNECTION_RESUMED;
            info.u.connection_resumed.player = 0;
            _callbacks.on_event(_callbacks.instance, &info);
            break;

            case ConnectionProtocol::Event::Disconnected:
                info.code = GGPO_EVENTCODE_DISCONNECTED_FROM_PEER;
            info.u.disconnected.player = 0;
            _callbacks.on_event(_callbacks.instance, &info);
            break;

            case ConnectionProtocol::Event::Input:
                GameInput& input = evt.u.input.input;

            _host.SetLocalFrameNumber(input.frame);
            _host.SendInputAck();
            _inputs[input.frame % SPECTATOR_FRAME_BUFFER_SIZE] = input;
            break;
        }
    }

protected:
    GGPOSessionCallbacks _callbacks;
    Poll _poll;
    Connection _connection;
    ConnectionProtocol _host;
    bool _synchronizing;
    int _input_size;
    int _num_players;
    int _next_input_to_send;
    GameInput _inputs[SPECTATOR_FRAME_BUFFER_SIZE];
};

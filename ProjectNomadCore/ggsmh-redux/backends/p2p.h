#pragma once

#include "ggsmh-redux/types.h"
#include "ggsmh-redux/poll.h"
#include "ggsmh-redux/sync.h"
#include "backend.h"
#include "ggsmh-redux/timesync.h"
#include "ggsmh-redux/network/connection_proto.h"

static const int RECOMMENDATION_INTERVAL = 240;
static const int DEFAULT_DISCONNECT_TIMEOUT = 5000;
static const int DEFAULT_DISCONNECT_NOTIFY_START = 750;

class Peer2PeerBackend : public IQuarkBackend, IPollSink, Connection::Callbacks {
public:
    Peer2PeerBackend(GGPOSessionCallbacks* cb, const char* gamename, GGPOConnection* ggpo_connection, int num_players,
                     int input_size) :
        _num_players(num_players),
        _input_size(input_size),
        _sync(_local_connect_status),
        _disconnect_timeout(DEFAULT_DISCONNECT_TIMEOUT),
        _disconnect_notify_start(DEFAULT_DISCONNECT_NOTIFY_START),
        _num_spectators(0),
        _next_spectator_frame(0) {
        _callbacks = *cb;
        _synchronizing = true;
        _next_recommended_sleep = 0;
        _ggpo_connection = ggpo_connection;
        /*
        * Initialize the synchronziation layer
        */
        Sync::Config config = {0};
        config.num_players = num_players;
        config.input_size = input_size;
        config.callbacks = _callbacks;
        config.num_prediction_frames = MAX_PREDICTION_FRAMES;
        _sync.Init(config);

        /*
        * Initialize the CONNECTION port
        */
        _connection.Init(&_poll, this, ggpo_connection);

        _endpoints = new ConnectionProtocol[_num_players];
        memset(_local_connect_status, 0, sizeof(_local_connect_status));
        for (int i = 0; i < ARRAY_SIZE(_local_connect_status); i++) {
            _local_connect_status[i].last_frame = -1;
        }

        /*
        * Preload the ROM
        */
        _callbacks.begin_game(_callbacks.instance, gamename);
    }

    virtual ~Peer2PeerBackend() {
        delete [] _endpoints;
    }


    virtual GGPOErrorCode DoPoll(int timeout) {
        if (!_sync.InRollback()) {
            _poll.Pump(0);

            PollConnectionProtocolEvents();

            if (!_synchronizing) {
                _sync.CheckSimulation(timeout);

                // notify all of our endpoints of their local frame number for their
                // next connection quality report
                int current_frame = _sync.GetFrameCount();
                for (int i = 0; i < _num_players; i++) {
                    _endpoints[i].SetLocalFrameNumber(current_frame);
                }

                int total_min_confirmed;
                if (_num_players <= 2) {
                    total_min_confirmed = Poll2Players(current_frame);
                } else {
                    total_min_confirmed = PollNPlayers(current_frame);
                }

                Log("last confirmed frame in p2p backend is %d.\n", total_min_confirmed);
                if (total_min_confirmed >= 0) {
                    ASSERT(total_min_confirmed != INT_MAX);
                    if (_num_spectators > 0) {
                        while (_next_spectator_frame <= total_min_confirmed) {
                            Log("pushing frame %d to spectators.\n", _next_spectator_frame);

                            GameInput input;
                            input.frame = _next_spectator_frame;
                            input.size = _input_size * _num_players;
                            _sync.GetConfirmedInputs(input.bits, _input_size * _num_players, _next_spectator_frame);
                            for (int i = 0; i < _num_spectators; i++) {
                                _spectators[i].SendInput(input);
                            }
                            _next_spectator_frame++;
                        }
                    }
                    Log("setting confirmed frame in sync to %d.\n", total_min_confirmed);
                    _sync.SetLastConfirmedFrame(total_min_confirmed);
                }

                // send timesync notifications if now is the proper time
                if (current_frame > _next_recommended_sleep) {
                    int interval = 0;
                    for (int i = 0; i < _num_players; i++) {
                        interval = MAX(interval, _endpoints[i].RecommendFrameDelay());
                    }

                    if (interval > 0) {
                        GGPOEvent info;
                        info.code = GGPO_EVENTCODE_TIMESYNC;
                        info.u.timesync.frames_ahead = interval;
                        _callbacks.on_event(_callbacks.instance, &info);
                        _next_recommended_sleep = current_frame + RECOMMENDATION_INTERVAL;
                    }
                }
                // XXX: this is obviously a farce...
                if (timeout) {
                    Sleep(1);
                }
            }
        }
        return GGPO_OK;
    }

    virtual GGPOErrorCode AddPlayer(GGPOPlayer* player, GGPOPlayerHandle* handle) {
        if (player->type == GGPO_PLAYERTYPE_SPECTATOR) {
            return AddSpectator(player->player_id);
        }

        int queue = player->player_num - 1;
        if (player->player_num < 1 || player->player_num > _num_players) {
            return GGPO_ERRORCODE_PLAYER_OUT_OF_RANGE;
        }
        *handle = QueueToPlayerHandle(queue);

        if (player->type == GGPO_PLAYERTYPE_REMOTE) {
            AddRemotePlayer(player->player_id, queue);
        }
        return GGPO_OK;
    }
    
    virtual GGPOErrorCode AddLocalInput(GGPOPlayerHandle player, void* values, int size) {
        int queue;
        GameInput input;
        GGPOErrorCode result;

        if (_sync.InRollback()) {
            return GGPO_ERRORCODE_IN_ROLLBACK;
        }
        if (_synchronizing) {
            return GGPO_ERRORCODE_NOT_SYNCHRONIZED;
        }
   
        result = PlayerHandleToQueue(player, &queue);
        if (!GGPO_SUCCEEDED(result)) {
            return result;
        }

        input.init(-1, (char *)values, size);

        // Feed the input for the current frame into the synchronzation layer.
        if (!_sync.AddLocalInput(queue, input)) {
            return GGPO_ERRORCODE_PREDICTION_THRESHOLD;
        }

        if (input.frame != GameInput::NullFrame) { // xxx: <- comment why this is the case
            // Update the local connect status state to indicate that we've got a
            // confirmed local frame for this player.  this must come first so it
            // gets incorporated into the next packet we send.

            Log("setting local connect status for local queue %d to %d", queue, input.frame);
            _local_connect_status[queue].last_frame = input.frame;

            // Send the input to all the remote players.
            for (int i = 0; i < _num_players; i++) {
                if (_endpoints[i].IsInitialized()) {
                    _endpoints[i].SendInput(input);
                }
            }
        }

        return GGPO_OK;
    }
    
    virtual GGPOErrorCode SyncInput(void* values, int size, int* disconnect_flags) {
        int flags;

        // Wait until we've started to return inputs.
        if (_synchronizing) {
            return GGPO_ERRORCODE_NOT_SYNCHRONIZED;
        }
        flags = _sync.SynchronizeInputs(values, size);
        if (disconnect_flags) {
            *disconnect_flags = flags;
        }
        return GGPO_OK;
    }
    
    virtual GGPOErrorCode IncrementFrame() {  
        Log("End of frame (%d)...\n", _sync.GetFrameCount());
        _sync.IncrementFrame();
        DoPoll(0);
        PollSyncEvents();

        return GGPO_OK;
    }

    /*
    * Called only as the result of a local decision to disconnect.  The remote
    * decisions to disconnect are a result of us parsing the peer_connect_settings
    * blob in every endpoint periodically.
    */
    virtual GGPOErrorCode DisconnectPlayer(GGPOPlayerHandle handle) {
        int queue;
        GGPOErrorCode result;

        result = PlayerHandleToQueue(player, &queue);
        if (!GGPO_SUCCEEDED(result)) {
            return result;
        }
   
        if (_local_connect_status[queue].disconnected) {
            return GGPO_ERRORCODE_PLAYER_DISCONNECTED;
        }

        if (!_endpoints[queue].IsInitialized()) {
            int current_frame = _sync.GetFrameCount();
            // xxx: we should be tracking who the local player is, but for now assume
            // that if the endpoint is not initalized, this must be the local player.
            Log("Disconnecting local player %d at frame %d by user request.\n", queue, _local_connect_status[queue].last_frame);
            for (int i = 0; i < _num_players; i++) {
                if (_endpoints[i].IsInitialized()) {
                    DisconnectPlayerQueue(i, current_frame);
                }
            }
        } else {
            Log("Disconnecting queue %d at frame %d by user request.\n", queue, _local_connect_status[queue].last_frame);
            DisconnectPlayerQueue(queue, _local_connect_status[queue].last_frame);
        }
        return GGPO_OK;
    }
    
    virtual GGPOErrorCode GetNetworkStats(GGPONetworkStats* stats, GGPOPlayerHandle handle) {
        int queue;
        GGPOErrorCode result;

        result = PlayerHandleToQueue(player, &queue);
        if (!GGPO_SUCCEEDED(result)) {
            return result;
        }

        memset(stats, 0, sizeof *stats);
        _endpoints[queue].GetNetworkStats(stats);

        return GGPO_OK;
    }
    
    virtual GGPOErrorCode SetFrameDelay(GGPOPlayerHandle player, int delay) { 
        int queue;
        GGPOErrorCode result;

        result = PlayerHandleToQueue(player, &queue);
        if (!GGPO_SUCCEEDED(result)) {
            return result;
        }
        _sync.SetFrameDelay(queue, delay);
        return GGPO_OK; 
    }
    
    virtual GGPOErrorCode SetDisconnectTimeout(int timeout) {
        _disconnect_timeout = timeout;
        for (int i = 0; i < _num_players; i++) {
            if (_endpoints[i].IsInitialized()) {
                _endpoints[i].SetDisconnectTimeout(_disconnect_timeout);
            }
        }
        return GGPO_OK;
    }
    
    virtual GGPOErrorCode SetDisconnectNotifyStart(int timeout) {
        _disconnect_notify_start = timeout;
        for (int i = 0; i < _num_players; i++) {
            if (_endpoints[i].IsInitialized()) {
                _endpoints[i].SetDisconnectNotifyStart(_disconnect_notify_start);
            }
        }
        return GGPO_OK;
    }

    virtual void OnMsg(int player_id, ConnectionMsg* msg, int len) {
        for (int i = 0; i < _num_players; i++) {
            if (_endpoints[i].HandlesMsg(player_id, msg)) {
                _endpoints[i].OnMsg(msg, len);
                return;
            }
        }
        for (int i = 0; i < _num_spectators; i++) {
            if (_spectators[i].HandlesMsg(player_id, msg)) {
                _spectators[i].OnMsg(msg, len);
                return;
            }
        }
    }

protected:
    GGPOErrorCode PlayerHandleToQueue(GGPOPlayerHandle player, int* queue) {
        int offset = ((int)player - 1);
        if (offset < 0 || offset >= _num_players) {
            return GGPO_ERRORCODE_INVALID_PLAYER_HANDLE;
        }
        *queue = offset;
        return GGPO_OK;
    }
    
    GGPOPlayerHandle QueueToPlayerHandle(int queue) {
        return (GGPOPlayerHandle)(queue + 1);
    }
    
    GGPOPlayerHandle QueueToSpectatorHandle(int queue) {
        return (GGPOPlayerHandle)(queue + 1000);
    }
    /* out of range of the player array, basically */
    
    void DisconnectPlayerQueue(int queue, int syncto) {
        GGPOEvent info;
        int framecount = _sync.GetFrameCount();

        _endpoints[queue].Disconnect();

        Log("Changing queue %d local connect status for last frame from %d to %d on disconnect request (current: %d).\n",
            queue, _local_connect_status[queue].last_frame, syncto, framecount);

        _local_connect_status[queue].disconnected = 1;
        _local_connect_status[queue].last_frame = syncto;

        if (syncto < framecount) {
            Log("adjusting simulation to account for the fact that %d disconnected @ %d.\n", queue, syncto);
            _sync.AdjustSimulation(syncto);
            Log("finished adjusting simulation.\n");
        }

        info.code = GGPO_EVENTCODE_DISCONNECTED_FROM_PEER;
        info.u.disconnected.player = QueueToPlayerHandle(queue);
        _callbacks.on_event(_callbacks.instance, &info);

        CheckInitialSync();
    }
    
    void PollSyncEvents() {
        Sync::Event e;
        while (_sync.GetEvent(e)) {
            OnSyncEvent(e);
        }
    }
    
    void PollConnectionProtocolEvents() {
        ConnectionProtocol::Event evt;
        for (int i = 0; i < _num_players; i++) {
            while (_endpoints[i].GetEvent(evt)) {
                OnConnectionProtocolPeerEvent(evt, i);
            }
        }
        for (int i = 0; i < _num_spectators; i++) {
            while (_spectators[i].GetEvent(evt)) {
                OnConnectionProtocolSpectatorEvent(evt, i);
            }
        }
    }
    
    void CheckInitialSync() {
        int i;

        if (_synchronizing) {
            // Check to see if everyone is now synchronized.  If so,
            // go ahead and tell the client that we're ok to accept input.
            for (i = 0; i < _num_players; i++) {
                // xxx: IsInitialized() must go... we're actually using it as a proxy for "represents the local player"
                if (_endpoints[i].IsInitialized() && !_endpoints[i].IsSynchronized() && !_local_connect_status[i].disconnected) {
                    return;
                }
            }
            for (i = 0; i < _num_spectators; i++) {
                if (_spectators[i].IsInitialized() && !_spectators[i].IsSynchronized()) {
                    return;
                }
            }

            GGPOEvent info;
            info.code = GGPO_EVENTCODE_RUNNING;
            _callbacks.on_event(_callbacks.instance, &info);
            _synchronizing = false;
        }
    }

    int Poll2Players(int current_frame) {
        int i;

        // discard confirmed frames as appropriate
        int total_min_confirmed = MAX_INT;
        for (i = 0; i < _num_players; i++) {
            bool queue_connected = true;
            if (_endpoints[i].IsRunning()) {
                int ignore;
                queue_connected = _endpoints[i].GetPeerConnectStatus(i, &ignore);
            }
            if (!_local_connect_status[i].disconnected) {
                total_min_confirmed = MIN(_local_connect_status[i].last_frame, total_min_confirmed);
            }
            Log("  local endp: connected = %d, last_received = %d, total_min_confirmed = %d.\n",
                !_local_connect_status[i].disconnected, _local_connect_status[i].last_frame, total_min_confirmed);
            if (!queue_connected && !_local_connect_status[i].disconnected) {
                Log("disconnecting i %d by remote request.\n", i);
                DisconnectPlayerQueue(i, total_min_confirmed);
            }
            Log("  total_min_confirmed = %d.\n", total_min_confirmed);
        }
        return total_min_confirmed;
    }

    int PollNPlayers(int current_frame) {
        int i, queue, last_received;

        // discard confirmed frames as appropriate
        int total_min_confirmed = MAX_INT;
        for (queue = 0; queue < _num_players; queue++) {
            bool queue_connected = true;
            int queue_min_confirmed = MAX_INT;
            Log("considering queue %d.\n", queue);
            for (i = 0; i < _num_players; i++) {
                // we're going to do a lot of logic here in consideration of endpoint i.
                // keep accumulating the minimum confirmed point for all n*n packets and
                // throw away the rest.
                if (_endpoints[i].IsRunning()) {
                    bool connected = _endpoints[i].GetPeerConnectStatus(queue, &last_received);

                    queue_connected = queue_connected && connected;
                    queue_min_confirmed = MIN(last_received, queue_min_confirmed);
                    Log("  endpoint %d: connected = %d, last_received = %d, queue_min_confirmed = %d.\n", i, connected,
                        last_received, queue_min_confirmed);
                } else {
                    Log("  endpoint %d: ignoring... not running.\n", i);
                }
            }
            // merge in our local status only if we're still connected!
            if (!_local_connect_status[queue].disconnected) {
                queue_min_confirmed = MIN(_local_connect_status[queue].last_frame, queue_min_confirmed);
            }
            Log("  local endp: connected = %d, last_received = %d, queue_min_confirmed = %d.\n",
                !_local_connect_status[queue].disconnected, _local_connect_status[queue].last_frame,
                queue_min_confirmed);

            if (queue_connected) {
                total_min_confirmed = MIN(queue_min_confirmed, total_min_confirmed);
            } else {
                // check to see if this disconnect notification is further back than we've been before.  If
                // so, we need to re-adjust.  This can happen when we detect our own disconnect at frame n
                // and later receive a disconnect notification for frame n-1.
                if (!_local_connect_status[queue].disconnected || _local_connect_status[queue].last_frame >
                    queue_min_confirmed) {
                    Log("disconnecting queue %d by remote request.\n", queue);
                    DisconnectPlayerQueue(queue, queue_min_confirmed);
                }
            }
            Log("  total_min_confirmed = %d.\n", total_min_confirmed);
        }
        return total_min_confirmed;
    }

    void AddRemotePlayer(int player_id, int queue) {
        /*
        * Start the state machine (xxx: no)
        */
        _synchronizing = true;

        _endpoints[queue].Init(&_connection, _poll, queue, player_id, _local_connect_status);
        _endpoints[queue].SetDisconnectTimeout(_disconnect_timeout);
        _endpoints[queue].SetDisconnectNotifyStart(_disconnect_notify_start);
        _endpoints[queue].Synchronize();
    }

    GGPOErrorCode AddSpectator(int player_id) {
        if (_num_spectators == GGPO_MAX_SPECTATORS) {
            return GGPO_ERRORCODE_TOO_MANY_SPECTATORS;
        }
        /*
        * Currently, we can only add spectators before the game starts.
        */
        if (!_synchronizing) {
            return GGPO_ERRORCODE_INVALID_REQUEST;
        }
        int queue = _num_spectators++;

        _spectators[queue].Init(&_connection, _poll, queue + 1000, player_id, _local_connect_status);
        _spectators[queue].SetDisconnectTimeout(_disconnect_timeout);
        _spectators[queue].SetDisconnectNotifyStart(_disconnect_notify_start);
        _spectators[queue].Synchronize();

        return GGPO_OK;
    }

    virtual void OnSyncEvent(Sync::Event& e) {}
    
    virtual void OnConnectionProtocolEvent(ConnectionProtocol::Event& e, GGPOPlayerHandle handle) {
        GGPOEvent info;

        switch (evt.type) {
            case ConnectionProtocol::Event::Connected:
                info.code = GGPO_EVENTCODE_CONNECTED_TO_PEER;
            info.u.connected.player = handle;
            _callbacks.on_event(_callbacks.instance, &info);
            break;
            case ConnectionProtocol::Event::Synchronizing:
                info.code = GGPO_EVENTCODE_SYNCHRONIZING_WITH_PEER;
            info.u.synchronizing.player = handle;
            info.u.synchronizing.count = evt.u.synchronizing.count;
            info.u.synchronizing.total = evt.u.synchronizing.total;
            _callbacks.on_event(_callbacks.instance, &info);
            break;
            case ConnectionProtocol::Event::Synchronzied:
                info.code = GGPO_EVENTCODE_SYNCHRONIZED_WITH_PEER;
            info.u.synchronized.player = handle;
            _callbacks.on_event(_callbacks.instance, &info);

            CheckInitialSync();
            break;

            case ConnectionProtocol::Event::NetworkInterrupted:
                info.code = GGPO_EVENTCODE_CONNECTION_INTERRUPTED;
            info.u.connection_interrupted.player = handle;
            info.u.connection_interrupted.disconnect_timeout = evt.u.network_interrupted.disconnect_timeout;
            _callbacks.on_event(_callbacks.instance, &info);
            break;

            case ConnectionProtocol::Event::NetworkResumed:
                info.code = GGPO_EVENTCODE_CONNECTION_RESUMED;
            info.u.connection_resumed.player = handle;
            _callbacks.on_event(_callbacks.instance, &info);
            break;
        }
    }
    
    virtual void OnConnectionProtocolPeerEvent(ConnectionProtocol::Event& e, int queue) {
        OnConnectionProtocolEvent(evt, QueueToPlayerHandle(queue));
        switch (evt.type) {
            case ConnectionProtocol::Event::Input:
                if (!_local_connect_status[queue].disconnected) {
                    int current_remote_frame = _local_connect_status[queue].last_frame;
                    int new_remote_frame = evt.u.input.input.frame;
                    ASSERT(current_remote_frame == -1 || new_remote_frame == (current_remote_frame + 1));

                    _sync.AddRemoteInput(queue, evt.u.input.input);
                    // Notify the other endpoints which frame we received from a peer
                    Log("setting remote connect status for queue %d to %d\n", queue, evt.u.input.input.frame);
                    _local_connect_status[queue].last_frame = evt.u.input.input.frame;
                }
            break;

            case ConnectionProtocol::Event::Disconnected:
                DisconnectPlayer(QueueToPlayerHandle(queue));
            break;
        }
    }
    
    virtual void OnConnectionProtocolSpectatorEvent(ConnectionProtocol::Event& e, int queue) {
        GGPOPlayerHandle handle = QueueToSpectatorHandle(queue);
        OnConnectionProtocolEvent(evt, handle);

        GGPOEvent info;

        switch (evt.type) {
            case ConnectionProtocol::Event::Disconnected:
                _spectators[queue].Disconnect();

            info.code = GGPO_EVENTCODE_DISCONNECTED_FROM_PEER;
            info.u.disconnected.player = handle;
            _callbacks.on_event(_callbacks.instance, &info);

            break;
        }
    }

protected:
    GGPOSessionCallbacks _callbacks;
    Poll _poll;
    Sync _sync;
    GGPOConnection* _ggpo_connection;
    Connection _connection;
    ConnectionProtocol* _endpoints;
    ConnectionProtocol _spectators[GGPO_MAX_SPECTATORS];
    int _num_spectators;
    int _input_size;

    bool _synchronizing;
    int _num_players;
    int _next_recommended_sleep;

    int _next_spectator_frame;
    int _disconnect_timeout;
    int _disconnect_notify_start;

    ConnectionMsg::connect_status _local_connect_status[CONNECTION_MSG_MAX_PLAYERS];
};
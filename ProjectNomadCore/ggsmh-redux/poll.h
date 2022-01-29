#pragma once

/* -----------------------------------------------------------------------
 * GGPO.net (http://ggpo.net)  -  Copyright 2009 GroundStorm Studios, LLC.
 *
 * Use of this software is governed by the MIT license that can be found
 * in the LICENSE file.
 */

#include "static_buffer.h"
#include "types.h"

#define MAX_POLLABLE_HANDLES     64


class IPollSink {
public:
    virtual ~IPollSink() { }
    virtual bool OnHandlePoll(void*) { return true; }
    virtual bool OnMsgPoll(void*) { return true; }
    virtual bool OnPeriodicPoll(void*, int) { return true; }
    virtual bool OnLoopPoll(void*) { return true; }
};

class Poll {
public:
    Poll() : _handle_count(0), _start_time(0) {
        /*
        * Create a dummy handle to simplify things.
        */
        _handles[_handle_count++] = CreateEvent(NULL, true, false, NULL);
    }

    void RegisterHandle(IPollSink* sink, HANDLE h, void* cookie = NULL) {
        ASSERT(_handle_count < MAX_POLLABLE_HANDLES - 1);

        _handles[_handle_count] = h;
        _handle_sinks[_handle_count] = PollSinkCb(sink, cookie);
        _handle_count++;
    }

    void RegisterMsgLoop(IPollSink* sink, void* cookie = NULL) {
        _msg_sinks.push_back(PollSinkCb(sink, cookie));
    }
    
    void RegisterPeriodic(IPollSink* sink, int interval, void* cookie = NULL) {
        _periodic_sinks.push_back(PollPeriodicSinkCb(sink, cookie, interval));
    }
    
    void RegisterLoop(IPollSink* sink, void* cookie = NULL) {
        _loop_sinks.push_back(PollSinkCb(sink, cookie));
    }

    void Run() {
        while (Pump(100)) {
            continue;
        }
    }
    
    bool Pump(int timeout) {
        int i, res;
        bool finished = false;

        if (_start_time == 0) {
            _start_time = Platform::GetCurrentTimeMS();
        }
        int elapsed = Platform::GetCurrentTimeMS() - _start_time;
        int maxwait = ComputeWaitTime(elapsed);
        if (maxwait != INFINITE) {
            timeout = MIN(timeout, maxwait);
        }

        res = WaitForMultipleObjects(_handle_count, _handles, false, timeout);
        if (res >= WAIT_OBJECT_0 && res < WAIT_OBJECT_0 + _handle_count) {
            i = res - WAIT_OBJECT_0;
            finished = !_handle_sinks[i].sink->OnHandlePoll(_handle_sinks[i].cookie) || finished;
        }
        for (i = 0; i < _msg_sinks.size(); i++) {
            PollSinkCb &cb = _msg_sinks[i];
            finished = !cb.sink->OnMsgPoll(cb.cookie) || finished;
        }

        for (i = 0; i < _periodic_sinks.size(); i++) {
            PollPeriodicSinkCb &cb = _periodic_sinks[i];
            if (cb.interval + cb.last_fired <= elapsed) {
                cb.last_fired = (elapsed / cb.interval) * cb.interval;
                finished = !cb.sink->OnPeriodicPoll(cb.cookie, cb.last_fired) || finished;
            }
        }

        for (i = 0; i < _loop_sinks.size(); i++) {
            PollSinkCb &cb = _loop_sinks[i];
            finished = !cb.sink->OnLoopPoll(cb.cookie) || finished;
        }
        return finished;
    }

protected:
    int ComputeWaitTime(int elapsed) {
        int waitTime = INFINITE;
        size_t count = _periodic_sinks.size();

        if (count > 0) {
            for (int i = 0; i < count; i++) {
                PollPeriodicSinkCb &cb = _periodic_sinks[i];
                int timeout = (cb.interval + cb.last_fired) - elapsed;
                if (waitTime == INFINITE || (timeout < waitTime)) {
                    waitTime = MAX(timeout, 0);
                }         
            }
        }
        return waitTime;
    }

    struct PollSinkCb {
        IPollSink* sink;
        void* cookie;
        PollSinkCb() : sink(NULL), cookie(NULL) { }
        PollSinkCb(IPollSink* s, void* c) : sink(s), cookie(c) { }
    };

    struct PollPeriodicSinkCb : public PollSinkCb {
        int interval;
        int last_fired;
        PollPeriodicSinkCb() : PollSinkCb(NULL, NULL), interval(0), last_fired(0) { }

        PollPeriodicSinkCb(IPollSink* s, void* c, int i) :
            PollSinkCb(s, c), interval(i), last_fired(0) { }
    };

    int _start_time;
    int _handle_count;
    HANDLE _handles[MAX_POLLABLE_HANDLES];
    PollSinkCb _handle_sinks[MAX_POLLABLE_HANDLES];

    StaticBuffer<PollSinkCb, 16> _msg_sinks;
    StaticBuffer<PollSinkCb, 16> _loop_sinks;
    StaticBuffer<PollPeriodicSinkCb, 16> _periodic_sinks;
};

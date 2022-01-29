#pragma once

#include "types.h"

static FILE *logfile = NULL;
static char logbuf[4 * 1024 * 1024];

extern void Log(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    Logv(fmt, args);
    va_end(args);
}

extern void Logv(const char *fmt, va_list args) {
    if (!Platform::GetConfigBool("ggpo.log") || Platform::GetConfigBool("ggpo.log.ignore")) {
        return;
    }
    if (!logfile) {
        sprintf_s(logbuf, ARRAY_SIZE(logbuf), "log-%d.log", Platform::GetProcessID());
        fopen_s(&logfile, logbuf, "w");
    }
    Logv(logfile, fmt, args);
}

extern void Logv(FILE *fp, const char *fmt, va_list args) {
    if (Platform::GetConfigBool("ggpo.log.timestamps")) {
        static int start = 0;
        int t = 0;
        if (!start) {
            start = Platform::GetCurrentTimeMS();
        } else {
            t = Platform::GetCurrentTimeMS() - start;
        }
        fprintf(fp, "%d.%03d : ", t / 1000, t % 1000);
    }

    vfprintf(fp, fmt, args);
    fflush(fp);
   
    vsprintf_s(logbuf, ARRAY_SIZE(logbuf), fmt, args);
}

extern void LogFlush() {
    if (logfile) {
        fflush(logfile);
    }
}

// TODO: Undefined and unused, so remove
extern void LogFlushOnLog(bool flush);
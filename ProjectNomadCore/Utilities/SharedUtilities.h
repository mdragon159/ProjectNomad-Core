#pragma once

#include <cstdint>
#include <chrono>

namespace ProjectNomad {
    class SharedUtilities {
    public:
        static uint64_t getTimeInSeconds() {
            using namespace std::chrono;
            return duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
        }

        static uint64_t getTimeInMilliseconds() {
            // Credits to https://stackoverflow.com/a/56107709/3735890
            using namespace std::chrono;
            return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        }

        static uint64_t getTimeInMicroseconds() {
            // Credits to https://stackoverflow.com/a/49066369/3735890
            using namespace std::chrono;
            return duration_cast<microseconds>(high_resolution_clock::now().time_since_epoch()).count();
        }
    };
}
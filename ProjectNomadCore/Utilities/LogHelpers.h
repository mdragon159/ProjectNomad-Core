#pragma once

#include <queue>
#include <source_location>

#include "ILogger.h"
#include "NetLogMessage.h"
#include "Utilities/DebugMessage.h"
#include "Physics/Model/FCollider.h"

namespace ProjectNomad {
    class LogHelpers {
      public:
        LogHelpers() = delete;

        static constexpr std::string LocationToString(const std::source_location& location) {
            // During testing, the "filename" seems to be the full file path which is a lot of needless noise.
            // Thus, strip out everything up to the last "/" path separator (whether or not it exists).
            // Based on: https://cplusplus.com/reference/string/string/find_last_of/
            std::string fullFilename = std::string(location.file_name());
            std::size_t lastSeparator = fullFilename.find_last_of("/\\"); // If not found, then will equal std::string::npos which is -1
            std::string specificFilenamePart = fullFilename.substr(lastSeparator+1);

            // Format the final result with all relevant useful identifier info
            return specificFilenamePart + "(" +
                std::to_string(location.line()) + "):" +
                location.function_name();
        }
    };
}

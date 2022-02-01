#pragma once

#include <EOS/Include/eos_common.h>

namespace ProjectNomad {
    class CrossPlatformIdWrapper {
        EOS_ProductUserId accountId = nullptr;

    public:
        CrossPlatformIdWrapper() {}
        CrossPlatformIdWrapper(EOS_ProductUserId accountId) : accountId(accountId) {}

        EOS_ProductUserId getAccountId() {
            return accountId;
        }

        bool isValid() {
            return accountId != nullptr && EOS_ProductUserId_IsValid(accountId);
        }

        // Based on SDK sample's FAccountHelpers::EpicAccountIDToString
        bool tryToString(std::string& result) {
            static char TempBuffer[EOS_PRODUCTUSERID_MAX_LENGTH + 1];
            int32_t tempBufferSize = sizeof(TempBuffer);
            EOS_EResult conversionResult = EOS_ProductUserId_ToString(accountId, TempBuffer, &tempBufferSize);

            if (conversionResult == EOS_EResult::EOS_Success) {
                result = TempBuffer;
                return true;
            }

            result = "ERROR";
            return false;
        }
    };
}

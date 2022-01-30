#pragma once

#include <EOS/Include/eos_common.h>

namespace ProjectNomad {
    class EpicAccountIdWrapper {
        EOS_EpicAccountId accountId = nullptr;

    public:
        EpicAccountIdWrapper() {}
        EpicAccountIdWrapper(EOS_EpicAccountId accountId) : accountId(accountId) {}

        EOS_EpicAccountId getAccountId() {
            return accountId;
        }

        bool isValid() {
            return accountId != nullptr && EOS_EpicAccountId_IsValid(accountId);
        }

        // Based on SDK sample's FAccountHelpers::EpicAccountIDToString
        bool tryToString(std::string& result) {
            static char TempBuffer[EOS_EPICACCOUNTID_MAX_LENGTH + 1];
            int32_t tempBufferSize = sizeof(TempBuffer);
            EOS_EResult conversionResult = EOS_EpicAccountId_ToString(accountId, TempBuffer, &tempBufferSize);

            if (conversionResult == EOS_EResult::EOS_Success) {
                result = TempBuffer;
                return true;
            }

            result = "ERROR";
            return false;
        }
    };
}

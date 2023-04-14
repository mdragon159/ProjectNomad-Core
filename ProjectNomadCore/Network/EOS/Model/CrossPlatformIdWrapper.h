#pragma once

#include <EOS/Include/eos_common.h>

namespace ProjectNomad {
    class CrossPlatformIdWrapper {
        EOS_ProductUserId accountId = nullptr;

    public:
        CrossPlatformIdWrapper() {}
        explicit CrossPlatformIdWrapper(EOS_ProductUserId accountId) : accountId(accountId) {}

        // Note that this method doesn't return a validated id, it just returns the string in the appropriate format.
        // This also means that isValid method will always return true due to being unable to verify
        static CrossPlatformIdWrapper fromString(const std::string& id) {
            EOS_ProductUserId formattedId = EOS_ProductUserId_FromString(id.c_str());
            return CrossPlatformIdWrapper(formattedId);
        }
        
        EOS_ProductUserId getAccountId() const {
            return accountId;
        }

        bool isValid() const {
            return accountId != nullptr && EOS_ProductUserId_IsValid(accountId);
        }

        // Based on SDK sample's FAccountHelpers::EpicAccountIDToString
        bool tryToString(std::string& result) const {
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

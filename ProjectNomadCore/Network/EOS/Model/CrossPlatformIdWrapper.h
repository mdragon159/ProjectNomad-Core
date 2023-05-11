#pragma once

#include <EOS/Include/eos_common.h>

namespace ProjectNomad {
    class CrossPlatformIdWrapper {
      public:
        CrossPlatformIdWrapper() {}
        explicit CrossPlatformIdWrapper(EOS_ProductUserId accountId) : mAccountId(accountId) {}

        // Note that this method doesn't return a validated id, it just returns the string in the appropriate format.
        // This also means that isValid method will always return true due to being unable to verify
        static CrossPlatformIdWrapper FromString(const std::string& id) {
            EOS_ProductUserId formattedId = EOS_ProductUserId_FromString(id.c_str());
            return CrossPlatformIdWrapper(formattedId);
        }
        
        EOS_ProductUserId GetAccountId() const {
            return mAccountId;
        }

        bool IsValid() const {
            return mAccountId != nullptr && EOS_ProductUserId_IsValid(mAccountId);
        }

        // Based on SDK sample's FAccountHelpers::EpicAccountIDToString
        bool TryToString(std::string& result) const {
            static char TempBuffer[EOS_PRODUCTUSERID_MAX_LENGTH + 1];
            int32_t tempBufferSize = sizeof(TempBuffer);
            EOS_EResult conversionResult = EOS_ProductUserId_ToString(mAccountId, TempBuffer, &tempBufferSize);

            if (conversionResult == EOS_EResult::EOS_Success) {
                result = TempBuffer;
                return true;
            }

            result = "ERROR";
            return false;
        }

        // Simple time saver for getting string without really caring about performance or tryToString somehow failing
        std::string ToStringForLogging() const {
            std::string result = "";
            TryToString(result);
            return result;
        }

        size_t ToHashValue() const {
            // Id is actually a pointer so use that for now. Easy conversion but questionable if correct for all situations.
            //      Eg, if create an id from string, would the id still point to the same exact value?
            return reinterpret_cast<std::size_t>(mAccountId);
        }

        // Implement operators primarily for std::set and std::unordered_map support
        bool operator==(const CrossPlatformIdWrapper& other) const
        {
            return GetAccountId() == other.GetAccountId();
        }
        bool operator!=(const CrossPlatformIdWrapper& other) const
        {
            return !(*this == other);
        }
        bool operator<(const CrossPlatformIdWrapper& other) const
        {
            return GetAccountId() < other.GetAccountId();
        }

      private:
        EOS_ProductUserId mAccountId = nullptr;
    };
}

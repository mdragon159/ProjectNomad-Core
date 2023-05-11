#pragma once

#include <string>

namespace ProjectNomad {
    // Sort of wrapper around EOS_UserInfo, the struct of data returned from EOS SDK querying for "user info".
    // Specifically, this struct wraps around the info we're interested in from those queries.
    struct EOSPlayerInfo {
        bool hasInfo = false;
        std::string sanitizedDisplayName = "<Not Yet Set>";
    };
}

#pragma once

namespace ProjectNomad {
    // Based on EOS sample app's SampleConstants.h
    class NetworkSecrets {
    public:
        /** The product id for the running application, found on the dev portal */
        static constexpr char ProductId[] = "";

        /** The sandbox id for the running application, found on the dev portal */
        static constexpr char SandboxId[] = "";

        /** The deployment id for the running application, found on the dev portal */
        static constexpr char DeploymentId[] = "";

        /** Client id of the service permissions entry, found on the dev portal */
        static constexpr char ClientCredentialsId[] = "";

        /** Client secret for accessing the set of permissions, found on the dev portal */
        static constexpr char ClientCredentialsSecret[] = "";

        /** Game name */
        // Not really a secret, can be extracted and put elsewhere
        static constexpr char GameName[] = "DragonJawad's Test Game";
    };
}

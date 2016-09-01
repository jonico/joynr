/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
 * %%
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * #L%
 */

#ifndef PROVIDERARBITRATOR_H
#define PROVIDERARBITRATOR_H
#include <unordered_set>
#include <string>
#include <vector>

#include "joynr/PrivateCopyAssign.h"
#include "joynr/JoynrExport.h"
#include "joynr/IArbitrationListener.h"
#include "joynr/Logger.h"
#include "joynr/DiscoveryQos.h"
#include "joynr/types/DiscoveryQos.h"
#include "joynr/types/Version.h"
#include "joynr/types/DiscoveryEntry.h"
#include "joynr/Semaphore.h"
#include "joynr/exceptions/JoynrException.h"

namespace joynr
{

namespace system
{
class IDiscoverySync;
} // namespace system

/*
 *  Base class for different arbitration strategies.
 */
class JOYNR_EXPORT ProviderArbitrator
{

public:
    virtual ~ProviderArbitrator() = default;

    /*
     *  Arbitrate until successful or until a timeout occurs
     */
    void startArbitration();

    /*
     *  attemptArbitration() has to be implemented by the concrete arbitration strategy.
     *  This method attempts arbitration and sets arbitrationStatus to indicate the
     *  state of arbitration.
     */
    virtual void attemptArbitration();

    /*
     *  Returns the result of the arbitration.
     */
    std::string getParticipantId();

    /*
     *  setArbitrationCallback expects a callback to a JoynrProviderProxy object which
     *  should be notified as soon as the arbitration is completed.
     */
    void setArbitrationListener(IArbitrationListener* listener);
    void removeArbitrationListener();

    // Testing.....
    virtual void receiveCapabilitiesLookupResults(
            const std::vector<joynr::types::DiscoveryEntry>& discoveryEntries);

protected:
    /*
     *  Creates a new ProviderArbitrator object which blocks the arbitration finished
     *  notification as long as no callback onject has been specified.
     *  This blocking is need for example for the fixed channel arbitrator which
     *  sets the channelId instantly.
     */
    ProviderArbitrator(const std::string& domain,
                       const std::string& interfaceName,
                       const joynr::types::Version& interfaceVersion,
                       joynr::system::IDiscoverySync& discoveryProxy,
                       const DiscoveryQos& discoveryQos);

    //    void receiveCapabilitiesLookupResults(
    //            const std::vector<joynr::types::DiscoveryEntry>& discoveryEntries);

    virtual std::string filterDiscoveryEntries(
            const std::vector<joynr::types::DiscoveryEntry>& discoveryEntries) = 0;

    void notifyArbitrationListener(const std::string& participantId);

    void notifyArbitrationListener(const exceptions::DiscoveryException& error);

    joynr::system::IDiscoverySync& discoveryProxy;
    DiscoveryQos discoveryQos;
    joynr::types::DiscoveryQos systemDiscoveryQos;
    std::vector<std::string> domains;
    std::string interfaceName;
    joynr::types::Version interfaceVersion;
    std::unordered_set<joynr::types::Version> discoveredIncompatibleVersions;
    exceptions::DiscoveryException arbitrationError;

private:
    DISALLOW_COPY_AND_ASSIGN(ProviderArbitrator);
    void setArbitrationStatus(ArbitrationStatus::ArbitrationStatusType arbitrationStatus);
    void setParticipantId(std::string participantId);
    void setArbitrationError(const exceptions::DiscoveryException& error);
    std::string participantId;
    ArbitrationStatus::ArbitrationStatusType arbitrationStatus;
    IArbitrationListener* listener;
    Semaphore listenerSemaphore;
    ADD_LOGGER(ProviderArbitrator);
};

} // namespace joynr
#endif // PROVIDERARBITRATOR_H

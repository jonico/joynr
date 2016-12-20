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
#ifndef PROXYBASE_H
#define PROXYBASE_H

#include <string>

#include "joynr/JoynrExport.h"
#include "joynr/Logger.h"
#include "joynr/MessagingQos.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/types/DiscoveryEntryWithMetaInfo.h"

namespace joynr
{

class IClientCache;
class ConnectorFactory;

class JOYNR_EXPORT ProxyBase
{

public:
    ProxyBase(ConnectorFactory* connectorFactory,
              IClientCache* cache,
              const std::string& domain,
              const MessagingQos& qosSettings,
              bool cached);
    virtual ~ProxyBase() = default;

    /**
     * Returns the participantId of the proxy object.
     * TODO: should this be part of the public API?
     * it is needed in proxy builder to register the next hop on message router.
     */
    const std::string& getProxyParticipantId() const;

protected:
    DISALLOW_COPY_AND_ASSIGN(ProxyBase);

    /*
     *  handleArbitrationFinished has to be implemented by the concrete provider proxy.
     *  It is called as soon as the arbitration result is available.
     */
    virtual void handleArbitrationFinished(
            const types::DiscoveryEntryWithMetaInfo& providerDiscoveryEntry,
            bool useInProcessConnector);

    ConnectorFactory* connectorFactory;
    IClientCache* cache;
    std::string domain;
    MessagingQos qosSettings;
    bool cached;
    std::string proxyParticipantId;
    types::DiscoveryEntryWithMetaInfo providerDiscoveryEntry;
    ADD_LOGGER(ProxyBase);
};

} // namespace joynr
#endif // PROXYBASE_H

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
#include "joynr/ProxyBase.h"
#include "joynr/types/DiscoveryEntryWithMetaInfo.h"
#include <tuple>

#include "joynr/Util.h"

namespace joynr
{

INIT_LOGGER(ProxyBase);

ProxyBase::ProxyBase(ConnectorFactory* connectorFactory,
                     IClientCache* cache,
                     const std::string& domain,
                     const MessagingQos& qosSettings,
                     bool cached)
        : connectorFactory(connectorFactory),
          cache(cache),
          domain(domain),
          qosSettings(qosSettings),
          cached(cached),
          proxyParticipantId(""),
          providerDiscoveryEntry()
{
    proxyParticipantId = util::createUuid();
}

void ProxyBase::handleArbitrationFinished(
        const types::DiscoveryEntryWithMetaInfo& providerDiscoveryEntry,
        bool useInProcessConnector)
{
    std::ignore = useInProcessConnector;
    this->providerDiscoveryEntry = providerDiscoveryEntry;
}

const std::string& ProxyBase::getProxyParticipantId() const
{
    return this->proxyParticipantId;
}

} // namespace joynr

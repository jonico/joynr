package io.joynr.messaging.routing;

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

import java.util.HashSet;
import java.util.Set;

import com.google.inject.Inject;
import com.google.inject.name.Named;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.messaging.MessagingPropertyKeys;
import joynr.JoynrMessage;
import joynr.system.RoutingTypes.Address;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class AddressManager {

    private static final Logger logger = LoggerFactory.getLogger(AddressManager.class);
    private final MulticastReceiverRegistry multicastReceiversRegistry;

    private RoutingTable routingTable;
    private MulticastAddressCalculator multicastAddressCalculator;

    protected static class PrimaryGlobalTransportHolder {
        @Inject(optional = true)
        @Named(MessagingPropertyKeys.PROPERTY_MESSAGING_PRIMARYGLOBALTRANSPORT)
        private String primaryGlobalTransport;

        public PrimaryGlobalTransportHolder() {
        }

        // For testing only
        protected PrimaryGlobalTransportHolder(String primaryGlobalTransport) {
            this.primaryGlobalTransport = primaryGlobalTransport;
        }

        public String get() {
            return primaryGlobalTransport;
        }
    }

    @Inject
    public AddressManager(RoutingTable routingTable,
                          PrimaryGlobalTransportHolder primaryGlobalTransport,
                          Set<MulticastAddressCalculator> multicastAddressCalculators,
                          MulticastReceiverRegistry multicastReceiverRegistry) {
        this.routingTable = routingTable;
        this.multicastReceiversRegistry = multicastReceiverRegistry;
        if (multicastAddressCalculators.size() > 1 && primaryGlobalTransport.get() == null) {
            throw new JoynrIllegalStateException("Multiple multicast address calculators registered, but no primary global transport set.");
        }
        if (multicastAddressCalculators.size() == 1) {
            this.multicastAddressCalculator = multicastAddressCalculators.iterator().next();
        } else {
            for (MulticastAddressCalculator multicastAddressCalculator : multicastAddressCalculators) {
                if (multicastAddressCalculator.supports(primaryGlobalTransport.get())) {
                    this.multicastAddressCalculator = multicastAddressCalculator;
                    break;
                }
            }
        }
    }

    /**
     * Get the address to which the passed in message should be sent to.
     * This can be an address contained in the {@link RoutingTable}, or a
     * multicast address calculated from the header content of the message.
     *
     * @param message the message for which we want to find an address to send it to.
     * @return the address to send the message to. Will not be null, because if an address can't be determined an exception is thrown.
     * @throws JoynrMessageNotSentException if no address can be determined / found for the given message.
     */
    public Set<Address> getAddresses(JoynrMessage message) {
        Set<Address> result = new HashSet<>();
        String toParticipantId = message.getTo();
        if (JoynrMessage.MESSAGE_TYPE_MULTICAST.equals(message.getType())) {
            handleMulticastMessage(message, result);
        } else if (toParticipantId != null && routingTable.containsKey(toParticipantId)) {
            Address address = routingTable.get(toParticipantId);
            if (address != null) {
                result.add(address);
            }
        }
        logger.trace("Found the following addresses for essage {}: {}", new Object[]{ message, result });
        if (result.size() == 0) {
            throw new JoynrMessageNotSentException("Failed to send Request: No address for given message: "
                + message);
        }
        return result;
    }

    private void handleMulticastMessage(JoynrMessage message, Set<Address> result) {
        if (!message.isReceivedFromGlobal() && multicastAddressCalculator != null) {
            Address calculatedAddress = multicastAddressCalculator.calculate(message);
            if (calculatedAddress != null) {
                result.add(calculatedAddress);
            }
        }
        String multicastId = message.getFrom() + "/" + message.getTo();
        Set<String> receivers = multicastReceiversRegistry.getReceivers(multicastId);
        for (String receiverParticipantId : receivers) {
            Address address = routingTable.get(receiverParticipantId);
            if (address != null) {
                result.add(address);
            }
        }
    }

}

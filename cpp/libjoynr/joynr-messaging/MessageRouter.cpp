/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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
#include "joynr/MessageRouter.h"
#include "joynr/DispatcherUtils.h"
#include "joynr/MessagingStubFactory.h"
#include "joynr/Directory.h"
#include "joynr/joynrlogging.h"
#include "joynr/system/RoutingTypes_QtAddress.h"
#include "joynr/types/QtProviderQos.h"
#include "joynr/RequestStatusCode.h"
#include "joynr/JsonSerializer.h"
#include "cluster-controller/access-control/IAccessController.h"
#include "joynr/IPlatformSecurityManager.h"

#include <chrono>

#include <cassert>

namespace joynr
{

using namespace joynr_logging;
Logger* MessageRouter::logger = Logging::getInstance()->getLogger("MSG", "MessageRouter");

using namespace std::chrono;

//------ ConsumerPermissionCallback --------------------------------------------

class ConsumerPermissionCallback : public IAccessController::IHasConsumerPermissionCallback
{
public:
    ConsumerPermissionCallback(MessageRouter& owningMessageRouter,
                               const JoynrMessage& message,
                               std::shared_ptr<system::RoutingTypes::QtAddress> destination);

    void hasConsumerPermission(bool hasPermission);

private:
    MessageRouter& owningMessageRouter;
    JoynrMessage message;
    std::shared_ptr<system::RoutingTypes::QtAddress> destination;
};

//------ MessageRouter ---------------------------------------------------------

MessageRouter::~MessageRouter()
{
    messageQueueCleanerTimer.shutdown();
    threadPool.shutdown();
    if (parentRouter != NULL) {
        delete parentRouter;
    }
    delete messagingStubFactory;
    delete messageQueue;
    delete runningParentResolves;
    delete securityManager;
}

MessageRouter::MessageRouter(IMessagingStubFactory* messagingStubFactory,
                             IPlatformSecurityManager* securityManager,
                             int maxThreads,
                             MessageQueue* messageQueue)
        : joynr::system::RoutingAbstractProvider(),
          messagingStubFactory(messagingStubFactory),
          routingTable("MessageRouter-RoutingTable"),
          routingTableLock(),
          threadPool("MessageRouter", maxThreads),
          parentRouter(NULL),
          parentAddress(NULL),
          incomingAddress(),
          messageQueue(messageQueue),
          messageQueueCleanerTimer(),
          runningParentResolves(new std::unordered_set<std::string>()),
          accessController(NULL),
          securityManager(securityManager),
          parentResolveMutex()
{
    messageQueueCleanerTimer.addTimer(
            [this](Timer::TimerId) { this->messageQueue->removeOutdatedMessages(); },
            [](Timer::TimerId) {},
            1000,
            true);

    providerQos.setCustomParameters(std::vector<joynr::types::CustomParameter>());
    providerQos.setProviderVersion(1);
    providerQos.setPriority(1);
    providerQos.setScope(joynr::types::ProviderScope::LOCAL);
    providerQos.setSupportsOnChangeSubscriptions(false);
}

MessageRouter::MessageRouter(
        IMessagingStubFactory* messagingStubFactory,
        std::shared_ptr<joynr::system::RoutingTypes::QtAddress> incomingAddress,
        int maxThreads,
        MessageQueue* messageQueue)
        : joynr::system::RoutingAbstractProvider(),
          messagingStubFactory(messagingStubFactory),
          routingTable("MessageRouter-RoutingTable"),
          routingTableLock(),
          threadPool("MessageRouter", maxThreads),
          parentRouter(NULL),
          parentAddress(NULL),
          incomingAddress(incomingAddress),
          messageQueue(messageQueue),
          messageQueueCleanerTimer(),
          runningParentResolves(new std::unordered_set<std::string>()),
          accessController(NULL),
          securityManager(NULL),
          parentResolveMutex()
{
    messageQueueCleanerTimer.addTimer(
            [this](Timer::TimerId) { this->messageQueue->removeOutdatedMessages(); },
            [](Timer::TimerId) {},
            1000,
            true);

    providerQos.setCustomParameters(std::vector<joynr::types::CustomParameter>());
    providerQos.setProviderVersion(1);
    providerQos.setPriority(1);
    providerQos.setScope(joynr::types::ProviderScope::LOCAL);
    providerQos.setSupportsOnChangeSubscriptions(false);
}

void MessageRouter::addProvisionedNextHop(
        std::string participantId,
        std::shared_ptr<joynr::system::RoutingTypes::QtAddress> address)
{
    addToRoutingTable(participantId, address);
}

void MessageRouter::setAccessController(std::shared_ptr<IAccessController> accessController)
{
    assert(accessController);
    this->accessController = accessController;
}

void MessageRouter::setParentRouter(
        joynr::system::RoutingProxy* parentRouter,
        std::shared_ptr<joynr::system::RoutingTypes::QtAddress> parentAddress,
        std::string parentParticipantId)
{
    this->parentRouter = parentRouter;
    this->parentAddress = parentAddress;

    // add the next hop to parent router
    // this is necessary because during normal registration, the parent proxy is not yet set
    addProvisionedNextHop(parentParticipantId, parentAddress);
    addNextHopToParent(parentRouter->getProxyParticipantId());
}

bool MessageRouter::isChildMessageRouter()
{
    if (!incomingAddress) {
        return false;
    }
    // if an incoming address is set, a parent message router is needed for correct configuration
    return parentRouter != NULL && parentAddress;
}

/**
  * Q (RDZ): What happens if the message cannot be forwarded? Exception? Log file entry?
  * Q (RDZ): When are messagingstubs removed? They are stored indefinitely in the factory
  */
void MessageRouter::route(const JoynrMessage& message)
{
    assert(messagingStubFactory != NULL);
    JoynrTimePoint now = time_point_cast<milliseconds>(system_clock::now());
    if (now > message.getHeaderExpiryDate()) {
        LOG_WARN(logger,
                 FormatString("Received expired message. Dropping the message (ID: %1).")
                         .arg(message.getHeaderMessageId())
                         .str());
        return;
    }

    // Validate the message if possible
    if (securityManager != NULL && !securityManager->validate(message)) {
        LOG_ERROR(logger,
                  FormatString("messageId %1 failed validation")
                          .arg(message.getHeaderMessageId())
                          .str());
        return;
    }

    LOG_DEBUG(logger,
              FormatString("Route message with Id %1 and payload %2")
                      .arg(message.getHeaderMessageId())
                      .arg(message.getPayload())
                      .str());
    // search for the destination address
    const QString destinationPartId = QString::fromStdString(message.getHeaderTo());
    std::shared_ptr<joynr::system::RoutingTypes::QtAddress> destAddress(NULL);

    {
        joynr::ReadLocker lock(routingTableLock);
        destAddress = routingTable.lookup(destinationPartId.toStdString());
    }
    // if destination address is not known
    if (!destAddress) {
        // save the message for later delivery
        messageQueue->queueMessage(message);
        LOG_DEBUG(logger, FormatString("message queued: %1").arg(message.getPayload()).str());

        // and try to resolve destination address via parent message router
        if (isChildMessageRouter()) {
            std::lock_guard<std::mutex> lock(parentResolveMutex);
            if (runningParentResolves->find(destinationPartId.toStdString()) ==
                runningParentResolves->end()) {
                runningParentResolves->insert(destinationPartId.toStdString());
                std::function<void(const bool&)> onSuccess =
                        [this, destinationPartId](const bool& resolved) {
                    if (resolved) {
                        LOG_INFO(this->logger,
                                 FormatString("Got destination address for participant %1")
                                         .arg(destinationPartId.toStdString())
                                         .str());
                        // save next hop in the routing table
                        this->addProvisionedNextHop(
                                destinationPartId.toStdString(), this->parentAddress);
                        this->removeRunningParentResolvers(destinationPartId);
                        this->sendMessages(destinationPartId.toStdString(), this->parentAddress);
                    } else {
                        LOG_ERROR(this->logger,
                                  FormatString("Failed to resolve next hop for participant %1")
                                          .arg(destinationPartId.toStdString())
                                          .str());
                        // TODO error handling in case of failing submission (?)
                    }
                };

                // TODO error handling in case of failing submission (?)
                parentRouter->resolveNextHopAsync(destinationPartId.toStdString(), onSuccess);
            }
        } else {
            // no parent message router to resolve destination address
            LOG_WARN(logger,
                     FormatString(
                             "No routing information found for destination participant ID \"%1\" "
                             "so far. Waiting for participant registration. "
                             "Queueing message (ID : %2)")
                             .arg(message.getHeaderTo())
                             .arg(message.getHeaderMessageId())
                             .str());
        }
        return;
    }

    if (accessController) {
        // Access control checks are asynchronous, callback will send message
        // if access is granted
        std::shared_ptr<IAccessController::IHasConsumerPermissionCallback> callback(
                new ConsumerPermissionCallback(*this, message, destAddress));
        accessController->hasConsumerPermission(message, callback);
        return;
    }

    // If this point is reached, the message can be sent without delay
    sendMessage(message, destAddress);
}

void MessageRouter::removeRunningParentResolvers(const QString& destinationPartId)
{
    std::lock_guard<std::mutex> lock(parentResolveMutex);
    if (runningParentResolves->find(destinationPartId.toStdString()) !=
        runningParentResolves->end()) {
        runningParentResolves->erase(destinationPartId.toStdString());
    }
}

void MessageRouter::sendMessages(const std::string& destinationPartId,
                                 std::shared_ptr<joynr::system::RoutingTypes::QtAddress> address)
{
    while (true) {
        MessageQueueItem* item = messageQueue->getNextMessageForParticipant(destinationPartId);
        if (!item) {
            break;
        }
        sendMessage(item->getContent(), address);
        delete item;
    }
}

void MessageRouter::sendMessage(const JoynrMessage& message,
                                std::shared_ptr<joynr::system::RoutingTypes::QtAddress> destAddress)
{
    auto stub = messagingStubFactory->create(*destAddress);
    if (stub) {
        threadPool.execute(new MessageRunnable(message, stub));
    }
}

void MessageRouter::addNextHop(
        const std::string& participantId,
        const std::shared_ptr<joynr::system::RoutingTypes::QtAddress>& inprocessAddress,
        std::function<void()> onSuccess)
{
    addToRoutingTable(participantId, inprocessAddress);

    addNextHopToParent(participantId, onSuccess);

    sendMessages(participantId, inprocessAddress);
}

// inherited from joynr::system::RoutingProvider
void MessageRouter::addNextHop(
        const std::string& participantId,
        const system::RoutingTypes::ChannelAddress& channelAddress,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    std::shared_ptr<joynr::system::RoutingTypes::QtChannelAddress> address(
            new joynr::system::RoutingTypes::QtChannelAddress(
                    joynr::system::RoutingTypes::QtChannelAddress::createQt(channelAddress)));
    addToRoutingTable(participantId, address);

    addNextHopToParent(participantId, onSuccess, onError);

    sendMessages(participantId, address);
}

// inherited from joynr::system::RoutingProvider
void MessageRouter::addNextHop(
        const std::string& participantId,
        const system::RoutingTypes::CommonApiDbusAddress& commonApiDbusAddress,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    std::shared_ptr<joynr::system::RoutingTypes::QtCommonApiDbusAddress> address(
            new joynr::system::RoutingTypes::QtCommonApiDbusAddress(
                    joynr::system::RoutingTypes::QtCommonApiDbusAddress::createQt(
                            commonApiDbusAddress)));
    addToRoutingTable(participantId, address);

    addNextHopToParent(participantId, onSuccess, onError);

    sendMessages(participantId, address);
}

// inherited from joynr::system::RoutingProvider
void MessageRouter::addNextHop(
        const std::string& participantId,
        const system::RoutingTypes::BrowserAddress& browserAddress,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    std::shared_ptr<joynr::system::RoutingTypes::QtBrowserAddress> address(
            new joynr::system::RoutingTypes::QtBrowserAddress(
                    joynr::system::RoutingTypes::QtBrowserAddress::createQt(browserAddress)));
    addToRoutingTable(participantId, address);

    addNextHopToParent(participantId, onSuccess, onError);

    sendMessages(participantId, address);
}

// inherited from joynr::system::RoutingProvider
void MessageRouter::addNextHop(
        const std::string& participantId,
        const system::RoutingTypes::WebSocketAddress& webSocketAddress,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    std::shared_ptr<joynr::system::RoutingTypes::QtWebSocketAddress> address(
            new joynr::system::RoutingTypes::QtWebSocketAddress(
                    joynr::system::RoutingTypes::QtWebSocketAddress::createQt(webSocketAddress)));
    addToRoutingTable(participantId, address);

    addNextHopToParent(participantId, onSuccess, onError);

    sendMessages(participantId, address);
}

// inherited from joynr::system::RoutingProvider
void MessageRouter::addNextHop(
        const std::string& participantId,
        const system::RoutingTypes::WebSocketClientAddress& webSocketClientAddress,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    std::shared_ptr<joynr::system::RoutingTypes::QtWebSocketClientAddress> address(
            new joynr::system::RoutingTypes::QtWebSocketClientAddress(
                    joynr::system::RoutingTypes::QtWebSocketClientAddress::createQt(
                            webSocketClientAddress)));
    addToRoutingTable(participantId, address);

    addNextHopToParent(participantId, onSuccess, onError);

    sendMessages(participantId, address);
}

void MessageRouter::addNextHopToParent(
        std::string participantId,
        std::function<void(void)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    std::function<void(const exceptions::JoynrException&)> onErrorWrapper =
            [onError](const exceptions::JoynrException& error) {
        onError(joynr::exceptions::ProviderRuntimeException(error.getMessage()));
    };

    // add to parent router
    if (isChildMessageRouter()) {
        if (incomingAddress->inherits("joynr::system::RoutingTypes::QtChannelAddress")) {
            parentRouter->addNextHopAsync(
                    participantId,
                    joynr::system::RoutingTypes::QtChannelAddress::createStd(
                            *dynamic_cast<joynr::system::RoutingTypes::QtChannelAddress*>(
                                    incomingAddress.get())),
                    onSuccess,
                    onErrorWrapper);
        }
        if (incomingAddress->inherits("joynr::system::RoutingTypes::QtCommonApiDbusAddress")) {
            parentRouter->addNextHopAsync(
                    participantId,
                    joynr::system::RoutingTypes::QtCommonApiDbusAddress::createStd(
                            *dynamic_cast<joynr::system::RoutingTypes::QtCommonApiDbusAddress*>(
                                    incomingAddress.get())),
                    onSuccess,
                    onErrorWrapper);
        }
        if (incomingAddress->inherits("joynr::system::RoutingTypes::QtBrowserAddress")) {
            parentRouter->addNextHopAsync(
                    participantId,
                    joynr::system::RoutingTypes::QtBrowserAddress::createStd(
                            *dynamic_cast<joynr::system::RoutingTypes::QtBrowserAddress*>(
                                    incomingAddress.get())),
                    onSuccess,
                    onErrorWrapper);
        }
        if (incomingAddress->inherits("joynr::system::RoutingTypes::QtWebSocketAddress")) {
            parentRouter->addNextHopAsync(
                    participantId,
                    joynr::system::RoutingTypes::QtWebSocketAddress::createStd(
                            *dynamic_cast<joynr::system::RoutingTypes::QtWebSocketAddress*>(
                                    incomingAddress.get())),
                    onSuccess,
                    onErrorWrapper);
        }
        if (incomingAddress->inherits("joynr::system::RoutingTypes::QtWebSocketClientAddress")) {
            parentRouter->addNextHopAsync(
                    participantId,
                    joynr::system::RoutingTypes::QtWebSocketClientAddress::createStd(
                            *dynamic_cast<joynr::system::RoutingTypes::QtWebSocketClientAddress*>(
                                    incomingAddress.get())),
                    onSuccess,
                    onErrorWrapper);
        }
    } else if (onSuccess) {
        onSuccess();
    }
}

void MessageRouter::addToRoutingTable(
        std::string participantId,
        std::shared_ptr<joynr::system::RoutingTypes::QtAddress> address)
{
    joynr::WriteLocker lock(routingTableLock);
    routingTable.add(participantId, address);
}

// inherited from joynr::system::RoutingProvider
void MessageRouter::removeNextHop(
        const std::string& participantId,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    {
        joynr::WriteLocker lock(routingTableLock);
        routingTable.remove(participantId);
    }

    std::function<void(const exceptions::JoynrException&)> onErrorWrapper =
            [onError](const exceptions::JoynrException& error) {
        onError(joynr::exceptions::ProviderRuntimeException(error.getMessage()));
    };

    // remove from parent router
    if (isChildMessageRouter()) {
        parentRouter->removeNextHopAsync(participantId, onSuccess, onErrorWrapper);
    } else if (onSuccess) {
        onSuccess();
    }
}

// inherited from joynr::system::RoutingProvider
void MessageRouter::resolveNextHop(
        const std::string& participantId,
        std::function<void(const bool& resolved)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> /*onError*/)
{
    joynr::ReadLocker lock(routingTableLock);
    bool resolved = routingTable.contains(participantId);
    onSuccess(resolved);
}

/**
 * IMPLEMENTATION of MessageRunnable class
 */

Logger* MessageRunnable::logger = Logging::getInstance()->getLogger("MSG", "MessageRunnable");

MessageRunnable::MessageRunnable(const JoynrMessage& message,
                                 std::shared_ptr<IMessaging> messagingStub)
        : joynr::Runnable(true),
          ObjectWithDecayTime(message.getHeaderExpiryDate()),
          message(message),
          messagingStub(messagingStub)
{
}

void MessageRunnable::shutdown()
{
}

void MessageRunnable::run()
{
    if (!isExpired()) {
        messagingStub->transmit(message);
    } else {
        LOG_ERROR(logger,
                  FormatString("Message with ID %1 expired: dropping!")
                          .arg(message.getHeaderMessageId())
                          .str());
    }
}

/**
 * IMPLEMENTATION of ConsumerPermissionCallback class
 */

ConsumerPermissionCallback::ConsumerPermissionCallback(
        MessageRouter& owningMessageRouter,
        const JoynrMessage& message,
        std::shared_ptr<system::RoutingTypes::QtAddress> destination)
        : owningMessageRouter(owningMessageRouter), message(message), destination(destination)
{
}

void ConsumerPermissionCallback::hasConsumerPermission(bool hasPermission)
{
    if (hasPermission) {
        owningMessageRouter.sendMessage(message, destination);
    }
}

} // namespace joynr

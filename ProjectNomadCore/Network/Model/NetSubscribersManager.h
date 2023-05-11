#pragma once

#include <optional>

#include "Network/INetEventsSubscriber.h"

namespace ProjectNomad {
    /**
    * Encapsulates responsibility of storing subscriber references and passing along callbacks as appropriate
    **/
    class NetSubscribersManager : public INetEventsSubscriber {
    public:
        ~NetSubscribersManager() override = default;

        void SetRendererSubscriber(std::reference_wrapper<INetEventsSubscriber> subscriber) {
            mRendererSubscriber = subscriber;
        }
        void SetSimGameSubscriber(std::reference_wrapper<INetEventsSubscriber> subscriber) {
            mSimGameSubscriber = subscriber;
        }

        #pragma region INetEventsSubscriber interface
        void OnEOSInitialized() override {
            if (IsSimGameSubscriberSet()) {
                mSimGameSubscriber->get().OnEOSInitialized();
            }
            if (IsRendererSubscriberSet()) {
                mRendererSubscriber->get().OnEOSInitialized();
            }
        }
        void OnLoginStatusChanged() override {
            if (IsSimGameSubscriberSet()) {
                mSimGameSubscriber->get().OnLoginStatusChanged();
            }
            if (IsRendererSubscriberSet()) {
                mRendererSubscriber->get().OnLoginStatusChanged();
            }
        }
        bool HandleReceivedP2PMessage(const CrossPlatformIdWrapper& senderId,
                                      NetMessageType messageType,
                                      const std::vector<char>& messageData) override {
            if (IsSimGameSubscriberSet()) {
                bool wasMessageHandled = mSimGameSubscriber->get().HandleReceivedP2PMessage(
                    senderId, messageType, messageData
                );
                
                // If message handled, then no need to propagate message further.
                //      Intention is a mix of tiny optimization, following patterns from memory, and fact that
                //          renderer subscriber logs if receives message type that can't handle.
                //      That last part is most important, as nice mechanism to validate that not forgetting to
                //          handle certain message types.
                if (wasMessageHandled) {
                    return true;
                }
            }
            if (IsRendererSubscriberSet()) {
                return mRendererSubscriber->get().HandleReceivedP2PMessage(
                    senderId, messageType, messageData
                );
            }

            return false;
        }
        void OnAllPlayerInfoQueriesCompleted() override {
            if (IsSimGameSubscriberSet()) {
                mSimGameSubscriber->get().OnAllPlayerInfoQueriesCompleted();
            }
            if (IsRendererSubscriberSet()) {
                mRendererSubscriber->get().OnAllPlayerInfoQueriesCompleted();
            }
        }
        void OnReceivedPlayerSpotMapping() override {
            if (IsSimGameSubscriberSet()) {
                mSimGameSubscriber->get().OnReceivedPlayerSpotMapping();
            }
            if (IsRendererSubscriberSet()) {
                mRendererSubscriber->get().OnReceivedPlayerSpotMapping();
            }
        }
        void OnLobbyCreationResult(bool didLobbyCreationSucceed) override {
            if (IsSimGameSubscriberSet()) {
                mSimGameSubscriber->get().OnLobbyCreationResult(didLobbyCreationSucceed);
            }
            if (IsRendererSubscriberSet()) {
                mRendererSubscriber->get().OnLobbyCreationResult(didLobbyCreationSucceed);
            }
        }
        void OnLobbyJoinResult(bool didLobbyJoinSucceed) override {
            if (IsSimGameSubscriberSet()) {
                mSimGameSubscriber->get().OnLobbyJoinResult(didLobbyJoinSucceed);
            }
            if (IsRendererSubscriberSet()) {
                mRendererSubscriber->get().OnLobbyJoinResult(didLobbyJoinSucceed);
            }
        }
        void OnLobbyLeftOrDestroyed(bool didSucceed) override {
            if (IsSimGameSubscriberSet()) {
                mSimGameSubscriber->get().OnLobbyLeftOrDestroyed(didSucceed);
            }
            if (IsRendererSubscriberSet()) {
                mRendererSubscriber->get().OnLobbyLeftOrDestroyed(didSucceed);
            }
        }
        void OnLobbyUpdated() override {
            if (IsSimGameSubscriberSet()) {
                mSimGameSubscriber->get().OnLobbyUpdated(); // Assuming no need for didSucceed parameter downstream
            }
            if (IsRendererSubscriberSet()) {
                mRendererSubscriber->get().OnLobbyUpdated(); // Assuming no need for didSucceed parameter downstream
            }
        }
        void OnLobbyJoinOrCreateBegin() override {
            if (IsSimGameSubscriberSet()) {
                mSimGameSubscriber->get().OnLobbyJoinOrCreateBegin();
            }
            if (IsRendererSubscriberSet()) {
                mRendererSubscriber->get().OnLobbyJoinOrCreateBegin();
            }
        }
        void OnLobbyLeaveBegin() override {
            if (IsSimGameSubscriberSet()) {
                mSimGameSubscriber->get().OnLobbyLeaveBegin();
            }
            if (IsRendererSubscriberSet()) {
                mRendererSubscriber->get().OnLobbyLeaveBegin();
            }
        }
        #pragma endregion

      private:
        bool IsRendererSubscriberSet() const {
            return mRendererSubscriber.has_value();
        }
        bool IsSimGameSubscriberSet() const {
            return mSimGameSubscriber.has_value();
        }
        
        // Use std::reference_wrapper to be explicit that the subscribers are NOT owned by this class whatsoever.
        // And use std::optional around that for null support (as std::reference_wrapper cannot be null).
        std::optional<std::reference_wrapper<INetEventsSubscriber>> mSimGameSubscriber = {};
        std::optional<std::reference_wrapper<INetEventsSubscriber>> mRendererSubscriber = {};
    };
}

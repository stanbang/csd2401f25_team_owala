/*********************************************************************//**
 * \file   MessageBus.h
 *
 * \primary_author
 *      Ng Jordain Ying Zhe (100%)
 *
 * \brief
 *      Implements a thread-safe MessageBus class used for decoupled
 *      communication between systems in the ECS framework.
 *
 * \details
 *      The MessageBus enables event-driven communication using a
 *      publish–subscribe model. Systems can:
 *        - Define custom event structs (e.g., `PlayerJumpedEvent`) in
 *          `GameMessages.h`.
 *        - Publish these events (e.g., from `PlayerSystem.cpp`).
 *        - Subscribe to them from other systems (e.g., `AudioSystem.cpp`)
 *          to react to gameplay actions without tight coupling.
 *
 *      Example usage:
 *      -----------------------------
 *      struct PlayerJumpedEvent {
 *          int playerId;
 *          Vector2D position;
 *      };
 *
 *      // In PlayerSystem.cpp
 *      MessageBus::Instance().Publish(PlayerJumpedEvent{ playerId, pos });
 *
 *      // In AudioSystem.cpp
 *      MessageBus::Instance().Subscribe<PlayerJumpedEvent>([](const PlayerJumpedEvent& e) {
 *          PlaySound("jump.wav");
 *      });
 *
 *      -----------------------------
 *      Internally, MessageBus uses `std::type_index` to map message types
 *      to vectors of generic callbacks, ensuring type safety and flexibility.
 *
 * \copyright
 *      Copyright (C) 2025 DigiPen Institute of Technology.
 *      Reproduction or disclosure of this file or its contents without the prior
 *      written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#pragma once
#include <unordered_map>
#include <vector>
#include <functional>
#include <typeindex>
#include <mutex>

namespace Framework {

    /**
     * \class MessageBus
     * \brief A singleton event bus for decoupled inter-system communication.
     *
     * The MessageBus allows systems to publish and subscribe to events
     * without having compile-time dependencies on one another.
     * Thread safety is ensured through internal mutex locking.
     */
    class MessageBus {
    public:
        /**
         * \brief Retrieves the singleton instance of the MessageBus.
         * \return Reference to the global MessageBus instance.
         */
        static MessageBus& Instance() {
            static MessageBus instance;
            return instance;
        }

        /**
         * \brief Subscribes to a specific message/event type.
         *
         * Registers a callback function that will be invoked whenever
         * an event of type \p T is published.
         *
         * \tparam T Type of event struct to subscribe to.
         * \param callback Function that takes a `const T&` argument.
         *
         * Example:
         * \code
         * MessageBus::Instance().Subscribe<PlayerJumpedEvent>(
         *     [](const PlayerJumpedEvent& e) { std::cout << "Player jumped!"; });
         * \endcode
         */
        template <typename T>
        void Subscribe(std::function<void(const T&)> callback) {
            std::lock_guard<std::mutex> lock(mutex_);
            auto& vec = subscribers_[typeid(T)];
            vec.push_back([callback](const void* msg) {
                callback(*static_cast<const T*>(msg));
                });
        }

        /**
         * \brief Publishes a message of a specific type.
         *
         * Immediately notifies all subscribers of type \p T by invoking
         * their registered callbacks with the given message.
         *
         * \tparam T Type of event struct being published.
         * \param message The event instance to deliver.
         *
         * Example:
         * \code
         * PlayerJumpedEvent evt{ playerId, playerPosition };
         * MessageBus::Instance().Publish(evt);
         * \endcode
         */
        template <typename T>
        void Publish(const T& message) {
            // PREPARE SNAPSHOT
            // Create a local list to hold the callbacks for this specific frame.
            std::vector<AnyCallback> callbacksToRun;

            {
                // We lock ONLY long enough to find and copy the pointers.
                // We do NOT hold the lock while running user code.
                std::lock_guard<std::mutex> lock(mutex_);

                auto it = subscribers_.find(typeid(T));
                if (it == subscribers_.end()) return;

                if (it != subscribers_.end()) {
                    callbacksToRun = it->second; // Copy the vector
                }
            }

            for (auto& sub : callbacksToRun)
                sub(&message);
        }

    private:
        using AnyCallback = std::function<void(const void*)>; ///< Generic callback type used internally.

        /**
         * \brief Maps message types to their corresponding list of subscribers.
         *
         * Each key (`std::type_index`) corresponds to an event struct type,
         * and each value is a vector of generic callback wrappers.
         */
        std::unordered_map<std::type_index, std::vector<AnyCallback>> subscribers_;

        std::mutex mutex_; ///< Synchronization lock for thread safety.

        /** \brief Private constructor (singleton pattern). */
        MessageBus() = default;

        /** \brief Deleted copy constructor to enforce singleton behavior. */
        MessageBus(const MessageBus&) = delete;

        /** \brief Deleted copy assignment operator to enforce singleton behavior. */
        MessageBus& operator=(const MessageBus&) = delete;
    };

} // namespace Framework

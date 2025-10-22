#pragma once
#include <unordered_map>
#include <vector>
#include <functional>
#include <typeindex>
#include <mutex>

namespace Framework {

    // --------------------------------------------------------------
    // MessageBus: decouples communication between systems
    // --------------------------------------------------------------
    class MessageBus {
    public:
        static MessageBus& Instance() {
            static MessageBus instance;
            return instance;
        }

        // Subscribe to a specific message type (e.g. CollisionEvent)
        template <typename T>
        void Subscribe(std::function<void(const T&)> callback) {
            std::lock_guard<std::mutex> lock(mutex_);
            auto& vec = subscribers_[typeid(T)];
            vec.push_back([callback](const void* msg) {
                callback(*static_cast<const T*>(msg));
                });
        }

        // Publish a message immediately to all subscribers
        template <typename T>
        void Publish(const T& message) {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = subscribers_.find(typeid(T));
            if (it == subscribers_.end()) return;

            for (auto& sub : it->second)
                sub(&message);
        }

    private:
        using AnyCallback = std::function<void(const void*)>;
        std::unordered_map<std::type_index, std::vector<AnyCallback>> subscribers_;
        std::mutex mutex_;

        MessageBus() = default;
        MessageBus(const MessageBus&) = delete;
        MessageBus& operator=(const MessageBus&) = delete;
    };

} // namespace Framework

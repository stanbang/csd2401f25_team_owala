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


    //can use the jumpevent for example in gamemessages.h, PlayerSystem.cpp, AudioSystem.cpp
    /* how to use
    *   -   create a event(struct) in gamemessages.h, containing all the information needed
    *   -   then decide a event that will trigger the publishing of the event (e.g. in PlayerSystem.cpp)
    *   -   then choose which systems to subscribe to the event and collect all the necessary information from 
            the struct, and what to do with the information (e.g. AudioSystem.cpp)
    */
    class MessageBus {
    public:
        static MessageBus& Instance() {
            static MessageBus instance;
            return instance;
        }

        // Subscribe to a specific message type (e.g. PlayerJumpedEvent)
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
        /*
        * std::type_index stores all the events created, then std::vector<AnyCallBack> stores all the systems that is subscribed to the event
        */
        std::unordered_map<std::type_index, std::vector<AnyCallback>> subscribers_;
        std::mutex mutex_;

        //std stuff
        MessageBus() = default;
        MessageBus(const MessageBus&) = delete;
        MessageBus& operator=(const MessageBus&) = delete;
    };

} // namespace Framework

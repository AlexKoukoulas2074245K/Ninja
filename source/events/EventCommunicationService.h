//
//  EventCommunicationService.h
//  Hardcore2D
//
//  Created by Alex Koukoulas on 07/01/2019.
//

#ifndef EventCommunicationService_h
#define EventCommunicationService_h

#include "../IService.h"
#include "../util/TypeTraits.h"

#include <memory>
#include <unordered_map>
#include <vector>
#include <functional>

class IEvent;
class EventCommunicator;
class EventCommunicationService final: public IService
{
    friend class App;
public:
    using EventCallback = std::function<void(const IEvent&)>;
    
    ~EventCommunicationService();

    template <class EventType>
    inline void RegisterEventCallback(EventCommunicator* listener, EventCallback eventCallback)
    {
        const auto eventTypeHash = GetStringHash(typeid(EventType).name());
        mEventCallbackRegistry[eventTypeHash].push_back(std::make_pair(listener, eventCallback));
    }
        
    std::unique_ptr<EventCommunicator> CreateEventCommunicator();

    void DispatchEvent(EventCommunicator* dispatcher, std::unique_ptr<IEvent> evt);
    
    void UnregisterAllCallbacksForListener(EventCommunicator* listener);
    
private:
    EventCommunicationService() = default;
    
    std::unordered_map<size_t, std::vector<std::pair<EventCommunicator*, EventCallback>>> mEventCallbackRegistry;
    std::vector<std::pair<std::string, EventCommunicator*>> mEventDispatchingHistory;    
};

#endif /* EventCommunicationService_h */

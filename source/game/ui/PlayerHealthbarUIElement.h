//
//  PlayerHealthbarUIElement.h
//  Hardcore2D
//
//  Created by Alex Koukoulas on 19/02/2019.
//

#ifndef PlayerHealthbarUIElement_h
#define PlayerHealthbarUIElement_h

#include "IUIElement.h"

#include <memory>

class CoreRenderingService;
class ServiceLocator;
class EntityComponentManager;
class EventCommunicator;
class ResourceManager;

class PlayerHealthbarUIElement final: public IUIElement
{
public:
    PlayerHealthbarUIElement(const ServiceLocator&);
    ~PlayerHealthbarUIElement();
    
    void VUpdate(const float dt) override;
    const std::vector<EntityId>& VGetEntityIds() const override;
    
private:
    void InitializeHealthbarEntities();
    
    const CoreRenderingService& mCoreRenderingService;
    EntityComponentManager& mEntityComponentManager;
    ResourceManager& mResourceManager;
    std::unique_ptr<EventCommunicator> mEventCommunicator;
    
    std::vector<EntityId> mEntityIds;
};

#endif /* PlayerHealthbarUIElement_h */

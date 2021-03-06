//
//  MovingPlatformAIComponent.h
//  Hardcore2D
//
//  Created by Alex Koukoulas on 23/02/2019.
//

#ifndef MovingPlatformAIComponent_h
#define MovingPlatformAIComponent_h

#include "IAIComponent.h"
#include "../util/TypeTraits.h"
#include "../util/MathUtils.h"
#include "../util/Timer.h"

class ServiceLocator;
class EntityComponentManager;

class MovingPlatformAIComponent final: public IAIComponent
{
public:
    MovingPlatformAIComponent(const ServiceLocator&, const EntityId entityId);
    
    void VUpdate(const float dt) override;
    
private:
    static const float MOVEMENT_DURATION_PER_DIRECTION;
    
    void OnMovementTimerTick();
    
    EntityComponentManager& mEntityComponentManager;
    const EntityId mEntityId;    
    bool mMovingNorthOrEastOrBoth;
    Timer mMovementTimer;
    
};

#endif /* MovingPlatformAIComponent_h */

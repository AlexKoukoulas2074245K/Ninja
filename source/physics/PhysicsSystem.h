//
//  PhysicsSystem.h
//  Hardcore2D
//
//  Created by Alex Koukoulas on 19/01/2019.
//

#ifndef PhysicsSystem_h
#define PhysicsSystem_h

#include "../IService.h"
#include "../util/TypeTraits.h"

#include <vector>
#include <utility>

class ServiceLocator;
class EntityComponentManager;

class PhysicsSystem final: public IService
{
    friend class App;
public:
    void Initialize();
    void UpdateEntities(const std::vector<EntityId>& entityIds, const float dt);
    
private:
    PhysicsSystem(const ServiceLocator&);
    
    enum class Axis
    {
        X_AXIS, Y_AXIS
    };

    EntityId CheckAndGetCollidedEntity(const EntityId referenceEntityId, const std::vector<EntityId>& allConsideredEntityIds);
    void PushEntityOutsideOtherEntityInAxis(const EntityId referenceEntityId, const EntityId collidedWithEntityId, const Axis);

    const ServiceLocator& mServiceLocator;
    EntityComponentManager* mEntityComponentManager;
};

#endif /* PhysicsSystem_h */
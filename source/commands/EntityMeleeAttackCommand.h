//
//  EntityMeleeAttackCommand.h
//  Hardcore2D
//
//  Created by Alex Koukoulas on 31/01/2019.
//

#ifndef EntityMeleeAttackCommand_h
#define EntityMeleeAttackCommand_h

#include "ICommand.h"
#include "../util/TypeTraits.h"

#include <memory>

class EntityComponentManager;
class ResourceManager;
class ServiceLocator;
class EventCommunicator;

class EntityMeleeAttackCommand final: public ICommand
{
public:
    static const StringId COMMAND_CLASS_ID;
    
    EntityMeleeAttackCommand(const ServiceLocator&, const EntityId);
    void Execute() override;
    StringId GetCommandClassId() const override;
    
private:
    EntityComponentManager& mEntityComponentManager;
    ResourceManager& mResourceManager;
    const EntityId mEntityId;
    std::unique_ptr<EventCommunicator> mEventCommunicator;
};


#endif /* EntityMeleeAttackCommand_h */
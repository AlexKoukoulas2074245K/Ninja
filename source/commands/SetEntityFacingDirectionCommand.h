//
//  SetEntityFacingDirectionCommand.h
//  Hardcore2D
//
//  Created by Alex Koukoulas on 31/01/2019.
//

#ifndef SetEntityFacingDirectionCommand_h
#define SetEntityFacingDirectionCommand_h

#include "ICommand.h"
#include "../game/GameTypeTraits.h"
#include "../game/GameConstants.h"

class EntityComponentManager;

class SetEntityFacingDirectionCommand final: public ICommand
{
public:
    SetEntityFacingDirectionCommand(EntityComponentManager&, const EntityId, const FacingDirection);
    void VExecute() override;
    
private:
    EntityComponentManager& mEntityComponentManager;
    
    const EntityId mEntityId;
    const FacingDirection mFacingDirection;
};

#endif /* SetEntityFacingDirectionCommand_h */

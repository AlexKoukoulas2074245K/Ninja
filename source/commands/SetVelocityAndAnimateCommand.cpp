//
//  SetVelocityAndAnimateCommand.cpp
//  Hardcore2D
//
//  Created by Alex Koukoulas on 21/01/2019.
//

#include "SetVelocityAndAnimateCommand.h"
#include "../commands/SetFacingDirectionCommand.h"
#include "../components/EntityComponentManager.h"
#include "../components/PhysicsComponent.h"
#include "../components/AnimationComponent.h"
#include "../game/GameConstants.h"
#include "../util/MathUtils.h"

SetVelocityAndAnimateCommand::SetVelocityAndAnimateCommand(EntityComponentManager& entityComponentManager, const EntityId entityId, const glm::vec3& velocity)
    : mEntityComponentManager(entityComponentManager)
    , mEntityId(entityId)
    , mVelocity(velocity)
{
    
}

void SetVelocityAndAnimateCommand::VExecute()
{
    if (mEntityComponentManager.HasComponent<AnimationComponent>(mEntityId))
    {
        auto& animationComponent = mEntityComponentManager.GetComponent<AnimationComponent>(mEntityId);
        if (Abs(mVelocity.x) < 1.0f)
        {
            animationComponent.PlayAnimation(StringId("idle"), true);
        }
        else
        {
            if (mVelocity.x < 0.0f)
            {
                if (animationComponent.GetCurrentFacingDirection() != FacingDirection::LEFT)
                {
                    SetFacingDirectionCommand(mEntityComponentManager, mEntityId, FacingDirection::LEFT).VExecute();
                }
            }
            else
            {
                if (animationComponent.GetCurrentFacingDirection() != FacingDirection::RIGHT)
                {
                    SetFacingDirectionCommand(mEntityComponentManager, mEntityId, FacingDirection::RIGHT).VExecute();
                }
            }
            
            if (animationComponent.HasAnimation(StringId("running")))
            {
                animationComponent.PlayAnimation(StringId("running"), true);
            }            
        }
        
    }
    mEntityComponentManager.GetComponent<PhysicsComponent>(mEntityId).mVelocity = mVelocity;
}

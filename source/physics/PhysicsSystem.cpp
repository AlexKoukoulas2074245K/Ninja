//
//  PhysicsSystem.cpp
//  Hardcore2D
//
//  Created by Alex Koukoulas on 19/01/2019.
//

#include "PhysicsSystem.h"
#include "../util/MathUtils.h"
#include "../util/Logging.h"
#include "../ServiceLocator.h"
#include "../components/EntityComponentManager.h"
#include "../components/PhysicsComponent.h"
#include "../components/TransformationComponent.h"

PhysicsSystem::PhysicsSystem(const ServiceLocator& serviceLocator)
    : mServiceLocator(serviceLocator)
{
    
}

void PhysicsSystem::Initialize()
{
    mEntityComponentManager = &(mServiceLocator.ResolveService<EntityComponentManager>());
}

void PhysicsSystem::UpdateEntities(const std::vector<EntityId>& entityIds, const float dt)
{
    for (const auto entityId: entityIds)
    {
        if (mEntityComponentManager->HasComponent<PhysicsComponent>(entityId) && 
            mEntityComponentManager->GetComponent<PhysicsComponent>(entityId).GetBodyType() != PhysicsComponent::BodyType::STATIC)
        {            
            auto& thisEntityTransformationComponent = mEntityComponentManager->GetComponent<TransformationComponent>(entityId);
            auto& thisEntityPhysicsComponent = mEntityComponentManager->GetComponent<PhysicsComponent>(entityId);
            
            // Update velocity
            thisEntityPhysicsComponent.GetVelocity() += thisEntityPhysicsComponent.GetGravity() * dt;
            
            // Clamp velocity to min/maxes
            thisEntityPhysicsComponent.GetVelocity() = ClampToMax(thisEntityPhysicsComponent.GetVelocity(), thisEntityPhysicsComponent.GetMaxVelocity());
            thisEntityPhysicsComponent.GetVelocity() = ClampToMin(thisEntityPhysicsComponent.GetVelocity(), thisEntityPhysicsComponent.GetMinVelocity());
            
            // Update horizontal position first
            thisEntityTransformationComponent.GetTranslation().x += thisEntityPhysicsComponent.GetVelocity().x * dt;
            
            // Find any entity collided with
            auto collisionCheckEntityId = CheckAndGetCollidedEntity(entityId, entityIds);            

            // If any were found, push the current entity outside of them
            if (collisionCheckEntityId != entityId &&                 
                mEntityComponentManager->GetComponent<PhysicsComponent>(collisionCheckEntityId).GetBodyType() != PhysicsComponent::BodyType::DYNAMIC)
            {
                const auto& otherEntityTransform = mEntityComponentManager->GetComponent<TransformationComponent>(collisionCheckEntityId);
                const auto& otherPhysicsComponent = mEntityComponentManager->GetComponent<PhysicsComponent>(collisionCheckEntityId);
                
                if (otherPhysicsComponent.GetBodyType() != PhysicsComponent::BodyType::KINEMATIC ||
                    (thisEntityTransformationComponent.GetTranslation().y < otherEntityTransform.GetTranslation().y + otherPhysicsComponent.GetHitBox().mDimensions.y * 0.5f))
                {
                    PushEntityOutsideOtherEntityInAxis(entityId, collisionCheckEntityId, Axis::X_AXIS);
                }
            }
            
            // Update vertical position next
            thisEntityTransformationComponent.GetTranslation().y += thisEntityPhysicsComponent.GetVelocity().y * dt;
            
            // Find any entity collided with
            collisionCheckEntityId = CheckAndGetCollidedEntity(entityId, entityIds);            

            // If any were found, push the current entity outside of them
            if (collisionCheckEntityId != entityId &&
                mEntityComponentManager->GetComponent<PhysicsComponent>(collisionCheckEntityId).GetBodyType() != PhysicsComponent::BodyType::DYNAMIC)
            {
                //const auto& otherEntityTransform = mEntityComponentManager->GetComponent<TransformationComponent>(collisionCheckEntityId);
                const auto& otherPhysicsComponent = mEntityComponentManager->GetComponent<PhysicsComponent>(collisionCheckEntityId);
                
                PushEntityOutsideOtherEntityInAxis(entityId, collisionCheckEntityId, Axis::Y_AXIS);
                
                if (otherPhysicsComponent.GetBodyType() == PhysicsComponent::BodyType::KINEMATIC)
                {
                    thisEntityTransformationComponent.GetTranslation() += otherPhysicsComponent.GetVelocity() * dt;
                }
                
            }
        }
    }
}

EntityId PhysicsSystem::CheckAndGetCollidedEntity(const EntityId referenceEntityId, const std::vector<EntityId>& allConsideredEntityIds)
{
    for (const EntityId otherEntityId: allConsideredEntityIds)
    {
        if (referenceEntityId != otherEntityId && mEntityComponentManager->HasComponent<PhysicsComponent>(otherEntityId))
        {
            auto& transfA = mEntityComponentManager->GetComponent<TransformationComponent>(referenceEntityId);
            auto& transfB = mEntityComponentManager->GetComponent<TransformationComponent>(otherEntityId);
            
            const auto& hitBoxA = mEntityComponentManager->GetComponent<PhysicsComponent>(referenceEntityId).GetHitBox();
            const auto& hitBoxB = mEntityComponentManager->GetComponent<PhysicsComponent>(otherEntityId).GetHitBox();

            const auto rectAX = transfA.GetTranslation().x + hitBoxA.mCenterPoint.x;
            const auto rectAY = transfA.GetTranslation().y + hitBoxA.mCenterPoint.y;
            
            const auto rectBX = transfB.GetTranslation().x + hitBoxB.mCenterPoint.x;
            const auto rectBY = transfB.GetTranslation().y + hitBoxB.mCenterPoint.y;

            const auto xAxisTest = Abs(rectAX - rectBX) * 2.0f - (hitBoxA.mDimensions.x + hitBoxB.mDimensions.x);
            const auto yAxisTest = Abs(rectAY - rectBY) * 2.0f - (hitBoxA.mDimensions.y + hitBoxB.mDimensions.y);
            
            if (xAxisTest < 0 && yAxisTest < 0)
            {
                return otherEntityId;
            }
        }
    }
    
    return referenceEntityId;
}

void PhysicsSystem::PushEntityOutsideOtherEntityInAxis(const EntityId referenceEntityId, const EntityId collidedWithEntityId, const PhysicsSystem::Axis axis)
{
    auto& thisEntityTransformationComponent = mEntityComponentManager->GetComponent<TransformationComponent>(referenceEntityId);
    auto& thisEntityPhysicsComponent = mEntityComponentManager->GetComponent<PhysicsComponent>(referenceEntityId);

    const auto& thisEntityHitBox = thisEntityPhysicsComponent.GetHitBox();
    const auto& otherPhysicsComponent = mEntityComponentManager->GetComponent<PhysicsComponent>(collidedWithEntityId);
    const auto& otherEntityHitBox = otherPhysicsComponent.GetHitBox();
    const auto& otherEntityTransf = mEntityComponentManager->GetComponent<TransformationComponent>(collidedWithEntityId);
    
    if (axis == Axis::X_AXIS)
    {
        // Collided with entity to the right
        if (thisEntityTransformationComponent.GetTranslation().x < otherEntityTransf.GetTranslation().x)
        {
            //if (Abs(-thisEntityTransformationComponent.GetTranslation().x + otherEntityTransf.GetTranslation().x - otherEntityHitBox.mDimensions.x * 0.5f - thisEntityHitBox.mDimensions.x * 0.5f) < 15.0f)
            //{
                Log(LogType::INFO, "Collided with right, will be moved by %.2f", (-thisEntityTransformationComponent.GetTranslation().x + otherEntityTransf.GetTranslation().x - otherEntityHitBox.mDimensions.x * 0.5f - thisEntityHitBox.mDimensions.x * 0.5f));
                thisEntityTransformationComponent.GetTranslation().x = otherEntityTransf.GetTranslation().x - otherEntityHitBox.mDimensions.x * 0.5f - thisEntityHitBox.mDimensions.x * 0.5f;
            //}
        }
        // Collided with entity to the left
        else
        {
            Log(LogType::INFO, "Collided with left, will be moved by %.2f", (-thisEntityTransformationComponent.GetTranslation().x + otherEntityTransf.GetTranslation().x + otherEntityHitBox.mDimensions.x * 0.5f + thisEntityHitBox.mDimensions.x * 0.5f));
            thisEntityTransformationComponent.GetTranslation().x = otherEntityTransf.GetTranslation().x + otherEntityHitBox.mDimensions.x * 0.5f + thisEntityHitBox.mDimensions.x * 0.5f;
        }
    }
    else
    {
        // Collided with entity above
        if (thisEntityTransformationComponent.GetTranslation().y < otherEntityTransf.GetTranslation().y)
        {
            //if (Abs(-thisEntityTransformationComponent.GetTranslation().y + otherEntityTransf.GetTranslation().y - otherEntityHitBox.mDimensions.y * 0.5f - thisEntityHitBox.mDimensions.y * 0.5f) < 15.0f)
            //{
                Log(LogType::INFO, "Collided with above, will be moved by %.2f", (-thisEntityTransformationComponent.GetTranslation().y + otherEntityTransf.GetTranslation().y - otherEntityHitBox.mDimensions.y * 0.5f - thisEntityHitBox.mDimensions.y * 0.5f));
                thisEntityTransformationComponent.GetTranslation().y = otherEntityTransf.GetTranslation().y - otherEntityHitBox.mDimensions.y * 0.5f - thisEntityHitBox.mDimensions.y * 0.5f;
                thisEntityPhysicsComponent.GetVelocity().y = 0.0f;
            //}
        }
        // Collided with entity below
        else
        {
            //if (Abs(-thisEntityTransformationComponent.GetTranslation().y + otherEntityTransf.GetTranslation().y + otherEntityHitBox.mDimensions.y * 0.5f + thisEntityHitBox.mDimensions.y * 0.5f) < 15.0f)
            //{
                Log(LogType::INFO, "Collided with below, will be moved by %.2f", (-thisEntityTransformationComponent.GetTranslation().y + otherEntityTransf.GetTranslation().y + otherEntityHitBox.mDimensions.y * 0.5f + thisEntityHitBox.mDimensions.y * 0.5f));
                thisEntityTransformationComponent.GetTranslation().y = otherEntityTransf.GetTranslation().y + otherEntityHitBox.mDimensions.y * 0.5f + thisEntityHitBox.mDimensions.y * 0.5f;
            //}
        }
    }
    
}
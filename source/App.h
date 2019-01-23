//
//  App.h
//  Hardcore2D
//
//  Created by Alex Koukoulas on 10/01/2019.
//

#ifndef App_h
#define App_h

#include "util/TypeTraits.h"

#include <memory>
#include <vector>

class ServiceLocator;
class EntityComponentManager;
class EventCommunicationService;
class CoreRenderingService;
class AnimationService;
class ResourceManager;
class PhysicsSystem;
class InputHandler;
class IInputActionConsumer;

union SDL_Event;

class App final
{
public:
    App();
    ~App();
    
    bool Initialize();
    void Run();
    
private:
    void Update(const float dt);
	void HandleInput();

    std::unique_ptr<ServiceLocator> mServiceLocator;
    std::unique_ptr<EntityComponentManager> mEntityComponentManager;
    std::unique_ptr<EventCommunicationService> mEventCommunicationService;
    std::unique_ptr<CoreRenderingService> mCoreRenderingService;
    std::unique_ptr<AnimationService> mAnimationService;
    std::unique_ptr<ResourceManager> mResourceManager;
    std::unique_ptr<InputHandler> mInputHandler;
    std::unique_ptr<PhysicsSystem> mPhysicsSystem;
    
    std::vector<std::unique_ptr<IInputActionConsumer>> mInputActionConsumers;
    
    std::vector<EntityId> mActiveEntityIds;
};

#endif /* App_h */

//
//  App.cpp
//  Hardcore2D
//
//  Created by Alex Koukoulas on 10/01/2019.
//

#include "App.h"
#include "rendering/Camera.h"
#include "input/PlayerInputActionConsumer.h"
#include "input/DebugInputActionConsumer.h"
#include "components/EntityComponentManager.h"
#include "events/EventCommunicationService.h"
#include "rendering/CoreRenderingService.h"
#include "rendering/AnimationService.h"
#include "physics/PhysicsSystem.h"
#include "ServiceLocator.h"
#include "resources/ResourceManager.h"
#include "util/Logging.h"
#include "util/FileUtils.h"
#include "gl/Context.h"
#include "resources/TextureResource.h"
#include "input/InputHandler.h"
#include "game/Level.h"
#include "game/LevelFactory.h"
#include "util/TypeTraits.h"
#include "util/StringId.h"

#include <vector>

App::App()
{
}

App::~App()
{
    
}

void App::Run()
{
	mCoreRenderingService->GameLoop([this](const float dt) { Update(dt); });
}

bool App::Initialize()
{
    // Initialize services
    mServiceLocator = std::unique_ptr<ServiceLocator>(new ServiceLocator);
    mEntityComponentManager = std::unique_ptr<EntityComponentManager>(new EntityComponentManager);
    mEventCommunicationService = std::unique_ptr<EventCommunicationService>(new EventCommunicationService);
    mResourceManager = std::unique_ptr<ResourceManager>(new ResourceManager());
    mCoreRenderingService = std::unique_ptr<CoreRenderingService>(new CoreRenderingService(*mServiceLocator));
    mAnimationService = std::unique_ptr<AnimationService>(new AnimationService(*mServiceLocator));
    mPhysicsSystem = std::unique_ptr<PhysicsSystem>(new PhysicsSystem(*mServiceLocator));
    mInputHandler = std::unique_ptr<InputHandler>(new InputHandler());

    // Register services
    mServiceLocator->RegisterService<EntityComponentManager>(mEntityComponentManager.get());
    mServiceLocator->RegisterService<EventCommunicationService>(mEventCommunicationService.get());
    mServiceLocator->RegisterService<CoreRenderingService>(mCoreRenderingService.get());
    mServiceLocator->RegisterService<PhysicsSystem>(mPhysicsSystem.get());
    mServiceLocator->RegisterService<ResourceManager>(mResourceManager.get());
    mServiceLocator->RegisterService<InputHandler>(mInputHandler.get());

    // 2nd step service initialization
    mPhysicsSystem->Initialize();
    if (!mCoreRenderingService->InitializeEngine()) return false;
    if (!mResourceManager->InitializeResourceLoaders()) return false;
    
    // Parse Level
    LevelFactory levelFactory(*mServiceLocator);
    mLevel = levelFactory.CreateLevel("1.json");
    
    // Initialize camera
    mCamera = std::make_unique<Camera>(*mServiceLocator, mCoreRenderingService->GetRenderableDimensions(), mLevel->GetHorizontalBounds(), mLevel->GetVerticalBounds());
    mCoreRenderingService->AttachCamera(mCamera.get());
    
    // Initialized in order of priority
    mInputActionConsumers.push_back(std::make_unique<DebugInputActionConsumer>(*mServiceLocator));
    mInputActionConsumers.push_back(std::make_unique<PlayerInputActionConsumer>(*mServiceLocator, mLevel->GetEntityIdFromName(StringId("player"))));
    
    return true;
}


void App::Update(const float dt)
{    
    HandleInput();  
    mPhysicsSystem->UpdateEntities(mLevel->GetAllActiveEntities(), dt);
    mAnimationService->UpdateAnimations(mLevel->GetAllActiveEntities(), dt);
    mCamera->Update(mLevel->GetEntityIdFromName(StringId("player")), dt);
    mCoreRenderingService->RenderEntities(mLevel->GetAllActiveEntities());
}

void App::HandleInput()
{
    const auto inputActions = mInputHandler->TranslateInputToActions();
    for (const auto inputAction: inputActions)
    {
        for (const auto& inputActionConsumer: mInputActionConsumers)
        {
            if (inputActionConsumer->VConsumeInputAction(inputAction))
                break;
        }
    }
}


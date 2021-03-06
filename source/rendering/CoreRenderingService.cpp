//
//  CoreRenderingService.cpp
//  Hardcore2D
//
//  Created by Alex Koukoulas on 10/01/2019.
//

#include "CoreRenderingService.h"
#include "Shader.h"
#include "Camera.h"
#include "../ServiceLocator.h"
#include "../util/StringUtils.h"
#include "../util/ShaderUtils.h"
#include "../util/FileUtils.h"
#include "../util/Logging.h"
#include "../util/SDLMessageBoxUtils.h"
#include "../util/MathUtils.h"
#include "../gl/Context.h"
#include "../resources/ResourceManager.h"
#include "../components/TransformComponent.h"
#include "../components/EntityComponentManager.h"
#include "../components/FactionComponent.h"
#include "../components/AnimationComponent.h"
#include "../components/ShaderComponent.h"
#include "../components/PhysicsComponent.h"
#include "../resources/TextureResource.h"
#include "../resources/TextFileResource.h"
#include "../events/EventCommunicationService.h"
#include "../events/EventCommunicator.h"
#include "../events/DebugToggleHitboxDisplayEvent.h"
#include "../events/DebugToggleSceneGraphDisplayEvent.h"
#include "../physics/PhysicsSystem.h"

#include <string>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>
#include <unordered_map>

static const std::string SHADER_DIRECTORY = "shaders/";

static const GLfloat QUAD_VERTICES[] =
{
    -1.01f, -1.01f, 0.0f, 0.01f, 0.01f,
     1.01f, -1.01f, 0.0f, 0.99f, 0.01f,
     1.01f,  1.01f, 0.0f, 0.99f, 0.99f,
    -1.01f,  1.01f, 0.0f, 0.01f, 0.99f
};

static const std::unordered_map<FactionGroup, std::string> sFactionGroupToTextureName = 
{    
    { FactionGroup::AMBIENT, "debug/debug_square_green.png" },
    { FactionGroup::ALLIES, "debug/debug_square_cyan.png" },
    { FactionGroup::ENEMIES, "debug/debug_square_pink.png" },
};

CoreRenderingService::CoreRenderingService(const ServiceLocator& serviceLocator)
    : mServiceLocator(serviceLocator)
    , mEntityComponentManager(nullptr)    
    , mEventCommunicator(nullptr)
    , mSdlWindow(nullptr)
    , mSdlGLContext(nullptr)
    , mDebugHitboxDisplay(false)
    , mDebugSceneGraphDisplay(false)
    , mRunning(false)
    , mRenderableDimensions(0.0f, 0.0f)
    , mSwirlAngle(0.0f)
    , mSceneBlurIntensity(0.0f)
    , mVAO(0U)
    , mVBO(0U)
    , mFrameBufferId(0U)
    , mScreenRenderingTexture(0U)
    , mCurrentShader("basic")

{
    
}

CoreRenderingService::~CoreRenderingService()
{
    GL_CHECK(glDeleteVertexArrays(1, &mVAO));
    GL_CHECK(glDeleteBuffers(1, &mVBO));
    
    SDL_GL_MakeCurrent(nullptr, nullptr);
    SDL_GL_DeleteContext(mSdlGLContext);
    SDL_DestroyWindow(mSdlWindow);
    SDL_Quit();
}

bool CoreRenderingService::VInitialize()
{
    mEntityComponentManager = &(mServiceLocator.ResolveService<EntityComponentManager>());    
    mResourceManager = &(mServiceLocator.ResolveService<ResourceManager>());
    mEventCommunicator = mServiceLocator.ResolveService<EventCommunicationService>().CreateEventCommunicator();
    
    if (!InitializeContext()) return false;    
    InitializeRenderingPrimitive();
    CompileAllShaders();
    RegisterEventCallbacks();
    
    return true;
}

void CoreRenderingService::AttachCamera(const Camera* camera)
{
    mAttachedCamera = camera;
}

void CoreRenderingService::GameLoop(std::function<void(const float)> appUpdateMethod)
{
    SDL_Event event;    
    float elapsedTicks = 0.0f;
    float dtAccumulator = 0.0f;
    long framesAccumulator = 0;
    mRunning = true;
    
    while(mRunning)
    {
        // Poll events
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
                case SDL_QUIT: mRunning = false; break;
                case SDL_WINDOWEVENT:
                {
                    switch (event.window.event)
                    {
                        case SDL_WINDOWEVENT_FOCUS_GAINED:
                            Log(LogType::INFO, "Window %d gained keyboard focus", event.window.windowID);
                        break;
                        case SDL_WINDOWEVENT_FOCUS_LOST:
                            Log(LogType::INFO, "Window %d lost keyboard focus", event.window.windowID);
                        break;
                    }                    
                } break;                
            }
        }
        
        // Calculate frame delta
        const auto currentTicks = static_cast<float>(SDL_GetTicks());
        auto lastFrameTicks = currentTicks - elapsedTicks;
        elapsedTicks = currentTicks;
        const auto dt = lastFrameTicks * 0.001f;
        framesAccumulator++;
        dtAccumulator += dt;
        
#ifndef NDEBUG
        if (dtAccumulator > 1.0f)
        {
            const auto windowTitle = "FPS: " + std::to_string(framesAccumulator) + mFrameStatisticsMessage;
            SDL_SetWindowTitle(mSdlWindow, windowTitle.c_str());
            framesAccumulator = 0;
            dtAccumulator = 0.0f;
        }
#endif        
        // Execute first pass rendering
        GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, mFrameBufferId));
        GL_CHECK(glClearColor(0.1f, 0.1f, 0.1f, 1.0f));
        GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
        GL_CHECK(glViewport(0, 0, static_cast<GLsizei>(mRenderableDimensions.x), static_cast<GLsizei>(mRenderableDimensions.y)));
        GL_CHECK(glBindVertexArray(mVAO));
        
        // Clear entities unaffected by post-processing (main menu texture etc)
        mEntitiesUnaffectedByPostProcessing.clear();
        
        // Update client
        appUpdateMethod(Min(dt, 0.05f));
        
        // Clear buffers for post-processing
        GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));
        GL_CHECK(glClearColor(1.0f, 1.0f, 1.0f, 1.0f));
        GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
        
        PreparePostProcessingPass();
        
        // Execute prost-processing pass
        GL_CHECK(glBindTexture(GL_TEXTURE_2D, mScreenRenderingTexture));
        GL_CHECK(glBindVertexArray(mVAO));
        GL_CHECK(glDrawArrays(GL_TRIANGLE_FAN, 0, 4));

        // Render all top layer entities unaffected by post-processing
        RenderEntitiesUnaffectedByPostProcessing();
        
        // Swap window buffers
        SDL_GL_SwapWindow(mSdlWindow);
    }
}

void CoreRenderingService::RenderEntities(const std::vector<EntityId>& entityIds)
{
    for (const auto entityId: entityIds)
    {
        if (mEntityComponentManager->HasComponent<ShaderComponent>(entityId))
        {
            if (mEntityComponentManager->GetComponent<ShaderComponent>(entityId).IsAffectedByPostProcessing())
            {
                RenderEntityInternal(entityId);
            }
            else
            {
                mEntitiesUnaffectedByPostProcessing.push_back(entityId);
            }
        }
    }
}

void CoreRenderingService::RenderEntities(const std::vector<EntityNameIdEntry>& entityIds)
{
    for (const auto entityEntry: entityIds)
    {
        if (mEntityComponentManager->HasComponent<ShaderComponent>(entityEntry.mEntityId))
        {
            if (mEntityComponentManager->GetComponent<ShaderComponent>(entityEntry.mEntityId).IsAffectedByPostProcessing())
            {
                RenderEntityInternal(entityEntry.mEntityId);
            }
            else
            {
                mEntitiesUnaffectedByPostProcessing.push_back(entityEntry.mEntityId);
            }
        }
    }
    
    RenderPhysicsDebugRectangles(entityIds);
}

void CoreRenderingService::SetFrameStatisticsMessage(const std::string& frameStatisticsMessage)
{
    mFrameStatisticsMessage = frameStatisticsMessage;
}

void CoreRenderingService::SetSceneBlurIntensity(const float blurIntensity)
{
    mSceneBlurIntensity = blurIntensity;
}

const glm::vec2& CoreRenderingService::GetRenderableDimensions() const
{
    return mRenderableDimensions;
}

float CoreRenderingService::GetAspectRatio() const
{
    return mRenderableDimensions.x/mRenderableDimensions.y;
}

bool CoreRenderingService::InitializeContext()
{
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO) < 0)
    {
        ShowMessageBox(SDL_MESSAGEBOX_ERROR, "Error initializing SDL", "An error has occurred while trying to initialize SDL");
        return false;
    }
    
    // Set SDL GL attributes        
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);    
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
   
#if DEMO_ENABLE_MULTISAMPLE
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 2);
#endif
    
    // Get Current Display Mode Resolution
    SDL_DisplayMode displayMode;
    SDL_GetCurrentDisplayMode(0, &displayMode);
    
    //const auto defaultWindowWidth = static_cast<int>(displayMode.w * 0.66f);
    //const auto defaultWindowHeight = static_cast<int>(displayMode.h * 0.66f);
	const auto defaultWindowWidth = 1280;
	const auto defaultWindowHeight = 720;

    // Create SDL window
    mSdlWindow = SDL_CreateWindow("Hardcore2D", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, defaultWindowWidth, defaultWindowHeight, SDL_WINDOW_OPENGL);
    
    if (mSdlWindow == nullptr)
    {
        ShowMessageBox(SDL_MESSAGEBOX_ERROR, "Error creating SDL window", "An error has occurred while trying to create an SDL_Window");
        return false;
    }
    
    // Make window non-resizable
    SDL_SetWindowResizable(mSdlWindow, SDL_FALSE);
    
    SDL_ShowWindow(mSdlWindow);
    
    // Create SDL GL context
    mSdlGLContext = SDL_GL_CreateContext(mSdlWindow);
    if (mSdlGLContext == nullptr)
    {
        ShowMessageBox(SDL_MESSAGEBOX_ERROR, "Error creating SDL context", "An error has occurred while trying to create an SDL_Context");
        return false;
    }
    
    SDL_GL_MakeCurrent(mSdlWindow, mSdlGLContext);
    SDL_GL_SetSwapInterval(0);
    
#ifdef _WIN32
    // Initialize GLES2 function table
    glFuncTable.initialize();
#endif
    
    // Get render buffer width/height
	int rendWidth = 0;
	int rendHeight = 0;
    SDL_GL_GetDrawableSize(mSdlWindow, &rendWidth, &rendHeight);
    mRenderableDimensions = glm::vec2(static_cast<float>(rendWidth), static_cast<float>(rendHeight));
    mProjectionMatrix = glm::orthoLH(0.0f, mRenderableDimensions.x, 0.0f, mRenderableDimensions.y, 0.001f, 100.0f);
    
    // Log GL driver info
    Log(LogType::INFO, "Vendor     : %s", GL_NO_CHECK(glGetString(GL_VENDOR)));
    Log(LogType::INFO, "Renderer   : %s", GL_NO_CHECK(glGetString(GL_RENDERER)));
    Log(LogType::INFO, "Version    : %s", GL_NO_CHECK(glGetString(GL_VERSION)));
    
    // Configure Blending
    GL_CHECK(glEnable(GL_BLEND));
    GL_CHECK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    
    // Create Frame Buffer
    GL_CHECK(glGenFramebuffers(1, &mFrameBufferId));
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, mFrameBufferId));

    // Create Frame Buffer Texture
    GL_CHECK(glGenTextures(1, &mScreenRenderingTexture));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, mScreenRenderingTexture));
    GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, static_cast<GLsizei>(mRenderableDimensions.x), static_cast<GLsizei>(mRenderableDimensions.y), 0, GL_RGB, GL_UNSIGNED_BYTE, 0));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    GL_CHECK(glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, mScreenRenderingTexture, 0));
    
    if (GL_NO_CHECK(glCheckFramebufferStatus(GL_FRAMEBUFFER)) != GL_FRAMEBUFFER_COMPLETE)
    {
        assert(false);
    }

    return true;
}

void CoreRenderingService::InitializeRenderingPrimitive()
{
    // Create VAO & VBO
    GL_CHECK(glGenVertexArrays(1, &mVAO));
    GL_CHECK(glGenBuffers(1, &mVBO));
    
    // Bind VAO & VBO
    GL_CHECK(glBindVertexArray(mVAO));
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, mVBO));
    
    // Fill in VBO
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER, sizeof(QUAD_VERTICES), QUAD_VERTICES, GL_STATIC_DRAW));
    
    // Set Attribute Pointers
    GL_CHECK(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(QUAD_VERTICES)/4, nullptr));
    GL_CHECK(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(QUAD_VERTICES)/4, (void*)(sizeof(float) * 3)));
    
    // Enable vertex attribute arrays
    GL_CHECK(glEnableVertexAttribArray(0));
    GL_CHECK(glEnableVertexAttribArray(1));
}

void CoreRenderingService::CompileAllShaders()
{	
	auto& resourceManager = mServiceLocator.ResolveService<ResourceManager>();

	auto currentVertexShaderId = 0;
	auto currentFragmentShaderId = 0;
	auto currentProgramId = 0;
    std::vector<std::string> shaderUniformNames;
    
    const auto shaderFileNames = GetAllFilenamesInDirectory(ResourceManager::RES_ROOT + SHADER_DIRECTORY);
    
    for (const auto fileName: shaderFileNames)
	{
        const auto fileExtension = GetFileExtension(fileName);
		if (fileExtension == "vs")
		{
			// Generate vertex shader id
			currentVertexShaderId = GL_NO_CHECK(glCreateShader(GL_VERTEX_SHADER));

			// Get vertex shader contents
			const auto vertexShaderFileResourceId = resourceManager.LoadResource(SHADER_DIRECTORY + fileName);
			const auto& vertexShaderFileResource = resourceManager.GetResource<TextFileResource&>(vertexShaderFileResourceId);
			const char* vertexShaderFileContents = vertexShaderFileResource.GetContents().c_str();
            
			// Compile vertex shader
			GL_CHECK(glShaderSource(currentVertexShaderId, 1, &vertexShaderFileContents, NULL));
			GL_CHECK(glCompileShader(currentVertexShaderId));

			// Check vertex shader compilation
			std::string infoLog;
			GLint infoLogLength;
			GL_CHECK(glGetShaderiv(currentVertexShaderId, GL_INFO_LOG_LENGTH, &infoLogLength));
			if (infoLogLength > 0)
			{
				infoLog.clear();
				infoLog.reserve(infoLogLength);
				GL_CHECK(glGetShaderInfoLog(currentVertexShaderId, infoLogLength, NULL, &infoLog[0]));
				Log(LogType::INFO, "While compiling vertex shader:\n%s", infoLog.c_str());
			}

			// Link shader program
			currentProgramId = GL_NO_CHECK(glCreateProgram());
			GL_CHECK(glAttachShader(currentProgramId, currentVertexShaderId));
			GL_CHECK(glAttachShader(currentProgramId, currentFragmentShaderId));
			GL_CHECK(glLinkProgram(currentProgramId));

#ifndef _WIN32
			glGetProgramiv(currentProgramId, GL_INFO_LOG_LENGTH, &infoLogLength);
			if (infoLogLength > 0)
			{
				infoLog.clear();
				infoLog.reserve(infoLogLength);
				GL_CHECK(glGetProgramInfoLog(currentProgramId, infoLogLength, NULL, &infoLog[0]));
				Log(LogType::INFO, "While linking shader:\n%s", infoLog.c_str());
			}
#endif

			GL_CHECK(glValidateProgram(currentProgramId));

#ifndef _WIN32
            GLint status;
			GL_CHECK(glGetProgramiv(currentProgramId, GL_VALIDATE_STATUS, &status));
			glGetProgramiv(currentProgramId, GL_INFO_LOG_LENGTH, &infoLogLength);
			if (infoLogLength > 0)
			{
				infoLog.clear();
				infoLog.reserve(infoLogLength);
				GL_CHECK(glGetProgramInfoLog(currentProgramId, infoLogLength, NULL, &infoLog[0]));
				Log(LogType::INFO, "While validating shader:\n%s", infoLog.c_str());
			}
#endif

			// Destroy intermediate compiled shaders
			GL_CHECK(glDetachShader(currentProgramId, currentVertexShaderId));
			GL_CHECK(glDetachShader(currentProgramId, currentFragmentShaderId));
			GL_CHECK(glDeleteShader(currentVertexShaderId));
			GL_CHECK(glDeleteShader(currentFragmentShaderId));
            
            // Extract vertex shader uniform names
            const auto uniformNames = GetUniformNames(vertexShaderFileResource.GetContents());
            shaderUniformNames.insert(shaderUniformNames.end(), uniformNames.begin(), uniformNames.end());
            
            // Extract uniform locations
            std::unordered_map<StringId, GLuint, StringIdHasher> shaderUniformNamesToLocations;
            for (const auto uniformName: shaderUniformNames)
            {
                shaderUniformNamesToLocations[uniformName] = GL_NO_CHECK(glGetUniformLocation(currentProgramId, uniformName.c_str()));
            }
            
			// Create shader object
			const auto programName = GetFileNameWithoutExtension(fileName);
            
            // Save shader
            mShaders[programName] = std::make_unique<Shader>(currentProgramId, shaderUniformNamesToLocations);
		}
		else if (fileExtension == "fs")
		{
			// Generate fragment shader id
			currentFragmentShaderId = GL_NO_CHECK(glCreateShader(GL_FRAGMENT_SHADER));

			// Get fragment shader contents			
			const auto fragmentShaderFileResourceId = resourceManager.LoadResource(SHADER_DIRECTORY + fileName);
			const auto& fragmentShaderFileResource = resourceManager.GetResource<TextFileResource&>(fragmentShaderFileResourceId);
			const char* fragmentShaderFileContents = fragmentShaderFileResource.GetContents().c_str();
			
			GL_CHECK(glShaderSource(currentFragmentShaderId, 1, &fragmentShaderFileContents, NULL));
			GL_CHECK(glCompileShader(currentFragmentShaderId));

			std::string infoLog;
			GLint infoLogLength;
			GL_CHECK(glGetShaderiv(currentFragmentShaderId, GL_INFO_LOG_LENGTH, &infoLogLength));
			if (infoLogLength > 0)
			{
				infoLog.clear();
				infoLog.reserve(infoLogLength);
				GL_CHECK(glGetShaderInfoLog(currentFragmentShaderId, infoLogLength, NULL, &infoLog[0]));
				Log(LogType::INFO, "While compiling fragment shader:\n%s", infoLog.c_str());
			}
            
            // Extract fragment shader uniform names
            shaderUniformNames.clear();
            const auto uniformNames = GetUniformNames(fragmentShaderFileResource.GetContents());
            shaderUniformNames.insert(shaderUniformNames.end(), uniformNames.begin(), uniformNames.end());
		}		
	}
}

void CoreRenderingService::RegisterEventCallbacks()
{
    mEventCommunicator->RegisterEventCallback<DebugToggleHitboxDisplayEvent>([this](const IEvent&)
    {
        mDebugHitboxDisplay = !mDebugHitboxDisplay;
    });
    mEventCommunicator->RegisterEventCallback<DebugToggleSceneGraphDisplayEvent>([this](const IEvent&)
    {
        mDebugSceneGraphDisplay = !mDebugSceneGraphDisplay;
    });
}

void CoreRenderingService::RenderOutlineRectangles(const std::list<std::pair<glm::vec2, glm::vec2>>& rectangles)
{
    mCurrentShader = StringId("basic");
    GL_CHECK(glUseProgram(mShaders[mCurrentShader]->GetShaderId()));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, mResourceManager->GetResource<TextureResource>("debug/debug_outline_square.png").GetGLTextureId()));
    GL_CHECK(glUniformMatrix4fv(mShaders[mCurrentShader]->GetUniformNamesToLocations().at(StringId("view")), 1, GL_FALSE, (GLfloat*)&(mAttachedCamera->GetViewMatrix())));
    GL_CHECK(glUniformMatrix4fv(mShaders[mCurrentShader]->GetUniformNamesToLocations().at(StringId("proj")), 1, GL_FALSE, (GLfloat*)&mProjectionMatrix));
    
    for (const auto& rectangleInfoPair: rectangles)
    {
        glm::mat4 worldMatrix(1.0f);
        worldMatrix = glm::translate(worldMatrix, glm::vec3(rectangleInfoPair.first.x + rectangleInfoPair.second.x * 0.5f, rectangleInfoPair.first.y + rectangleInfoPair.second.y * 0.5f, 1.0f));
        worldMatrix = glm::scale(worldMatrix, glm::vec3(rectangleInfoPair.second.x * 0.5f, rectangleInfoPair.second.y * 0.5f, 1.0));
        
        GL_CHECK(glUniformMatrix4fv(mShaders[mCurrentShader]->GetUniformNamesToLocations().at(StringId("world")), 1, GL_FALSE, (GLfloat*)&worldMatrix));
        GL_CHECK(glDrawArrays(GL_TRIANGLE_FAN, 0, 4));
    }
}

void CoreRenderingService::RenderEntityInternal(const EntityId entityId)
{
    if (mEntityComponentManager->HasComponent<AnimationComponent>(entityId) == false)
    {
        return;
    }
    
    mCurrentShader = mEntityComponentManager->GetComponent<ShaderComponent>(entityId).GetShaderName();
    GL_CHECK(glUseProgram(mShaders[mCurrentShader]->GetShaderId()));
    
    const auto& shaderUniforms = mShaders[mCurrentShader]->GetUniformNamesToLocations();
    
    if (shaderUniforms.count(StringId("view")) != 0)
    {
        GL_CHECK(glUniformMatrix4fv(mShaders[mCurrentShader]->GetUniformNamesToLocations().at(StringId("view")), 1, GL_FALSE, (GLfloat*)&(mAttachedCamera->GetViewMatrix())));
    }
    
    if (shaderUniforms.count(StringId("proj")) != 0)
    {
        GL_CHECK(glUniformMatrix4fv(mShaders[mCurrentShader]->GetUniformNamesToLocations().at(StringId("proj")), 1, GL_FALSE, (GLfloat*)&mProjectionMatrix));
    }
    
    const auto& animationComponent = mEntityComponentManager->GetComponent<AnimationComponent>(entityId);
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, animationComponent.GetCurrentFrameResourceId()));
    
    if (StringStartsWith(mCurrentShader.GetString(), "basic"))
    {
        const auto textureFlip = animationComponent.GetCurrentFacingDirection() == FacingDirection::LEFT ? 1 : 0;
        GL_CHECK(glUniform1i(mShaders[mCurrentShader]->GetUniformNamesToLocations().at(StringId("flip_tex_hor")), textureFlip));
    }
    
    const auto& transformComponent = mEntityComponentManager->GetComponent<TransformComponent>(entityId);
    
    // Camera view rect rejection test (with backgrounds excluded from test)
    if (mCurrentShader != StringId("background") && mAttachedCamera->IsTransformInsideViewRect(transformComponent) == false)
    {
        return;
    }
    
    glm::mat4 translationMatrix(1.0f);
    translationMatrix = glm::translate(translationMatrix, transformComponent.GetTranslation());
    
    auto animationDisplacement = animationComponent.GetSpecificAnimationDisplacement(animationComponent.GetCurrentAnimation());
    if (animationComponent.GetCurrentFacingDirection() == FacingDirection::LEFT)
    {
        animationDisplacement.x = -animationDisplacement.x;
    }
    
    translationMatrix = glm::translate(translationMatrix, glm::vec3(animationDisplacement.x, animationDisplacement.y, 0.0f));
    
    glm::mat4 rotationMatrix(1.0f);
    glm::vec3 rotation = transformComponent.GetRotation();
    rotationMatrix = glm::rotate(rotationMatrix, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
    rotationMatrix = glm::rotate(rotationMatrix, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
    rotationMatrix = glm::rotate(rotationMatrix, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
    
    glm::mat4 scaleMatrix(1.0f);
    scaleMatrix = glm::scale(scaleMatrix, transformComponent.GetScale() * 0.5f);
    
    glm::mat4 worldMatrix(1.0f);
    worldMatrix = translationMatrix * rotationMatrix * scaleMatrix;
    GL_CHECK(glUniformMatrix4fv(mShaders[mCurrentShader]->GetUniformNamesToLocations().at(StringId("world")), 1, GL_FALSE, (GLfloat*)&worldMatrix));
    
    
    GL_CHECK(glDrawArrays(GL_TRIANGLE_FAN, 0, 4));
}

void CoreRenderingService::RenderPhysicsDebugRectangles(const std::vector<EntityNameIdEntry>& entities)
{
    for (const auto& entityEntry: entities)
    {
        const auto entityId = entityEntry.mEntityId;
        if (mDebugHitboxDisplay &&
            mEntityComponentManager->HasComponent<PhysicsComponent>(entityId) &&
            mEntityComponentManager->HasComponent<FactionComponent>(entityId))
        {
            const auto& physicsComponent = mEntityComponentManager->GetComponent<PhysicsComponent>(entityId);
            const auto& factionComponent = mEntityComponentManager->GetComponent<FactionComponent>(entityId);
            const auto& transformComponent = mEntityComponentManager->GetComponent<TransformComponent>(entityId);
            
            glm::mat4 worldMatrix = glm::mat4(1.0f);
            const auto& entityTranslation = transformComponent.GetTranslation();
            const auto& entityRotation = transformComponent.GetRotation();
            
            worldMatrix = glm::translate(worldMatrix, glm::vec3(entityTranslation.x + physicsComponent.GetHitBox().mCenterPoint.x, entityTranslation.y + physicsComponent.GetHitBox().mCenterPoint.y, 1.0f));
            worldMatrix = glm::rotate(worldMatrix, entityRotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
            worldMatrix = glm::rotate(worldMatrix, entityRotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
            worldMatrix = glm::rotate(worldMatrix, entityRotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
            worldMatrix = glm::scale(worldMatrix, glm::vec3(physicsComponent.GetHitBox().mDimensions.x * 0.5f, physicsComponent.GetHitBox().mDimensions.y * 0.5f, 1.0));
            
            mCurrentShader = StringId("debug_rect");
            GL_CHECK(glUseProgram(mShaders[mCurrentShader]->GetShaderId()));
            GL_CHECK(glUniformMatrix4fv(mShaders[mCurrentShader]->GetUniformNamesToLocations().at(StringId("world")), 1, GL_FALSE, (GLfloat*)&worldMatrix));
            GL_CHECK(glBindTexture(GL_TEXTURE_2D, (mResourceManager->GetResource<TextureResource>(sFactionGroupToTextureName.at(factionComponent.GetFactionGroup()))).GetGLTextureId()));
            GL_CHECK(glUniformMatrix4fv(mShaders[mCurrentShader]->GetUniformNamesToLocations().at(StringId("view")), 1, GL_FALSE, (GLfloat*)&(mAttachedCamera->GetViewMatrix())));
            GL_CHECK(glUniformMatrix4fv(mShaders[mCurrentShader]->GetUniformNamesToLocations().at(StringId("proj")), 1, GL_FALSE, (GLfloat*)&mProjectionMatrix));
            GL_CHECK(glDrawArrays(GL_TRIANGLE_FAN, 0, 4));
        }
    }
}

void CoreRenderingService::PreparePostProcessingPass()
{
    mCurrentShader = StringId("postprocessing");
    GL_CHECK(glUseProgram(mShaders[mCurrentShader]->GetShaderId()));
    
    const auto& currentShaderUniforms = mShaders[mCurrentShader]->GetUniformNamesToLocations();
    GL_CHECK(glUniform1f(currentShaderUniforms.at(StringId("swirlRadius")), 300.0f));
    GL_CHECK(glUniform1f(currentShaderUniforms.at(StringId("swirlAngle")), mSwirlAngle));
    GL_CHECK(glUniform1f(currentShaderUniforms.at(StringId("blurIntensity")), mSceneBlurIntensity));
    
    glm::vec2 swirlCenter(mRenderableDimensions.x * 0.5f, mRenderableDimensions.y * 0.5f);
    GL_CHECK(glUniform2fv(currentShaderUniforms.at(StringId("swirlCenter")), 1, (GLfloat*)&swirlCenter));
    
    glm::vec2 swirlDimensions(mRenderableDimensions.x, mRenderableDimensions.y);
    GL_CHECK(glUniform2fv(currentShaderUniforms.at(StringId("swirlDimensions")), 1, (GLfloat*)&swirlDimensions));
}

void CoreRenderingService::RenderEntitiesUnaffectedByPostProcessing()
{
    for (const auto& entityId: mEntitiesUnaffectedByPostProcessing)
    {
        RenderEntityInternal(entityId);
    }
    
    if (mDebugSceneGraphDisplay)
    {
        RenderOutlineRectangles(mServiceLocator.ResolveService<PhysicsSystem>().GetSceneGraphDebugRectangles());
    }
}

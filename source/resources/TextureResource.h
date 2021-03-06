//
//  TextureResource.h
//  Hardcore2D
//
//  Created by Alex Koukoulas on 13/01/2019.
//

#ifndef TextureResource_h
#define TextureResource_h

#include "IResource.h"

#include <memory>

struct SDL_Surface;
class TextureResource final: public IResource
{
    friend class TextureLoader;
public:
    ~TextureResource();
    
    GLuint GetGLTextureId() const;
    
private:
    TextureResource(SDL_Surface*, GLuint);
    
    SDL_Surface* mSdlSurface;
    GLuint mGLTextureId;
};

#endif /* TextureResource_h */

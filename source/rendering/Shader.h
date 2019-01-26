//
//  Shader.h
//  Hardcore2D
//
//  Created by Alex Koukoulas on 15/01/2019.
//

#ifndef Shader_h
#define Shader_h

#include "../util/TypeTraits.h"
#include "../util/StringId.h"

#include <string>
#include <unordered_map>

class Shader final
{
public:
    Shader(const GLuint shaderId, const std::unordered_map<StringId, GLuint> uniformNamesToLocations);
    
    GLuint GetShaderId() const;
    const std::unordered_map<StringId, GLuint>& GetUniformNamesToLocations() const;
    
private:
    GLuint mShaderId;
    std::unordered_map<StringId, GLuint> mShaderUniformNamesToLocations;
};

#endif /* Shader_h */

//
//  TextFileLoader.cpp
//  Hardcore2D
//
//  Created by Alex Koukoulas on 10/01/2019.
//

#include "TextFileLoader.h"
#include "TextFileResource.h"
#include "../util/SDLMessageBoxUtils.h"

#include <string>
#include <fstream>
#include <streambuf>

bool TextFileLoader::Initialize()
{
    return true;
}

std::unique_ptr<IResource> TextFileLoader::VCreateAndLoadResource(const std::string& resourcePath)
{
    std::ifstream file(resourcePath);
    
    if (!file.good())
    {
        ShowMessageBox(SDL_MESSAGEBOX_ERROR, "File could not be found", resourcePath.c_str());
        return nullptr;
    }
    
    std::string str;
    
    file.seekg(0, std::ios::end);
    str.reserve(static_cast<size_t>(file.tellg()));
    file.seekg(0, std::ios::beg);
    
    str.assign((std::istreambuf_iterator<char>(file)),
               std::istreambuf_iterator<char>());
    
    return std::unique_ptr<IResource>(new TextFileResource(str));
}

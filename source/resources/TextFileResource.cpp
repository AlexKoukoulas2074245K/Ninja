//
//  TextFileResource.cpp
//  Ninja
//
//  Created by Alex Koukoulas on 14/01/2019.
//

#include "TextFileResource.h"

TextFileResource::TextFileResource(const std::string& contents, const ResourceId resourceId)
    : IResource(resourceId)
    , mContents(contents)
{
    
}

const std::string& TextFileResource::GetContents() const
{
    return mContents;
}
#pragma once

#include <iostream>
#include "abstractModule.h"

using namespace std;

class BaseFactory
{
public:
    virtual ~BaseFactory() = default;
    virtual BaseModule* createModule(const ModuleParams& params = {}) const = 0;
private:
    
};
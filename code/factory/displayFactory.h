#pragma once

#include "abstractFactory.h"
#include "abstractModule.h"
#include "display.h"

class DisplayFactory : public BaseFactory {
public:
    BaseModule* createModule(const ModuleParams& params = {}) const override {
        return new Display(params);
    }
};
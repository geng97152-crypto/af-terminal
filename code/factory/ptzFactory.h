#pragma once

#include "abstractFactory.h"
#include "abstractModule.h"
#include "pantilt.h"

class PtzFactory : public BaseFactory {
public:
    BaseModule* createModule(const ModuleParams& params = {}) const override {
        return new Pantilt(params);
    }
};//PtzFactory 是一个云台模块工厂，负责创建并返回 Pantilt 对象。
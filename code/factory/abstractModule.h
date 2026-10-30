#pragma once

#include "publisher.h"
#include "observer.h"
#include "string"
#include "observer.h"
#include "publisher.h"

struct VideoParams {
    int pipeId;
    int viChannelId;
    int vencChannelId;
    int video_width;
    int video_height;
};

struct ModuleParams {
    std::string name;
    struct VideoParams videoparams;        
};

// 既支持多产品族的统一接口，又允许单个产品族内部的灵活扩展
// 抽象基类
class BaseModule:public Publisher, public Observer
{
public:
    BaseModule(const ModuleParams& params)
        : Publisher(), Observer(), m_params(params)
    {
        m_name = params.name;
    }

    virtual ~BaseModule() = default;

protected:
    ModuleParams m_params;
    string m_name;
};


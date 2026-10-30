#ifndef __OBSERVER_H__
#define __OBSERVER_H__

#include <iostream>
#include "publisher.h"
#include <string>
#include <algorithm>
#include <list>

using namespace std;

class Publisher;

// 抽象观察者类
class Observer {
public:

    // 只创建观察者，不订阅
    Observer();

    // 支持只订阅一个发布者的构造
    Observer(Publisher* publisher);

    // 支持订阅多个发布者
    Observer(const std::list<Publisher*>& publishers);
    
    virtual ~Observer();

    // 添加订阅指定发布者
    void subscribe(Publisher* publisher);

    // 取消订阅所有发布者
    void unsubscribeAll();

    // 取消订阅指定发布者
    void unsubscribe(Publisher* publisher);
    

    // 更新消息
    virtual void update(Publisher* publisher, string msg) = 0;


protected:
    list<Publisher*> m_publishers;      // 订阅的发布者列表
};
#endif
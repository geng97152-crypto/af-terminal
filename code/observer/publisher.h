#ifndef _PUBLISHER_H
#define _PUBLISHER_H

#include <string>
#include <vector>
#include <list>
#include "observer.h"

using namespace std;

class Observer;

// 发布者基类
class Publisher
{ 
public:
    Publisher();
    virtual ~Publisher() {}         // 基类，虚析构，多态时能通过基类指针析构各个子类的析构函数

    // 1. 添加订阅者
    void attach(Observer* observer);
    // 2. 删除订阅者
    void detach(Observer* observer);
    // 3. 通知订阅者 不同子类发送消息不一样，定义为存虚，要求必须重写notify函数
    virtual void notify(string msg) = 0;    
    
protected:
    // 订阅者列表 添加删除效率高
    list<Observer*> m_observers;
};

#endif
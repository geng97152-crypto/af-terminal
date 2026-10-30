#include "observer.h"

// 创建观察者，暂不订阅
Observer::Observer() {}


// 支持只订阅一个发布者的构造
Observer::Observer(Publisher* publisher)
{
    if (publisher) {
        m_publishers.push_back(publisher);
        publisher->attach(this);
    }
}

// 支持多个订阅
Observer::Observer(const std::list<Publisher*>& publishers)
{
    for (auto* pub : publishers) {
        if (pub) {
            m_publishers.push_back(pub);
            pub->attach(this);
        }
    }
}
Observer::~Observer() {
    unsubscribeAll();
}


// 添加订阅指定发布者
void Observer::subscribe(Publisher* publisher) {
    if (publisher && std::find(m_publishers.begin(), m_publishers.end(), publisher) == m_publishers.end()) {
        m_publishers.push_back(publisher);
        publisher->attach(this);
    }
}

// 取消订阅所有发布者
void Observer::unsubscribeAll() {
    for (auto* pub : m_publishers) {
        if (pub) pub->detach(this);
    }
    m_publishers.clear();
}

 // 取消订阅指定发布者
void Observer::unsubscribe(Publisher* publisher) {
    m_publishers.remove(publisher);
    if (publisher) publisher->detach(this);
}

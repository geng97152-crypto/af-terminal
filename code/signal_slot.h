
#ifndef SIGNAL_SLOT_H
#define SIGNAL_SLOT_H

#include <functional>
#include <vector>
#include <string>
#include <algorithm>
#include <mutex>
#include <memory>

template <typename... Args>//让一个模板可以接收**任意数量、任意类型**的模板参数。
class Signal {
public:
    using Slot = std::function<void(Args...)>;//给接收函数类型取名为 Slot；它接收 Args... 参数，不返回值。

    // 连接成员函数
    template <typename T>
    void connect(T* object, void (T::*method)(Args...)) {
        auto slot = [object, method](Args... args) {
            (object->*method)(args...);  // 调用成员函数
        };
        std::lock_guard<std::mutex> lock(mutex_);
        slots.push_back(std::make_shared<Slot>(slot));  // 存储槽，把刚创建的 lambda 保存到列表里。
    }//接收对象指针 object，以及这个对象的成员函数指针 method。

    // 连接成员函数
    template <typename T>
    void connect(T* object, void (T::*method)(const Args&...)) {
        auto slot = [object, method](Args... args) {
            (object->*method)(args...);  // 调用成员函数
        };
        std::lock_guard<std::mutex> lock(mutex_);
        slots.push_back(std::make_shared<Slot>(slot));  // 存储槽
    }

    // 连接普通函数
    void connect(Slot slot) {
        std::lock_guard<std::mutex> lock(mutex_);
        slots.push_back(std::make_shared<Slot>(std::move(slot)));
    }

    // 断开连接
    void disconnect(Slot slot) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = std::remove_if(slots.begin(), slots.end(),
                                 [&slot](const std::shared_ptr<Slot>& s) { return *s == slot; });
        slots.erase(it, slots.end());
    }

    // 发射信号
    void emit(Args... args) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& slot : slots) {
            (*slot)(args...);  // 调用每个槽函数
        }
    }

private:
    std::vector<std::shared_ptr<Slot>> slots;
    std::mutex mutex_;  // 用于线程安全
};

#endif // SIGNAL_SLOT_H
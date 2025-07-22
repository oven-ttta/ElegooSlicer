#ifndef slic3r_Singleton_hpp_
#define slic3r_Singleton_hpp_

#include <atomic>
#include <mutex>
#include <memory>

namespace Slic3r {

template <typename T>
class Singleton {
public:
    static T* instance() {
        T* expected = nullptr;
        if (s_instance.compare_exchange_strong(expected, nullptr)) {
            std::lock_guard<std::mutex> lock(s_mutex);
            if (s_instance.load() == nullptr) {
                s_instance.store(new T);
            }
        }
        return s_instance.load();
    }

    static void destroy() {
        T* old_instance = s_instance.exchange(nullptr);
        if (old_instance) {
            delete old_instance;
        }
    }

    static bool isInstanceExist() {
        return s_instance.load() != nullptr;
    }

protected:
    Singleton() {}
    virtual ~Singleton() {}

private:
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;

private:
    static std::mutex s_mutex;
    static std::atomic<T*> s_instance;
};

template <typename T>
std::mutex Singleton<T>::s_mutex;

template <typename T>
std::atomic<T*> Singleton<T>::s_instance{nullptr};

} // namespace Slic3r

#endif 
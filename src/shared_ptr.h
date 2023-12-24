//
// Created by henry on 12/21/2023.
//

#ifndef SHARED_PTR_H_
#define SHARED_PTR_H_

#include <atomic>

class control_block {
  public:
    constexpr control_block() : strong(1), weak(1) {}

    void inc_strong() { strong.fetch_add(1, std::memory_order_relaxed); }
    void inc_weak() { weak.fetch_add(1, std::memory_order_relaxed); }
    int32_t dec_strong() { return strong.fetch_sub(1, std::memory_order_release); }
    int32_t dec_weak() { return weak.fetch_sub(1, std::memory_order_release); }
    int32_t use_count() { return strong.load(std::memory_order_relaxed); }

  public:
    std::atomic<int32_t> strong;
    std::atomic<int32_t> weak;
};

template<class T>
class weak_ptr;

template<class T>
class shared_ptr {
  public:
    using element_type = T;
    using weak_type = weak_ptr<T>;

    friend class weak_ptr<T>;
  public:
    constexpr shared_ptr() noexcept: object(nullptr), blk(nullptr) {}

    explicit shared_ptr(T *ptr) : object(ptr), blk(new control_block()) {}

    shared_ptr(const shared_ptr &other) noexcept {
        object = other.object;
        blk = other.blk;
        if (blk) {
            blk->inc_strong();
        }
    }

    shared_ptr(shared_ptr &&other) noexcept {
        object = other.object;
        blk = other.blk;
        other.object = nullptr;
        other.blk = nullptr;
    }

    shared_ptr<T> &operator=(const shared_ptr &other) noexcept {
        if (this == &other) return *this;
        release_strong_ref();
        object = other.object;
        blk = other.blk;
        if (blk) {
            blk->inc_strong();
        }
        return *this;
    };

    shared_ptr<T> &operator=(shared_ptr &&other) noexcept {
        if (this == &other) return *this;
        object = other.object;
        blk = other.blk;
        other.object = nullptr;
        other.blk = nullptr;
        return *this;
    };

    ~shared_ptr() {
        release_strong_ref();
    }
  public:
    explicit operator bool() const noexcept { return blk != nullptr; }

    [[nodiscard]] element_type *get() const noexcept { return object; }
    [[nodiscard]] int32_t use_count() const noexcept { return blk ? blk->use_count() : 0; }
    T &operator*() const noexcept { return *object; }
    T *operator->() const noexcept { return object; }

  private:
    void release_strong_ref() noexcept {
        if (!blk) return;
        if (blk->dec_strong() == 1) {
            std::atomic_thread_fence(std::memory_order_acquire);
            delete object;
            if (blk->dec_weak() == 1) {
                std::atomic_thread_fence(std::memory_order_acquire);
                delete blk;
            }
        }
    }

  private:
    T *object;
    control_block *blk;
};

template<class T>
class weak_ptr {
  public:
    using element_type = T;

    friend class shared_ptr<T>;
  public:
    constexpr weak_ptr() noexcept: object(nullptr), blk(nullptr) {};

    weak_ptr(const weak_ptr &other) noexcept {
        release_weak_ref();
        object = other.object;
        blk = other.blk;
        if (blk) {
            blk->inc_weak();
        }
    }

    weak_ptr(weak_ptr &&other) noexcept {
        object = other.object;
        blk = other.blk;
        other.object = nullptr;
        other.blk = nullptr;
    }

    template<class Y>
    weak_ptr(const shared_ptr<Y> &other) noexcept {
        if (!other) {
            object = nullptr;
            blk = nullptr;
            return;
        }
        object = other.object;
        blk = other.blk;
        blk->inc_weak();
    }

    ~weak_ptr() {
        release_weak_ref();
    }

    template<class Y>
    weak_ptr<T> &operator=(const shared_ptr<Y> &other) noexcept {
        release_weak_ref();
        if (!other) {
            object = nullptr;
            blk = nullptr;
            return *this;
        }
        object = other.object;
        blk = other.blk;
        blk->inc_weak();
        return *this;
    };

    weak_ptr<T> &operator=(const weak_ptr &other) noexcept {
        if (this == &other) return *this;
        release_weak_ref();
        object = other.object;
        blk = other.blk;
        if (blk) {
            blk->inc_weak();
        }
        return *this;
    };

    weak_ptr<T> &operator=(weak_ptr &&other) noexcept {
        if (this == &other) return *this;
        object = other.object;
        blk = other.blk;
        other.object = nullptr;
        other.blk = nullptr;
        return *this;
    };

  public:
    explicit operator bool() const noexcept { return blk != nullptr; }

    shared_ptr<T> lock() noexcept {
        if (!blk) return shared_ptr<T>();
        int32_t old = blk->strong.load(std::memory_order_relaxed);
        do {
            if (old == 0) {
                return shared_ptr<T>();
            }
        } while (
            !blk->strong.compare_exchange_weak(
                old, old + 1,
                std::memory_order_acq_rel,
                std::memory_order_relaxed)
            );
        shared_ptr<T> ptr;
        ptr.object = object;
        ptr.blk = blk;
        return ptr;
    }

    [[nodiscard]]int32_t use_count() const noexcept { return blk ? blk->use_count() : 0; }
    [[nodiscard]]bool expired() const noexcept { return use_count() == 0; }

  private:
    void release_weak_ref() noexcept {
        if (!blk) return;
        if (blk->dec_weak() == 1) {
            std::atomic_thread_fence(std::memory_order_acquire);
            delete blk;
        }
    }

  private:
    T *object;
    control_block *blk;
};

#endif //SHARED_PTR_H_


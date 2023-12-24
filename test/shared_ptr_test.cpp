#include <gtest/gtest.h>
#include <thread>

#include "shared_ptr.h"

class SharedPtrTest : public ::testing::Test {};

TEST_F(SharedPtrTest, DefaultConstructor) {
    shared_ptr<int> ptr;
    EXPECT_FALSE(bool(ptr));
    EXPECT_EQ(ptr.get(), nullptr);
    EXPECT_EQ(ptr.use_count(), 0);
}

TEST_F(SharedPtrTest, RawPointerConstructor) {
    int *rawPtr = new int(42);
    shared_ptr<int> ptr(rawPtr);
    EXPECT_TRUE(static_cast<bool>(ptr));  // Should not be nullptr
    EXPECT_EQ(ptr.get(), rawPtr);
    EXPECT_EQ(ptr.use_count(), 1);
    EXPECT_EQ(*ptr, 42);
}

TEST_F(SharedPtrTest, CopyConstructor) {
    shared_ptr<int> original(new int(42));
    shared_ptr<int> copy(original);
    EXPECT_TRUE(bool(copy));
    EXPECT_EQ(copy.get(), original.get());
    EXPECT_EQ(copy.use_count(), 2);
    EXPECT_EQ(original.use_count(), 2);
    EXPECT_EQ(*copy, 42);
}

TEST_F(SharedPtrTest, MoveConstructor) {
    shared_ptr<int> original(new int(42));
    shared_ptr<int> moved(std::move(original));
    EXPECT_FALSE(bool(original));
    EXPECT_TRUE(bool(moved));
    EXPECT_EQ(moved.use_count(), 1);
    EXPECT_EQ(original.use_count(), 0);
    EXPECT_EQ(*moved, 42);
}

TEST_F(SharedPtrTest, CopyAssignmentOperator) {
    shared_ptr<int> original(new int(42));
    shared_ptr<int> copy;
    copy = original;
    EXPECT_TRUE(bool(copy));
    EXPECT_EQ(copy.get(), original.get());
    EXPECT_EQ(copy.use_count(), 2);
    EXPECT_EQ(original.use_count(), 2);
    EXPECT_EQ(*copy, 42);
}

class WeakPtrTest : public ::testing::Test {};

TEST_F(WeakPtrTest, DefaultConstructor) {
    weak_ptr<int> wp;
    EXPECT_FALSE(static_cast<bool>(wp));
    EXPECT_EQ(wp.use_count(), 0);
    EXPECT_TRUE(wp.expired());
}

TEST_F(WeakPtrTest, CopyConstructor) {
    shared_ptr<int> sp(new int(42));
    weak_ptr<int> wp1(sp);

    EXPECT_TRUE(static_cast<bool>(wp1));
    EXPECT_EQ(wp1.use_count(), 1);
    EXPECT_EQ(*wp1.lock(), 42);
}

TEST_F(WeakPtrTest, MoveConstructor) {
    shared_ptr<int> sp(new int(42));
    weak_ptr<int> wp1(sp);
    weak_ptr<int> wp2(std::move(wp1));

    EXPECT_FALSE(static_cast<bool>(wp1));
    EXPECT_TRUE(static_cast<bool>(wp2));
    EXPECT_EQ(wp2.use_count(), 1);
    EXPECT_EQ(*wp2.lock(), 42);
}

TEST_F(WeakPtrTest, SharedPtrAssignment) {
    shared_ptr<int> sp(new int(42));
    weak_ptr<int> wp = sp;

    EXPECT_TRUE(static_cast<bool>(wp));
    EXPECT_EQ(wp.use_count(), 1);
    EXPECT_EQ(*wp.lock(), 42);
}

class LifetimeTest : public ::testing::Test {};

TEST_F(LifetimeTest, WeakPtrOutlivesSharedPtr) {
    weak_ptr<int> wp1;
    {
        shared_ptr<int> sp1(new int(42));
        wp1 = sp1;
    }
    EXPECT_TRUE(wp1.expired());
}

TEST_F(LifetimeTest, MultipleWeakPtrOutlivesSharedPtr) {
    weak_ptr<int> wp1;
    weak_ptr<int> wp2;
    {
        shared_ptr<int> sp1(new int(42));
        wp1 = sp1;
        wp2 = wp1;
    }
    EXPECT_TRUE(wp1.expired());
    EXPECT_TRUE(wp2.expired());
}

TEST_F(LifetimeTest, SharedAndWeakOutlivesSharedPtr) {
    weak_ptr<int> wp1;
    shared_ptr<int> sp2;
    {
        shared_ptr<int> sp1(new int(42));
        wp1 = sp1;
        sp2 = wp1.lock();
    }
    EXPECT_FALSE(wp1.expired());
    EXPECT_EQ(wp1.use_count(), 1);
    EXPECT_EQ(sp2.use_count(), 1);
}

TEST_F(LifetimeTest, MultiThreadDestruct) {
    weak_ptr<int> wp1;

    {
        shared_ptr<int> sp1(new int(42));
        wp1 = sp1;
        EXPECT_EQ(wp1.use_count(), 1);

        std::vector<std::jthread> pool;
        for (int i = 1; i <= 10; ++i) {
            pool.emplace_back([sp1] {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            });
        }
        EXPECT_EQ(pool.size(), 10);
        EXPECT_EQ(wp1.use_count(), 11);
    }

    EXPECT_TRUE(wp1.expired());
}

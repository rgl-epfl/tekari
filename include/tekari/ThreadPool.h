#pragma once

#include <vector>
#include <condition_variable>
#include <thread>
#include <functional>
#include <mutex>
#include "SharedQueue.h"

template<unsigned int N = 10>
class ThreadPool
{
public:

    ThreadPool()
        : mTasksAvailable(0)
        , mShouldTerminate(false)
    {
        for (unsigned int i = 0; i < N; i++)
        {
            mThreads.emplace_back([this](void) {
                while (!mShouldTerminate) {
                    std::unique_lock<std::mutex> lock{ mTasksMutex };
                    while (!mShouldTerminate && mTasksAvailable <= 0) {
                        mTasksCond.wait(lock);
                    }
                    // if we didn't quit te waiting loop because we should terminate
                    if (!mShouldTerminate)
                    {
                        auto task = mTaskQueue.pop();
                        --mTasksAvailable;
                        lock.unlock();
                        mTasksCond.notify_all();
                        task();
                    }
                    else
                    {
                        lock.unlock();
                    }
                }
            });
        }
    }

    ~ThreadPool() {
        std::unique_lock<std::mutex> lock{ mTasksMutex };
        mShouldTerminate = true;
        mTasksCond.notify_all();
        lock.unlock();

        for (auto& thread : mThreads)
            thread.join();
    }

    void addTask(const std::function<void(void)> task) {
        std::unique_lock<std::mutex> lock{ mTasksMutex };
        mTaskQueue.push(task);
        ++mTasksAvailable;
        mTasksCond.notify_one();
    }

    void waitForTasks() {
        std::unique_lock<std::mutex> lock{ mTasksMutex };
        while (mTasksAvailable) {
            mTasksCond.wait(lock);
        }
    }

    constexpr unsigned int size() const { return N; }

private:

    std::vector<std::thread> mThreads;
    SharedQueue<std::function<void(void)>> mTaskQueue;

    std::mutex mTasksMutex;
    std::condition_variable mTasksCond;
    int mTasksAvailable;

    bool mShouldTerminate;
};
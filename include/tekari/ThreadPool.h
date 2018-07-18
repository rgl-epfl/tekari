#pragma once

#include <vector>
#include <iostream>
#include <functional>
#include <condition_variable>
#include <thread>
#include <mutex>
#include <tekari/SharedQueue.h>

template<unsigned int N = 10>
class ThreadPool
{
public:

    ThreadPool()
    {
#if !defined(__EMSCRIPTEN__)
        for (unsigned int i = 0; i < N; i++)
        {
            m_threads.emplace_back([this](void) {
                while (!m_should_terminate) {
                    std::unique_lock<std::mutex> lock{ m_tasks_mutex };
                    while (!m_should_terminate && m_tasks_available <= 0) {
                        m_tasks_cond.wait(lock);
                    }
                    // if we didn't quit te waiting loop because we should terminate
                    if (!m_should_terminate)
                    {
                        auto task = m_task_queue.pop();
                        --m_tasks_available;
                        lock.unlock();
                        m_tasks_cond.notify_all();
                        task();
                    }
                    else
                    {
                        lock.unlock();
                    }
                }
            });
        }
#endif
    }

    ~ThreadPool() {
#if !defined(__EMSCRIPTEN__)
        std::unique_lock<std::mutex> lock{ m_tasks_mutex };
        m_should_terminate = true;
        m_tasks_cond.notify_all();
        lock.unlock();

        for (auto& thread : m_threads)
            thread.join();
#endif
    }

    void add_task(const std::function<void(void)> task) {
#if defined(__EMSCRIPTEN__)
        task();
#else
        std::unique_lock<std::mutex> lock{ m_tasks_mutex };
        m_task_queue.push(task);
        ++m_tasks_available;
        m_tasks_cond.notify_one();
#endif
    }

    void wait_for_tasks() {
#if !defined(__EMSCRIPTEN__)
        std::unique_lock<std::mutex> lock{ m_tasks_mutex };
        while (m_tasks_available) {
            m_tasks_cond.wait(lock);
        }
#endif
    }

    constexpr unsigned int size() const { return N; }

private:

#if !defined(__EMSCRIPTEN__)
    std::vector<std::thread> m_threads;
    SharedQueue<std::function<void(void)>> m_task_queue;
    std::mutex m_tasks_mutex;
    std::condition_variable m_tasks_cond;
    int m_tasks_available   = 0;
    bool m_should_terminate = false;
#endif

};
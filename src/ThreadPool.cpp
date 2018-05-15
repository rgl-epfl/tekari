#include "tekari/ThreadPool.h"

template<class N>
ThreadPool<N>::ThreadPool()
:   m_TasksAvailable(0)
,   m_ShouldTerminate(false)
{
    for (unsigned int i = 0; i < N; i++)
    {
        m_Threads.emplace_back([this](void) {
            while (!m_ShouldTerminate) {
                std::unique_lock<std::mutex> lock{ m_TasksMutex };
                while (!m_ShouldTerminate && m_TasksAvailable <= 0) {
                    m_TasksCond.wait(lock);
                }
                // if we didn't quit te waiting loop because we should terminate
                if (!m_ShouldTerminate)
                {
                    auto task = m_TaskQueue.pop();
                    --m_TasksAvailable;
                    lock.unlock();
                    m_TasksCond.notify_all();
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

template<typename N>
ThreadPool<N>::~ThreadPool()
{
    std::unique_lock<std::mutex> lock{ m_TasksMutex };
    m_ShouldTerminate = true;
    m_TasksCond.notify_all();
    lock.unlock();

    for (auto& thread : m_Threads)
        thread.join();
}

template<typename N>
void ThreadPool<N>::addTask(const std::function<void(void)> task) {
    std::unique_lock<std::mutex> lock{ m_TasksMutex };
    m_TaskQueue.push(task);
    ++m_TasksAvailable;
    m_TasksCond.notify_one();
}

template<typename N>
void ThreadPool<N>::waitForTasks() {
    std::unique_lock<std::mutex> lock{ m_TasksMutex };
    while (m_TasksAvailable) {
        m_TasksCond.wait(lock);
    }
}
#include "tekari/ThreadPool.h"

template<class N>
ThreadPool<N>::ThreadPool()
:   m_Tasks_available(0)
,   m_Should_terminate(false)
{
    for (unsigned int i = 0; i < N; i++)
    {
        m_Threads.emplace_back([this](void) {
            while (!m_Should_terminate) {
                std::unique_lock<std::mutex> lock{ m_Tasks_mutex };
                while (!m_Should_terminate && m_Tasks_available <= 0) {
                    m_Tasks_cond.wait(lock);
                }
                // if we didn't quit te waiting loop because we should terminate
                if (!m_Should_terminate)
                {
                    auto task = m_Task_queue.pop();
                    --m_Tasks_available;
                    lock.unlock();
                    m_Tasks_cond.notify_all();
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
    std::unique_lock<std::mutex> lock{ m_Tasks_mutex };
    m_Should_terminate = true;
    m_Tasks_cond.notify_all();
    lock.unlock();

    for (auto& thread : m_Threads)
        thread.join();
}

template<typename N>
void ThreadPool<N>::add_task(const std::function<void(void)> task) {
    std::unique_lock<std::mutex> lock{ m_Tasks_mutex };
    m_Task_queue.push(task);
    ++m_Tasks_available;
    m_Tasks_cond.notify_one();
}

template<typename N>
void ThreadPool<N>::wait_for_tasks() {
    std::unique_lock<std::mutex> lock{ m_Tasks_mutex };
    while (m_Tasks_available) {
        m_Tasks_cond.wait(lock);
    }
}
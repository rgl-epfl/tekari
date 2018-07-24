#pragma once
#include <mutex>
#include <queue>

template <typename T>
class SharedQueue
{
public:
    SharedQueue() = default;

    void push(const T& value)
    {
        std::lock_guard<std::mutex> lock{ m_queue_mutex };
        m_raw_queue.push(value);
    }

    T pop()
    {
        std::lock_guard<std::mutex> lock{ m_queue_mutex };
        T ret_val = m_raw_queue.front();
        m_raw_queue.pop();
        return ret_val;
    }

    T try_pop()
    {
        using std::cout;
        using std::endl;
        std::lock_guard<std::mutex> lock{ m_queue_mutex };
        if (m_raw_queue.empty())
        {
            throw std::runtime_error("Pop from empty stack");
        }
        T ret_val = std::move(m_raw_queue.front());
        m_raw_queue.pop();
        return ret_val;
    }

    bool emtpy()
    {
        std::lock_guard<std::mutex> lock{ m_queue_mutex };
        return m_raw_queue.empty();
    }

    size_t size()
    {
        std::lock_guard<std::mutex> lock{ m_queue_mutex };
        return m_raw_queue.size();
    }

private:
    std::queue<T> m_raw_queue;
    std::mutex m_queue_mutex;

    unsigned int m_size = 0;
};
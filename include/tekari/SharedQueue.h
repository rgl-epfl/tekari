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
        std::lock_guard<std::mutex> lock{ m_QueueMutex };
        m_RawQueue.push(value);
    }

    T pop()
    {
        std::lock_guard<std::mutex> lock{ m_QueueMutex };
        T retVal = m_RawQueue.front();
        m_RawQueue.pop();
        return retVal;
    }

    T tryPop()
    {
        std::lock_guard<std::mutex> lock{ m_QueueMutex };
        if (m_RawQueue.empty())
        {
            throw std::runtime_error("Pop from empty stack");
        }
        T retVal = std::move(m_RawQueue.front());
        m_RawQueue.pop();
        return retVal;
    }

    bool emtpy()
    {
        std::lock_guard<std::mutex> lock{ m_QueueMutex };
        return m_RawQueue.empty();
    }

    size_t size()
    {
        std::lock_guard<std::mutex> lock{ m_QueueMutex };
        return m_RawQueue.size();
    }

private:
    std::queue<T> m_RawQueue;
    std::mutex m_QueueMutex;

    unsigned int m_Size = 0;
};
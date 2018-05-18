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
        std::lock_guard<std::mutex> lock{ mQueueMutex };
        mRawQueue.push(value);
    }

    T pop()
    {
        std::lock_guard<std::mutex> lock{ mQueueMutex };
        T retVal = mRawQueue.front();
        mRawQueue.pop();
        return retVal;
    }

    T tryPop()
    {
        std::lock_guard<std::mutex> lock{ mQueueMutex };
        if (mRawQueue.empty())
        {
            throw std::runtime_error("Pop from empty stack");
        }
        T retVal = std::move(mRawQueue.front());
        mRawQueue.pop();
        return retVal;
    }

    bool emtpy()
    {
        std::lock_guard<std::mutex> lock{ mQueueMutex };
        return mRawQueue.empty();
    }

    size_t size()
    {
        std::lock_guard<std::mutex> lock{ mQueueMutex };
        return mRawQueue.size();
    }

private:
    std::queue<T> mRawQueue;
    std::mutex mQueueMutex;

    unsigned int mSize = 0;
};
#ifndef CONCURRENT_QUEUE_
#define CONCURRENT_QUEUE_

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

template <typename T>
class FrameQueue
{
  public:
    T pop()
    {
        std::unique_lock<std::mutex> mlock(mutex_);
        if (queue_.empty())
        {
            return NULL;
        }
        auto value = queue_.front();
        queue_.pop();
        return value;
    }

    void pop(T &item)
    {
        std::unique_lock<std::mutex> mlock(mutex_);
        while (queue_.empty())
        {
            cond_.wait(mlock);
        }
        item = queue_.front();
        queue_.pop();
    }

    void push(const T &item)
    {
        std::unique_lock<std::mutex> mlock(mutex_);
        queue_.push(item);
        mlock.unlock();
        cond_.notify_one();
    }
    FrameQueue() = default;
    FrameQueue(const FrameQueue &) = delete;
    FrameQueue &operator=(const FrameQueue &) = delete;

  private:
    std::queue<T> queue_;
    std::mutex mutex_;
    std::condition_variable cond_;
};

#endif
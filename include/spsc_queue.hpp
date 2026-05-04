#pragma once

#include <atomic>
#include <cstddef>
#include <optional>
#include <array>
#include <span>

template <typename T, std::size_t Capacity>
class SPSCQueue {
static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of two");
public:
    // Non-copyable, non-movable
    SPSCQueue() = default;
    SPSCQueue(const SPSCQueue&) = delete;
    SPSCQueue& operator=(const SPSCQueue&) = delete;

    // Called from producer thread only.
    // Returns false if the queue is full.
    template<typename U>
    requires std::convertible_to<U, T>
    bool push(U&& val) {
        if(full()) return false;
        size_t writeIdx = tail_.load(std::memory_order_relaxed);
        size_t nextWriteIdx = (writeIdx + 1) & (Capacity - 1);
        data_[writeIdx] = std::forward<U>(val);
        tail_.store(nextWriteIdx, std::memory_order_release);
        return true;
    }

    bool push(std::span<T> batch) {
        if(batch.size() > remaining()) return false;
        size_t writeIdx = tail_.load(std::memory_order_relaxed);
        for(auto& val : batch) {
            data_[writeIdx] = val;
            writeIdx = (writeIdx + 1) & (Capacity - 1);
        }
        tail_.store(writeIdx, std::memory_order_release);
        return true;
    }

    size_t pop(std::span<T> out) {
        size_t head = head_.load(std::memory_order_relaxed);
        size_t tail = tail_.load(std::memory_order_acquire);
        size_t itemsInQueue = (head > tail) ? tail + Capacity - head : tail - head; 
        if(itemsInQueue < out.size()) return 0;
        for(size_t i = 0; i < out.size(); ++i) {
            out[i] = data_[head];
            head = (head + 1) & (Capacity - 1);
        }
        head_.store(head, std::memory_order_release);
        return out.size();
    }

    // Called from consumer thread only.
    // Returns false if the queue is empty.
    bool pop(T& out) {
        size_t readIdx = head_.load(std::memory_order_relaxed);
        if(tail_.load(std::memory_order_acquire) == readIdx) return false;
        size_t nextReadIdx = (readIdx + 1) & (Capacity - 1);
        out = data_[readIdx];
        head_.store(nextReadIdx, std::memory_order_release);
        return true;
    }

    // Convenience wrapper -- returns nullopt if empty.
    std::optional<T> try_pop() {
        T result;
        return pop(result) ? std::make_optional(result) : std::nullopt;
    }

    bool empty() const {
        return head_.load(std::memory_order_relaxed) == tail_.load(std::memory_order_relaxed) ;
    }

    bool full() const {
        return ((tail_.load(std::memory_order_relaxed) + 1) & (Capacity - 1)) == head_.load(std::memory_order_relaxed);
    }

    size_t remaining() const {
        size_t tail = tail_.load(std::memory_order_relaxed);
        size_t head = head_.load(std::memory_order_relaxed);
        size_t itemsInQueue = (head > tail) ? tail + Capacity - head : tail - head; 
        return Capacity - 1 - itemsInQueue;
    }

private:
    alignas(64) std::array<T, Capacity> data_;
    // Read from head
    alignas(64) std::atomic<size_t> head_ {0};
    // Write at tail
    alignas(64) std::atomic<size_t> tail_ {0};
};

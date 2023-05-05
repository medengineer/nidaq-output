#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>

template <typename T>
class CircularBuffer {
public:
    CircularBuffer(size_t size) : buffer(size), read_index(0), write_index(0), size(size) {}

    void write(const T* data, size_t numSamples) {
        std::unique_lock<std::mutex> lock(mutex);
        for (size_t i = 0; i < numSamples; i++) {
            buffer[write_index] = data[i];
            write_index = (write_index + 1) % size;
        }
        cv.notify_one();
    }

    void read(T* data, size_t numSamples) {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock, [this, numSamples] { return (write_index + size - read_index) % size >= numSamples; });
        for (int i = 0; i < numSamples; i++) {
            data[i] = buffer[read_index];
            read_index = (read_index + 1) % size;
        }
    }

    size_t get_write_index() {
        std::lock_guard<std::mutex> lock(mutex);
        return write_index;
    }

    size_t get_read_index() {
        std::lock_guard<std::mutex> lock(mutex);
        return read_index;
    }

private:
    std::vector<T> buffer;
    size_t read_index, write_index, size;
    std::mutex mutex;
    std::condition_variable cv;
};
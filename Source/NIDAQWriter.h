#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>

class NIDAQWriter {
public:
    NIDAQWriter() : shouldRun(true) {}

    void start() {
        workerThread = std::thread(&NIDAQWriter::run, this);
    }

    void stop() {
        shouldRun.store(false);
        if (workerThread.joinable()) {
            workerThread.join();
        }
    }

private:
    std::atomic<bool> shouldRun;
    std::thread workerThread;

    void run() {
        while (shouldRun.load()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
};
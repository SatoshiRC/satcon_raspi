#pragma once

// Simple GPIO edge interrupt helper using libgpiod (v1.x API)
// Works on Raspberry Pi OS (Bullseye/Bookworm). Link with -lgpiod -lpthread.

#include <functional>
#include <atomic>
#include <thread>
#include <string>
#include <filesystem>
#include <fstream>

struct gpiod_chip;
struct gpiod_line;

class GPIOInterrupt {
public:
    enum class Edge { Rising, Falling, Both };

    // chip: e.g., "gpiochip0"; lineOffset: BCM GPIO number (offset on the chip)
    GPIOInterrupt(const std::string& chip, int lineOffset, Edge edge = Edge::Both);
    ~GPIOInterrupt();

    // Starts background thread that waits for edge events and calls cb(edgeIsRising)
    bool start();
    bool start(const std::function<void(bool /*rising*/)> &cb);
    void setCallback(const std::function<void(bool /*rising*/)> &cb);

    // Stops background thread and releases resources
    void stop();

    // Non-copyable
    GPIOInterrupt(const GPIOInterrupt&) = delete;
    GPIOInterrupt& operator=(const GPIOInterrupt&) = delete;

private:
    bool requestLine();
    void releaseLine();
    void run();

    std::string chipName;
    int offset;
    Edge edgeMode;

    gpiod_chip* chip = nullptr;
    gpiod_line* line = nullptr;

    std::function<void(bool)> callback;
    std::atomic<bool> running{false};
    std::thread worker;
};

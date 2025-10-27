#include "GPIOInterrupt.h"

#include <gpiod.h>
#include <chrono>
#include <time.h>

GPIOInterrupt::GPIOInterrupt(const std::string& chip, int lineOffset, Edge edge)
    : chipName(chip), offset(lineOffset), edgeMode(edge) {}

GPIOInterrupt::~GPIOInterrupt() {
    stop();
}

bool GPIOInterrupt::requestLine() {
    if (chip || line) {
        releaseLine();
    }

    chip = gpiod_chip_open_by_name(chipName.c_str());
    if (!chip) return false;

    line = gpiod_chip_get_line(chip, offset);
    if (!line) return false;

    int req = 0;
    switch (edgeMode) {
        case Edge::Rising: req = gpiod_line_request_rising_edge_events(line, "satcon_raspi"); break;
        case Edge::Falling: req = gpiod_line_request_falling_edge_events(line, "satcon_raspi"); break;
        case Edge::Both: req = gpiod_line_request_both_edges_events(line, "satcon_raspi"); break;
    }
    if (req < 0) return false;
    return true;
}

void GPIOInterrupt::releaseLine() {
    if (line) {
        gpiod_line_release(line);
        line = nullptr;
    }
    if (chip) {
        gpiod_chip_close(chip);
        chip = nullptr;
    }
}

bool GPIOInterrupt::start() {
    if (running.load()) return true;
    if (!requestLine()) return false;
    running = true;
    worker = std::thread(&GPIOInterrupt::run, this);
    return true;
}

bool GPIOInterrupt::start(const std::function<void(bool)> &cb) {
    setCallback(cb);
    return start();
}

void GPIOInterrupt::setCallback(const std::function<void(bool /*rising*/)> &cb){
    if (running.load()) return;
    callback = cb;
}

void GPIOInterrupt::stop() {
    if (!running.load()) return;
    running = false;
    if (worker.joinable()) worker.join();
    releaseLine();
}

void GPIOInterrupt::run() {
    // Wait loop using libgpiod event wait/read
    while (running.load()) {
        timespec timeout{1, 0}; // 1 second timeout to allow periodic check of running flag
        int rv = gpiod_line_event_wait(line, &timeout);
        if (rv < 0) {
            // error; attempt to continue
            continue;
        } else if (rv == 0) {
            // timeout; loop again
            continue;
        }
        gpiod_line_event ev{};
        if (gpiod_line_event_read(line, &ev) == 0) {
            if (callback) {
                bool rising = (ev.event_type == GPIOD_LINE_EVENT_RISING_EDGE);
                callback(rising);
            }
        }
    }
}

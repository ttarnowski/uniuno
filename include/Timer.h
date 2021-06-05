#pragma once

#include <Schedule.h>
#include <algorithm>
#include <functional>
#include <vector>

#ifndef ARDUINO
#include <native/millis.hpp>
#endif

namespace uniuno {

class Timer {
public:
  Timer(std::function<unsigned long(void)> getNowInMS = millis) {
    this->now = getNowInMS;
  }

  unsigned int set_timeout(std::function<void(void)> callback,
                           unsigned long timeout_ms) {
    this->timeouts.push_back(
        Timeout{this->index, this->now() + timeout_ms, callback});

    std::sort(this->timeouts.begin(), this->timeouts.end(),
              [](Timeout t1, Timeout t2) {
                return t1.next_call_ms < t2.next_call_ms;
              });

    return this->index++;
  }

  unsigned int set_immediate(std::function<void(void)> callback) {
    return this->set_timeout(callback, 0);
  }

  unsigned int set_interval(std::function<void(void)> callback,
                            unsigned long interval_ms) {
    this->intervals.push_back(Interval{this->index, this->now() + interval_ms,
                                       interval_ms, [callback]() {
                                         callback();
                                         return false;
                                       }});

    return this->index++;
  }

  unsigned int set_on_loop(std::function<void(void)> callback) {
    return this->set_interval(callback, 0);
  }

  void clear_timeout(unsigned int timeoutId) {
    for (auto it = this->timeouts.begin(); it != this->timeouts.end(); ++it) {
      if (it->id == timeoutId) {
        this->timeouts.erase(it);
        break;
      }
    }
  }

  void clear_interval(unsigned int interval_id) {
    for (auto it = this->intervals.begin(); it != this->intervals.end(); ++it) {
      if (it->id == interval_id) {
        this->intervals.erase(it);
        break;
      }
    }
  }

  unsigned int set_interval_until(std::function<bool(void)> callback,
                                  unsigned long interval_ms,
                                  std::function<void(void)> on_timeout,
                                  unsigned long timeout_ms) {
    auto timeoutCallMs = this->now() + timeout_ms;

    return this->set_interval_until(
        [this, timeoutCallMs, callback, on_timeout]() {
          bool shouldStop = callback();
          if (!shouldStop && timeoutCallMs <= this->now()) {
            on_timeout();
            return true;
          }

          return shouldStop;
        },
        interval_ms);
  }

  unsigned int set_interval_until(std::function<bool(void)> callback,
                                  unsigned long interval_ms) {
    this->intervals.push_back(Interval{this->index, this->now() + interval_ms,
                                       interval_ms, callback});

    return this->index++;
  }

  unsigned int set_on_loop_until(std::function<bool(void)> callback,
                                 std::function<void(void)> on_timeout,
                                 unsigned long timeout_ms) {
    return this->set_interval_until(callback, 0, on_timeout, timeout_ms);
  }

  unsigned int set_on_loop_until(std::function<bool(void)> callback) {
    return this->set_interval_until(callback, 0);
  }

  void attach_to_loop() { this->attached = true; }
  void detach_from_loop() { this->attached = false; }

  void tick() {
    for (std::size_t i = 0; i < this->timeouts.size(); i++) {
      if (this->now() < this->timeouts[i].next_call_ms) {
        break;
      }

      this->timeouts[i].callback();
      this->timeouts.erase(this->timeouts.begin() + i);
    }

    for (std::size_t i = 0; i < this->intervals.size(); i++) {
      if (this->now() < this->intervals[i].next_call_ms) {
        continue;
      }

      if (this->intervals[i].callback()) {
        this->intervals.erase(this->intervals.begin() + i);
      } else {
        this->intervals[i].next_call_ms =
            this->now() + this->intervals[i].interval_ms;
      }
    }

    if (this->attached) {
      this->schedule();
    }
  }

private:
  void schedule() {
    schedule_function([this]() {
      if (this == nullptr) {
        return;
      }
      this->tick();
    });

    std::function<unsigned long(void)> now;

    unsigned int index = 0;

    struct Timeout {
      unsigned int id;
      unsigned long next_call_ms;
      std::function<void(void)> callback;
    };

    std::vector<Timeout> timeouts;

    struct Interval {
      unsigned int id;
      unsigned long next_call_ms;
      unsigned long interval_ms;
      std::function<bool(void)> callback;
    };

    std::vector<Interval> intervals;

    bool attached = false;
  };
};

} // namespace uniuno
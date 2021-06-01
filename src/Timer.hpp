#pragma once

#include <algorithm>
#include <vector>

class Timer {
public:
#ifdef ARDUINO
  Timer(std::function<unsigned long(void)> getNowInMS = millis()) {
    this->now = getNowInMS;
  }

#else
  Timer(std::function<unsigned long(void)> getNowInMS) {
    this->now = getNowInMS;
  }
#endif

  unsigned int setTimeout(std::function<void(void)> callback,
                          unsigned long timeoutMs) {
    this->timeouts.push_back(
        Timeout{this->index, this->now() + timeoutMs, callback});

    std::sort(
        this->timeouts.begin(), this->timeouts.end(),
        [](Timeout t1, Timeout t2) { return t1.nextCallMs < t2.nextCallMs; });

    return this->index++;
  }

  unsigned int setImmediate(std::function<void(void)> callback) {
    return this->setTimeout(callback, 0);
  }

  unsigned int setInterval(std::function<void(void)> callback,
                           unsigned long intervalMs) {
    this->intervals.push_back(Interval{this->index, this->now() + intervalMs,
                                       intervalMs, [callback]() {
                                         callback();
                                         return false;
                                       }});

    return this->index++;
  }

  unsigned int setOnLoop(std::function<void(void)> callback) {
    return this->setInterval(callback, 0);
  }

  void clearTimeout(unsigned int timeoutId) {
    for (auto it = this->timeouts.begin(); it != this->timeouts.end(); ++it) {
      if (it->id == timeoutId) {
        this->timeouts.erase(it);
        break;
      }
    }
  }

  void clearInterval(unsigned int intervalId) {
    for (auto it = this->intervals.begin(); it != this->intervals.end(); ++it) {
      if (it->id == intervalId) {
        this->intervals.erase(it);
        break;
      }
    }
  }

  unsigned int setIntervalUntil(std::function<bool(void)> callback,
                                unsigned long intervalMs,
                                std::function<void(void)> onTimeout,
                                unsigned long timeoutMs) {
    auto timeoutCallMs = this->now() + timeoutMs;

    return this->setIntervalUntil(
        [this, timeoutCallMs, callback, onTimeout]() {
          bool shouldStop = callback();
          if (!shouldStop && timeoutCallMs <= this->now()) {
            onTimeout();
            return true;
          }

          return shouldStop;
        },
        intervalMs);
  }

  unsigned int setIntervalUntil(std::function<bool(void)> callback,
                                unsigned long intervalMs) {
    this->intervals.push_back(
        Interval{this->index, this->now() + intervalMs, intervalMs, callback});

    return this->index++;
  }

  unsigned int setOnLoopUntil(std::function<bool(void)> callback,
                              std::function<void(void)> onTimeout,
                              unsigned long timeoutMs) {
    return this->setIntervalUntil(callback, 0, onTimeout, timeoutMs);
  }

  unsigned int setOnLoopUntil(std::function<bool(void)> callback) {
    return this->setIntervalUntil(callback, 0);
  }

  void tick() {
    for (std::size_t i = 0; i < this->timeouts.size(); i++) {
      if (this->now() < this->timeouts[i].nextCallMs) {
        break;
      }

      this->timeouts[i].callback();
      this->timeouts.erase(this->timeouts.begin() + i);
    }

    for (std::size_t i = 0; i < this->intervals.size(); i++) {
      if (this->now() < this->intervals[i].nextCallMs) {
        continue;
      }

      if (this->intervals[i].callback()) {
        this->intervals.erase(this->intervals.begin() + i);
      } else {
        this->intervals[i].nextCallMs =
            this->now() + this->intervals[i].intervalMs;
      }
    }
  }

private:
  std::function<unsigned long(void)> now;

  unsigned int index = 0;

  struct Timeout {
    unsigned int id;
    unsigned long nextCallMs;
    std::function<void(void)> callback;
  };

  std::vector<Timeout> timeouts;

  struct Interval {
    unsigned int id;
    unsigned long nextCallMs;
    unsigned long intervalMs;
    std::function<bool(void)> callback;
  };

  std::vector<Interval> intervals;
};
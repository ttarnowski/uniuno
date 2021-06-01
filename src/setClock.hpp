#pragma once

#include <Timer.hpp>

void setClock(Timer *timer, std::function<void(bool)> onClockSet,
              unsigned long timeoutMs = 60000) {
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  Serial.println("Waiting for NTP time sync: ");

  timer->setOnLoopUntil(
      [onClockSet]() {
        time_t now = time(nullptr);
        if (now >= 8 * 3600 * 2) {
          Serial.println("");
          struct tm timeinfo;
          gmtime_r(&now, &timeinfo);
          Serial.print("Current time: ");
          Serial.print(asctime(&timeinfo));
          onClockSet(true);
          return true;
        }

        return false;
      },
      [onClockSet]() { onClockSet(false); }, timeoutMs);
}
#pragma once

#include <Future.hpp>
#include <WiFiConnector.hpp>
#include <create_timeout_future.hpp>
#include <logging.h>
#include <time.h>

static Future<void, time_t> set_clock(WiFiConnector *connector,
                                      unsigned long timeout_ms = 60000) {
  return create_timeout_future(
      connector->connect(timeout_ms)
          .and_then([]() {
            configTime(0, 0, "pool.ntp.org", "time.nist.gov");
            DEBUG("waiting for NTP time sync.");
          })
          .and_then(create_future([]() {
            time_t now = time(nullptr);

            TRACEF("synchronizing with NTP, time is: %l", now);

            if (now >= 8 * 3600 * 2) {
              struct tm timeinfo;
              gmtime_r(&now, &timeinfo);
              DEBUGF("synchronized with NTP, current time: %s\n",
                     asctime(&timeinfo));
              return AsyncResult<time_t>::resolve(now);
            }

            return AsyncResult<time_t>::pending();
          })),
      timeout_ms);
}
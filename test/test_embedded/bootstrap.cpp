#include <Arduino.h>
#include <gtest/gtest.h>

void setup() {
  Serial.begin(115200);
  delay(1000);

  testing::InitGoogleTest();

  /*
    workaround until
    https://github.com/platformio/platformio-core/issues/3572
    is resolved
  */
  if (!RUN_ALL_TESTS()) {
    Serial.println("Tests Failures Ignored :PASS");
  } else {
    Serial.println("Tests Failures Ignored :FAIL");
  }
}

void loop() {}
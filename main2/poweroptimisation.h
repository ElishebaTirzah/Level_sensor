#ifndef POWEROPTIMISATION_H
#define POWEROPTIMISATION_H

void configureDeepSleep() {
    uint64_t sleepDuration = 7 * 60 * 1000000ULL;
    esp_sleep_enable_timer_wakeup(sleepDuration);
    Serial.println("Configured to sleep for 7 minutes...");
}

void enterDeepSleep() {
    Serial.println("Entering deep sleep now...");
    Serial.flush();
    esp_deep_sleep_start();
}

#endif

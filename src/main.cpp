/*
 * Project:    Sentinel Power Monitor
 * Created By: Mr. Sunil Kumar S.
 * LinkedIn:   https://www.linkedin.com/in/sunilkumarsekar/
 * Year:       2026
 *
 * File: main.cpp
 * Brief: Arduino entry point. Delegates entirely to Boot.cpp.
 *        Keep this file minimal – no logic lives here.
 */

#include <Arduino.h>
#include "Boot.h"

void setup() {
    bootInit();
}

void loop() {
    bootLoop();
}

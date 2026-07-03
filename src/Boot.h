/*
 * Project:    Sentinel Power Monitor
 * Created By: Mr. Sunil Kumar S.
 * LinkedIn:   https://www.linkedin.com/in/sunilkumarsekar/
 * Year:       2026
 *
 * File: Boot.h
 * Brief: Declares the two entry-point functions called from main.cpp.
 */

#pragma once

/** @brief Called once from Arduino setup(). Initialises all services. */
void bootInit();

/** @brief Called every iteration from Arduino loop(). Drives all services. */
void bootLoop();

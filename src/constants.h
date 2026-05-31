#pragma once

#include <Arduino.h>

// AcidBox MIDI pins
#define MIDIRX_PIN 4
#define MIDITX_PIN 15

// AcidBox analog input pins
#define POT_NUM 3

// I2S output pins
#define I2S_BCLK_PIN 5
#define I2S_DOUT_PIN 6
#define I2S_WCLK_PIN 7
const uint8_t POT_PINS[POT_NUM] = {15, 16, 17};

// AcidBox status LED pin
#define LED_BEAT_PIN 13

// Streamer mode enable pin
#define STREAMER_ENABLE_PIN 21

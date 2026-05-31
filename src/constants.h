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

// AcidBox status LED pin
#define LED_BEAT_PIN 13

// Streamer mode enable pin
#define STREAMER_ENABLE_PIN 21

// AcidBanger button pins (button to GND, use INPUT_PULLUP)
#define GEN_SYNTH1_BUTTON_PIN 16
#define GEN_SYNTH2_BUTTON_PIN 17
#define GEN_NOTES_BUTTON 18
#define GEN_DRUM_BUTTON 15
#define PLAY_BUTTON 8
#define MEM1_BUTTON 14  // unused pin (memory buttons not wired)
#define MEM2_BUTTON 14
#define MEM3_BUTTON 14
#define MEM4_BUTTON 14
#define MEM5_BUTTON 14

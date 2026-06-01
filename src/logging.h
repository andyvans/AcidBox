#pragma once

// Note that MIDI_VIA_SERIAL & MIDI_VIA_SERIAL2 conflict with Serial debugging, so if you want to use them,
// you have to disable Serial debugging (see config.h) or use MIDI_USB_DEVICE instead
// Note that debugging eats ticks initially belonging to real-time tasks and can affect sound.
//#define DEBUG_ON   

#ifndef DEBUG_PORT
  #define DEBUG_PORT Serial
#endif

// debug macros
#ifdef DEBUG_ON
  #define DEB(...)    DEBUG_PORT.print(__VA_ARGS__)
  #define DEBF(...)   DEBUG_PORT.printf(__VA_ARGS__)
  #define DEBUG(...)  DEBUG_PORT.println(__VA_ARGS__)
#else
  #define DEB(...)
  #define DEBF(...)
  #define DEBUG(...)
#endif
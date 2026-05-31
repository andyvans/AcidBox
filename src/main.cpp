#include <Arduino.h>
#include "constants.h"
#include "stream/Streamer.h"
#include "acidbox/AcidBox.h"

Streamer* streamer = nullptr;
AcidBox* acidBox = nullptr;
bool streamerMode = true; // Default to streamer mode, will be overridden in setup() based on pin state
unsigned long lastModeCheckMs = 0;
const unsigned long MODE_CHECK_INTERVAL_MS = 1000;

bool GetStreamerMode() 
{
  return digitalRead(STREAMER_ENABLE_PIN) == LOW; // Active LOW
}

void setup()
{
  Serial.begin(115200);
  delay(100);
  Serial.println("Boot: AcidBox setup() entered");
  
  pinMode(STREAMER_ENABLE_PIN, INPUT_PULLUP);
  
  streamerMode = GetStreamerMode();
  if (streamerMode)
  {
    Serial.println("Streamer mode enabled");
    streamer = new Streamer();
    streamer->Setup();
  }
  else
  {
    Serial.println("Streamer mode disabled - starting AcidBox");
    acidBox = new AcidBox();
    acidBox->Setup();
  }
}

void loop()
{
  unsigned long now = millis();
  if (now - lastModeCheckMs >= MODE_CHECK_INTERVAL_MS)
  {
    lastModeCheckMs = now;
    bool currentMode = GetStreamerMode();
    if (currentMode != streamerMode)
    {
      Serial.println("Mode pin changed, restarting...");
      delay(50);
      ESP.restart();
    }
  }

  if (streamer != nullptr)
    streamer->Tick();

  if (acidBox != nullptr)
    acidBox->Tick();
}
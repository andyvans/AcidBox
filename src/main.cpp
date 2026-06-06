#include <Arduino.h>
#include <esp_system.h>
#include "constants.h"
#include "acidbox/config.h"
#include "stream/Streamer.h"
#include "acidbox/AcidBox.h"

Streamer* streamer = nullptr;
AcidBox* acidBox = nullptr;
bool streamerMode = true; // Default to streamer mode, will be overridden in setup() based on pin state
unsigned long lastModeCheckMs = 0;
const unsigned long MODE_CHECK_INTERVAL_MS = 50;
const unsigned long MODE_CHANGE_DEBOUNCE_MS = 300;
bool pendingModeChange = false;
bool pendingModeValue = true;
unsigned long pendingModeSinceMs = 0;

static bool GetStreamerMode();
static void checkStreamerMode();

void setup()
{
#ifdef DEBUG_ON
  DEBUG_PORT.begin(115200);
#endif

  DEBUG("Boot: AcidBox setup() entered");
  delay(1000);  
  pinMode(STREAMER_ENABLE_PIN, INPUT_PULLUP);
 
  streamerMode = GetStreamerMode();
  if (streamerMode)
  {
    DEBUG("Starting Streamer mode");
    streamer = new Streamer();
    streamer->Setup();
  }
  else
  {
    DEBUG("Starting AcidBox mode");
    acidBox = new AcidBox();
    acidBox->Setup();
  }
}

void loop()
{
  checkStreamerMode();

  if (streamer != nullptr)
    streamer->Tick();

  if (acidBox != nullptr)
    acidBox->Tick();
}

static void checkStreamerMode()
{
  unsigned long now = millis();
  if (now - lastModeCheckMs >= MODE_CHECK_INTERVAL_MS)
  {
    lastModeCheckMs = now;
    bool currentMode = GetStreamerMode();
    if (currentMode != streamerMode)
    {
      if (!pendingModeChange || pendingModeValue != currentMode)
      {
        pendingModeChange = true;
        pendingModeValue = currentMode;
        pendingModeSinceMs = now;
      }

      if (pendingModeChange && (now - pendingModeSinceMs >= MODE_CHANGE_DEBOUNCE_MS))
      {
        DEBUG("Mode pin change confirmed, restarting...");
        delay(50);
        ESP.restart();
      }
    }
    else
    {
      pendingModeChange = false;
    }
  }
}

static bool GetStreamerMode()
{
  return digitalRead(STREAMER_ENABLE_PIN) == HIGH;
}

#include "Streamer.h"
#include <WiFiManager.h>
#include "logging.h"

Streamer::Streamer()
{
    _audioOut = nullptr;
    _deviceControls = nullptr;
    _radioConfig = nullptr;
    _deviceTask = NULL;
    _audioInitialized = false;
}

Streamer::~Streamer()
{
    if (_deviceTask != NULL)
    {
        vTaskDelete(_deviceTask);
        _deviceTask = NULL;
    }
    delete _deviceControls;
    delete _audioOut;
    delete _radioConfig;
}

void Streamer::Setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    WiFi.setHostname("UltimateRadio");

    WiFiManager wm;
    wm.setConfigPortalTimeout(180);
    DEBUG("Connecting to WiFi...");
    if (wm.autoConnect("AcidBox-Streamer-Setup"))
    {
        DEBUG("WiFi connected!");
        DEB("IP: ");
        DEBUG(WiFi.localIP());
        InitializeAudio();
        return;
    }

    DEBUG("WiFi connection failed - restarting...");
    ESP.restart();
}

void Streamer::InitializeAudio()
{
    if (_audioInitialized)
    {
        return;
    }

    // AAC support requires PSRAM due to the larger buffers
    bool supportAac = ESP.getPsramSize() > 0;
    if (!supportAac)
        DEBUG("No PSRAM detected - AAC support disabled");

    DEBUG("=== AcidBox Streamer Starting ===");

    _radioConfig = ChannelManager::LoadChannels(CONFIG_URL);
    if (_radioConfig == nullptr)
    {
        DEBUG("Using default channels");
        _radioConfig = ChannelManager::GetDefaultChannels();
    }

    _audioOut = new AudioOut(supportAac);
    _audioOut->Setup(_radioConfig);

    _deviceControls = new DeviceControls();
    _deviceControls->Setup(_audioOut);

    StartDeviceTask();
    _audioInitialized = true;
}

void Streamer::Tick()
{
    static unsigned long lastLedToggleMs = 0;
    static bool ledState = false;
    unsigned long now = millis();
    if (now - lastLedToggleMs >= 1000)
    {
        lastLedToggleMs = now;
        ledState = !ledState;
        digitalWrite(LED_BUILTIN, ledState ? HIGH : LOW);
    }

    if (_audioOut != nullptr)
        _audioOut->Tick();
}

void Streamer::StartDeviceTask()
{
    DEBUG("Creating device task");
    xTaskCreatePinnedToCore(
        ProcessDevicesTask,
        "Device",
        4096,
        this,
        1,
        &_deviceTask,
        0); // Core 0 (shared with WiFi & system tasks)
}

void Streamer::ProcessDevicesTask(void* parameter)
{
    Streamer* self = static_cast<Streamer*>(parameter);
    for (;;)
    {
        if (self->_deviceControls != nullptr)
        {
            self->_deviceControls->Tick();
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
#include "Streamer.h"
#include <WiFiManager.h>

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
    WiFi.setHostname("UltimateRadio");

    WiFiManager wm;
    wm.setConfigPortalTimeout(180);
    Serial.println("Connecting to WiFi...");
    if (wm.autoConnect("AcidBox-Streamer-Setup"))
    {
        Serial.println("WiFi connected!");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
        InitializeAudio();
        return;
    }

    Serial.println("WiFi connection failed - restarting...");
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
        Serial.println("No PSRAM detected - AAC support disabled");

    Serial.println("=== AcidBox Streamer Starting ===");

    _radioConfig = ChannelManager::LoadChannels(CONFIG_URL);
    if (_radioConfig == nullptr)
    {
        Serial.println("Using default channels");
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
    if (_audioOut != nullptr)
        _audioOut->Tick();
}

void Streamer::StartDeviceTask()
{
    Serial.println("Creating device task");
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
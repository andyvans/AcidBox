#pragma once
#include <Arduino.h>
#include "AudioOut.h"
#include "DeviceControls.h"
#include "ChannelManager.h"

class WiFiManager;

// Configuration URL - change this to your config file location
#define CONFIG_URL "https://raw.githubusercontent.com/andyvans/rinsefm-box/main/radio-config.txt"

class Streamer
{
public:
    Streamer();
    ~Streamer();
    void Setup();
    void Tick();
    void StartDeviceTask();

    AudioOut* GetAudioOut() { return _audioOut; }
    DeviceControls* GetDeviceControls() { return _deviceControls; }

private:
    AudioOut* _audioOut;
    DeviceControls* _deviceControls;
    RadioConfig* _radioConfig;
    TaskHandle_t _deviceTask;
    bool _audioInitialized;

    void InitializeAudio();

    static void ProcessDevicesTask(void* parameter);
};

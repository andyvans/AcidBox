#pragma once
#include <Arduino.h>
#include <Wire.h>
#include "constants.h"
#include "stream/AudioOut.h"

class DeviceControls
{
public:
    DeviceControls();
    void Setup(AudioOut* audio);
    void Tick();

private:
    int _currentChannel;
    int _pendingChannel;
    unsigned long _lastPositionChangeTime;
    bool _hasPendingChange;       
    AudioOut* _audioOut;
    
    int ChannelChangeDelayMs = 750;
};

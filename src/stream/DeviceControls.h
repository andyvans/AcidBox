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
    bool _initialChannelStarted;
    bool _lastUpPressed;
    bool _lastDownPressed;
    AudioOut* _audioOut;
    
    int ChannelChangeDelayMs = 750;
};

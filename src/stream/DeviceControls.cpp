#include "DeviceControls.h"
#include "AudioOut.h"

DeviceControls::DeviceControls() :
    _audioOut(nullptr),
    _currentChannel(0),
    _pendingChannel(-1),
    _lastPositionChangeTime(0),
    _hasPendingChange(false)
{
}

void DeviceControls::Setup(AudioOut* audioOut)
{
    _audioOut = audioOut;

    Serial.println("=== Setting up DeviceControls ===");

    // Max value is maxChannels - 1 since we're using 0-based indexing
    _currentChannel = _audioOut != nullptr ? _audioOut->GetCurrentChannel() : 0;
    int maxChannel = _audioOut != nullptr ? _audioOut->GetChannelCount() - 1 : 0;

    
}

void DeviceControls::Tick()
{    
    if (_audioOut == nullptr) return;
       
    if (false)
    {
        // New position detected - update pending channel and reset timer
        //_pendingChannel = posState.position;
        _lastPositionChangeTime = millis();
        _hasPendingChange = true;
        
    }

    // Debounce check if we should apply the pending channel change
    if (_hasPendingChange && (millis() - _lastPositionChangeTime >= ChannelChangeDelayMs))
    {
        _currentChannel = _pendingChannel;
        _hasPendingChange = false;
        _audioOut->Start(_currentChannel);        
    }
}
#include "DeviceControls.h"
#include "AudioOut.h"
#include "logging.h"

DeviceControls::DeviceControls() :
    _audioOut(nullptr),
    _currentChannel(0),
    _pendingChannel(-1),
    _lastPositionChangeTime(0),
    _hasPendingChange(false),
    _initialChannelStarted(false),
    _lastUpPressed(false),
    _lastDownPressed(false)
{
}

void DeviceControls::Setup(AudioOut* audioOut)
{
    _audioOut = audioOut;

    DEBUG("=== Setting up DeviceControls ===");

    // Max value is maxChannels - 1 since we're using 0-based indexing
    _currentChannel = _audioOut != nullptr ? _audioOut->GetCurrentChannel() : 0;
    _pendingChannel = _currentChannel;

    // Buttons are wired to GND and use internal pull-ups.
    pinMode(GEN_SYNTH2_BUTTON_PIN, INPUT_PULLUP); // channel up
    pinMode(GEN_SYNTH1_BUTTON_PIN, INPUT_PULLUP); // channel down

    _lastUpPressed = (digitalRead(GEN_SYNTH2_BUTTON_PIN) == LOW);
    _lastDownPressed = (digitalRead(GEN_SYNTH1_BUTTON_PIN) == LOW);    
}

void DeviceControls::Tick()
{    
    if (_audioOut == nullptr) return;

    int maxChannel = _audioOut->GetChannelCount() - 1;
    if (maxChannel < 0)
    {
        return;
    }

    if (!_initialChannelStarted)
    {
        _currentChannel = constrain(_currentChannel, 0, maxChannel);
        _pendingChannel = _currentChannel;
        _audioOut->Start(_currentChannel);
        _initialChannelStarted = true;
    }

    bool upPressed = (digitalRead(GEN_SYNTH2_BUTTON_PIN) == LOW);
    bool downPressed = (digitalRead(GEN_SYNTH1_BUTTON_PIN) == LOW);

    // Act only on press edges, and ignore simultaneous presses.
    if (upPressed && !downPressed && !_lastUpPressed)
    {
        _pendingChannel = constrain(_pendingChannel + 1, 0, maxChannel);
        _lastPositionChangeTime = millis();
        _hasPendingChange = true;
    }
    else if (downPressed && !upPressed && !_lastDownPressed)
    {
        _pendingChannel = constrain(_pendingChannel - 1, 0, maxChannel);
        _lastPositionChangeTime = millis();
        _hasPendingChange = true;
    }

    _lastUpPressed = upPressed;
    _lastDownPressed = downPressed;

    // Debounce check if we should apply the pending channel change
    if (_hasPendingChange && (millis() - _lastPositionChangeTime >= ChannelChangeDelayMs))
    {
        _currentChannel = constrain(_pendingChannel, 0, maxChannel);
        _hasPendingChange = false;
        _audioOut->Start(_currentChannel);        
    }
}
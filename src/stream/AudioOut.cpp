#include "AudioOut.h"
#include "AudioTools/AudioCodecs/CodecMP3Helix.h"
#include "logging.h"

AudioOut::AudioOut(bool supportAac)
{
    _supportAac = supportAac;
    _currentChannel = 0;
    _pendingChannel = 0;
    _mode = AUDIO_MODE_OFF;
    _isPlaying = false;
    _usingDynamicChannels = false;
    _channels = nullptr;
    _channelCount = 0;
}

AudioOut::~AudioOut()
{
    // Note: Dynamic channel memory is managed by RadioConfig
}

void AudioOut::Setup(RadioConfig* config)
{
    DEBUG("=== Setting up AudioOut ===");

    if (config->channels != nullptr && config->channelCount > 0)
    {
        _channels = config->channels;
        _channelCount = config->channelCount;
        DEB("Using ");
        DEB(_channelCount);
        DEBUG(" dynamically loaded channels");
    }
    else
    {
        DEBUG("No channels provided!");
        _channels = nullptr;
        _channelCount = 0;
    }
    if (config->defaultChannel >= 0 && config->defaultChannel < _channelCount)
    {
        _currentChannel = config->defaultChannel;
        _pendingChannel = config->defaultChannel;
    }

    AudioToolsLogger.begin(Serial, AudioToolsLogLevel::Warning);

    DEBUG("Creating URLStream...");
    _urlStream = new URLStreamBuffered();
    _audioSourceUrl = new AudioSourceDynamicURL(*_urlStream, nullptr, _currentChannel);

    // Add all the URLs to the dynamic source
    for (int i = 0; i < _channelCount; i++)
    {
        _audioSourceUrl->addURL(_channels[i].url);
    }

    DEBUG("Creating decoders...");
    _mp3Decoder = new MP3DecoderHelix();
    _aacDecoder = _supportAac ? new AACDecoderHelix() : nullptr;

    DEBUG("Creating MultiDecoder...");
    _multiDecoder = new MultiDecoder(*_urlStream);
    _multiDecoder->addDecoder(*_mp3Decoder, "audio/mp3");
    _multiDecoder->addDecoder(*_mp3Decoder, "audio/mpeg");
    if (_supportAac)
    {
        _multiDecoder->addDecoder(*_aacDecoder, "audio/aac");
        _multiDecoder->addDecoder(*_aacDecoder, "audio/aacp");
    }

    DEBUG("Creating I2S stream...");
    _i2sOut = new I2SStream();

    DEBUG("Configuring I2S output...");
    auto configOut = _i2sOut->defaultConfig(TX_MODE);
    configOut.pin_bck = I2S_BCLK_PIN;
    configOut.pin_ws = I2S_WCLK_PIN;
    configOut.pin_data = I2S_DOUT_PIN;

    DEBUG("Starting I2S stream...");
    _i2sOut->begin(configOut);

    DEBUG("Creating audio player...");
    _audioPlayer = new AudioPlayer(*_audioSourceUrl, *_i2sOut, *_multiDecoder);
        
    _audioPlayer->setVolume(config->volume); // Set volume from config

    DEBUG("=== AudioOut setup complete ===");
}

int AudioOut::GetChannelCount()
{
    return _channelCount;
}

int AudioOut::GetCurrentChannel()
{
    return _currentChannel;
}

const char* AudioOut::GetChannelName(int channel) const
{
    if (_channels == nullptr || channel < 0 || channel >= _channelCount) return nullptr;
    return _channels[channel].name;
}

void AudioOut::Start(int channel)
{
    if (_channels == nullptr || _channelCount <= 0) return;
    if (channel < 0) channel = 0;
    if (channel >= _channelCount) channel = _channelCount - 1;
    if (channel != _pendingChannel)
    {
        DEB("Changing pending audio to channel: ");
        DEBUG(_channels[channel].url);
        _pendingChannel = channel;
    }
    _mode = AUDIO_MODE_RADIO;
}

void AudioOut::Stop()
{
    _mode = AUDIO_MODE_OFF;
}

AudioMode AudioOut::GetMode()
{
    return _mode;
}

void AudioOut::Tick()
{
    if (_audioPlayer == nullptr) return;
    if (_channels == nullptr || _channelCount <= 0) return;

    if (_mode == AUDIO_MODE_OFF && _isPlaying)
    {
        DEBUG("Stopping audio playback");
        _audioPlayer->end();
        _isPlaying = false;
    }

    if (_pendingChannel != _currentChannel)
    {
        _currentChannel = _pendingChannel;
        DEB("Switching to channel: ");
        DEBUG(_channels[_currentChannel].url);
        _audioPlayer->setIndex(_currentChannel);
    }

    if (_mode == AUDIO_MODE_RADIO && !_isPlaying)
    {
        DEB("Starting channel: ");
        DEBUG(_channels[_currentChannel].url);
        _audioPlayer->begin(_currentChannel);
        _isPlaying = true;
    }

    _audioPlayer->copy();
}

bool AudioOut::IsPlaying()
{
    return _isPlaying && _audioPlayer != nullptr && _audioPlayer->isActive();
}
#pragma once
#include <Arduino.h>
#include "Constants.h"

// Audio-Tools: Buffer sizes adjusted for memory constraints
#define DEFAULT_BUFFER_SIZE 1536 // Default is 1024
#define I2S_BUFFER_SIZE 512 // Default is 512
#define I2S_BUFFER_COUNT 8 // Default is 6
#define USE_AUDIO_LOGGING false

#include <AudioTools.h>
#include <AudioTools/Communication/AudioHttp.h>
#include <AudioTools/Communication/HLSStream.h>
#include <AudioTools/Disk/AudioSourceURL.h>
#include <AudioTools/AudioCodecs/CodecMP3Helix.h>
#include <AudioTools/AudioCodecs/CodecAACHelix.h>
#include <AudioTools/AudioCodecs/CodecMTS.h>
#include <AudioTools/AudioCodecs/MultiDecoder.h>
#include "ConfigLoader.h"

using namespace audio_tools;

enum AudioMode
{
    AUDIO_MODE_OFF,
    AUDIO_MODE_RADIO,
};

class AudioOut
{
public:
    // Streaming buffer tuning: prefer smaller chunks with more buffers.
    static constexpr int kUrlBufferSize = 1024;
    static constexpr int kUrlBufferCount = 64;
    static constexpr int kPlayerCopyBufferSize = 4096;

    AudioOut(bool supportAac);
    ~AudioOut();
    void Setup(RadioConfig* config);
    void Stop();
    void Start(int channel);
    void Tick();
    int GetChannelCount();
    int GetCurrentChannel();
    const char* GetChannelName(int channel) const;
    AudioMode GetMode();
    bool IsPlaying();

private:
    enum StreamKind
    {
        STREAM_KIND_DIRECT,
        STREAM_KIND_HLS,
    };

    static bool IsHlsUrl(const char* url);
    void DestroyPipeline();
    bool BuildPipelineForChannel(int channel);

    static void HandleStreamChange(Stream* stream, void* reference);
    void OnStreamChanged(Stream* stream);

    volatile AudioMode _mode;
    volatile int _currentChannel;
    volatile int _pendingChannel;
    volatile bool _isPlaying;
    bool _usingDynamicChannels;
    bool _supportAac;
    float _volume;
    StreamKind _streamKind;

    URLStreamBuffered* _urlStream;
    HLSStream* _hlsStream;
    AudioSourceDynamicURL* _audioSourceUrl;
    I2SStream* _i2sOut;
    MP3DecoderHelix* _mp3Decoder;
    AACDecoderHelix* _aacDecoder;
    MTSDecoder* _mtsDecoder;
    MultiDecoder* _multiDecoder;
    AudioPlayer* _audioPlayer;

    // Channel storage
    ChannelConfig* _channels;
    int _channelCount;
    static const int _defaultChannelCount;
};
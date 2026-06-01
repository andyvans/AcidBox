#include "ConfigLoader.h"
#include <WiFiClientSecure.h>
#include <AudioTools.h>
#include "logging.h"

using namespace audio_tools;

bool ConfigLoader::LoadConfig(const char* configUrl, RadioConfig& config)
{
    DEB("Loading config from: ");
    DEBUG(configUrl);

    WiFiClientSecure client;
    client.setInsecure(); // Skip certificate validation for simplicity

    HttpRequest http(client);
    Url url(configUrl);

    int statusCode = http.get(url, "text/plain");

    if (statusCode != 200)
    {
        DEB("HTTP request failed with status: ");
        DEBUG(statusCode);
        return false;
    }

    // Read response into a String
    String data;
    const int chunkSize = 512;
    uint8_t chunk[chunkSize];

    while (http.available())
    {
        int bytesRead = http.readBytes(chunk, chunkSize);
        if (bytesRead > 0)
        {
            data.concat((const char*)chunk, bytesRead);
        }
        else
        {
            break;
        }
    }

    http.end();

    DEB("Downloaded ");
    DEB(data.length());
    DEBUG(" bytes");

    return ParseConfig(data, config);
}

bool ConfigLoader::ParseConfig(const String& data, RadioConfig& config)
{
    if (data.length() == 0)
    {
        DEBUG("No data to parse");
        return false;
    }

    config.channels = new ChannelConfig[MAX_CHANNELS];
    if (config.channels == nullptr)
    {
        DEBUG("Failed to allocate channel array");
        return false;
    }

    config.channelCount = 0;
    config.defaultChannel = 0;
    int lineNum = 0;
    int pos = 0;

    while (pos <= (int)data.length())
    {
        // Find end of line
        int eol = data.indexOf('\n', pos);
        if (eol < 0) eol = data.length();

        String line = data.substring(pos, eol);
        line.trim();

        // Strip trailing \r
        if (line.endsWith("\r"))
            line.remove(line.length() - 1);

        pos = eol + 1;

        // Skip empty lines and comments
        if (line.length() == 0 || line[0] == '#')
            continue;

        if (lineNum == 0)
        {
            config.defaultChannel = line.toInt();
            DEB("Default channel: ");
            DEBUG(config.defaultChannel);
        }
        else if (lineNum == 1)
        {
            config.volume = line.toFloat();
            DEB("Volume: ");
            DEBUG(config.volume);
        }
        else if (config.channelCount < MAX_CHANNELS)
        {
            int commaPos = line.indexOf(',');
            String urlStr;
            String nameStr;

            if (commaPos >= 0)
            {
                urlStr = line.substring(0, commaPos);
                nameStr = line.substring(commaPos + 1);
            }
            else
            {
                urlStr = line;
            }

            urlStr.trim();
            nameStr.trim();

            char* url = DuplicateString(urlStr, MAX_URL_LENGTH);
            if (url != nullptr)
            {
                char* name = nameStr.length() > 0
                    ? DuplicateString(nameStr, MAX_NAME_LENGTH)
                    : DuplicateString(urlStr, MAX_NAME_LENGTH);

                config.channels[config.channelCount].url = url;
                config.channels[config.channelCount].name = name;

                DEBF("Channel %d URL: %s\n", config.channelCount, url);
                DEBF("Channel %d Name: %s\n", config.channelCount, name ? name : "");

                config.channelCount++;
            }
        }

        lineNum++;
    }

    DEB("Loaded ");
    DEB(config.channelCount);
    DEBUG(" channels");

    return config.channelCount > 0;
}

char* ConfigLoader::DuplicateString(const String& str, int maxLen)
{
    int copyLen = min((int)str.length(), maxLen - 1);
    char* out = (char*)malloc(copyLen + 1);
    if (out != nullptr)
    {
        memcpy(out, str.c_str(), copyLen);
        out[copyLen] = '\0';
    }
    return out;
}
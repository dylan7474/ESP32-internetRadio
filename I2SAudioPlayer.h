#ifndef I2S_AUDIO_PLAYER_H
#define I2S_AUDIO_PLAYER_H

#include <Arduino.h>
#include <AudioFileSourceICYStream.h>
#include <AudioFileSourceBuffer.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2S.h>

class I2SAudioPlayer {
public:
    I2SAudioPlayer() : file(nullptr), buffer(nullptr), volume(0) {}

    void setPinout(int bclk, int lrck, int dout) {
        out.SetPinout(bclk, lrck, dout);
    }

    void setVolume(int vol) {
        volume = vol;
        out.SetGain((float)vol / 30.0f);
    }

    void setVolumeSteps(int) {
        // Not required for this implementation but kept for compatibility
    }

    void setTone(int, int, int) {
        // Tone controls are not implemented in this minimal wrapper
    }

    void connecttohost(const char *url) {
        stopSong();
        file = new AudioFileSourceICYStream(url);
        buffer = new AudioFileSourceBuffer(file, 2048);
        mp3.begin(buffer, &out);
    }

    void loop() {
        if (mp3.isRunning()) {
            if (!mp3.loop()) {
                mp3.stop();
            }
        }
    }

    void stopSong() {
        if (mp3.isRunning()) {
            mp3.stop();
        }
        if (buffer) {
            delete buffer;
            buffer = nullptr;
        }
        if (file) {
            delete file;
            file = nullptr;
        }
    }

private:
    AudioGeneratorMP3 mp3;
    AudioFileSourceICYStream *file;
    AudioFileSourceBuffer *buffer;
    AudioOutputI2S out;
    int volume;
};

#endif // I2S_AUDIO_PLAYER_H

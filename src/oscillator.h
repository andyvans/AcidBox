#pragma once
#include "bandlimited_tables.h"


class MipmapOscillator {

public:
    MipmapOscillator() {
        setSampleRate(44100.0f);
        setGlideMs(60.0f);
        setGlide(false);
        setWaveform(Waveform::Saw);
        setFrequency(440.0f);
        selectMip();
    }
    enum class Waveform : uint8_t { Saw, Square };

    void setSampleRate(float sr) { sampleRate = sr; divSampleRate = 1.0f / sr;}

    void setWaveform(Waveform wf) { waveform = wf; }

    void resetPhase(float phase = 0.0f) {
        currentPhase = phase;
    }

    inline float process() {
    //    if(unlikely(!currentTable)) {return 0.0f;}
        float idx = currentPhase * tableSize;
        int i0 = int(idx);
        float frac = idx - i0;
        int i1 = i0 + 1;
        if (i1 >= tableSize) i1 = 0;

        float out = (1.0f - frac) * currentTable[i0] + frac * currentTable[i1];

        currentPhase += currentStep;
        currentPhase -= (currentPhase >= 1.0f) ? 1.0f : 0.0f;
        if (glide) {
            if (fabs(endStep - currentStep) >= absDeltaStep) {
                currentStep += deltaStep;
            } else {
                currentStep = endStep;
                deltaStep = 0.0f;
                glide = false;
            }
        }
        return out;
    }

    inline void updateSteps()               { 
        endStep = noteStep * tuningMod * pitchbendMod;
        if (glide) {
            deltaStep = (endStep - currentStep) * divSlideS * divSampleRate;
            absDeltaStep = fabs(deltaStep);
        } else {
            deltaStep = 0.0f;
            absDeltaStep = 0.0f;
            currentStep = endStep;
        }
    }
    inline void setPitchbendMod(float k)    { pitchbendMod = k; updateSteps(); }
    inline void setTuningMod(float k)       { tuningMod = k; updateSteps(); }
    inline void setGlide(bool onOff)        { glide = onOff; updateSteps(); }
    inline void setGlideMs(float ms = 60.0f) { slideMs = ms; divSlideS = 1000.0f / ms; updateSteps(); }
    inline void setFrequency(float noteFreq) {
        frequency = noteFreq; 
        noteStep = noteFreq * divSampleRate;
        updateSteps();
        selectMip();
    }

    float sampleRate = 44100.0f;
    float divSampleRate = 1.0f / 44100.0f;
    float frequency = 440.0f;
    float currentStep = 0.0f;
    float noteStep = 0.0f;
    float endStep = 0.0f; // baked with pitchBend and tuning applied
    float deltaStep = 0.0f;
    float absDeltaStep = 0.0f;
    float pitchbendMod = 1.0f;
    float tuningMod = 1.0f;
    bool  glide = false;
    float currentPhase = 0.0f;
    float slideMs = 60.0f;
    float divSlideS = 1000.0f / 60.0f;

    const float* currentTable = nullptr;
    int tableSize = 256;
    Waveform waveform = Waveform::Saw;

    inline void selectMip() {
        for (int i = 0; i < mip_count; ++i) {
            if (currentStep >= div_sizes[i]) {
                tableSize = sizes[i];
                currentTable = (waveform == Waveform::Saw)
                                 ? saw_tables[i]
                                 : square_tables[i];
                return;
            }
        }
 
        tableSize = sizes[mip_count - 1];
        currentTable = (waveform == Waveform::Saw)
                         ? saw_tables[mip_count - 1]
                         : square_tables[mip_count - 1];
    }
	 
};
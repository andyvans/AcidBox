#include "synthvoice.h"

void SynthVoice::Init() {
  _envMod = 0.5f;
  _accentLevel = 0.5f;
  _cutoff = 0.2f; // 0..1 normalized freq range. Keep in mind that EnvMod set to max practically doubles this range
  _filter_freq = linToExp(_cutoff, 0.0f, 1.0f, MIN_CUTOFF_FREQ, MAX_CUTOFF_FREQ);
  _reso = 0.4f;
  _gain = 0.0f; // values >1 will distort sound
  _drive = 0.0f;
  //  midiNotes[0] = -1;
  //  midiNotes[1] = -1;
  _midiNote = 69;
  _currentStep = 1.0f;
  _targetStep = 1.0f;
  _tuning = 1.0f;
  _pitchbend = 1.0f;
  _deltaStep = 0.0f;
  _slideMs = 60.0f;
  _phaze = 0.0f;
  mvaStack.n = 0 ;
  _pan = 0.5;

  AmpEnv.init(SAMPLE_RATE);
  FltEnv.init(SAMPLE_RATE);
  osc.setSampleRate(SAMPLE_RATE);
  osc.setWaveform(MipmapOscillator::Waveform::Saw);
  // parameters of envelopes
  _ampAttackMs = 0.5;
  _ampDecayMs = 1230.0;
  _ampReleaseMs = 1.0;
  _filterAttackMs = 3.0;
  _filterDecayMs = 1000.0;
  _filterAccentAttackMs = 30.0;
  _filterAccentDecayMs = 200.0;
  
  Distortion.Init();
  Drive.Init();

  Filter.Init((float)SAMPLE_RATE);
  
#if FILTER_TYPE == 2
//  Filter.SetMode(TeeBeeFilter::LP_18);
  Filter.SetMode(TeeBeeFilter::TB_303);
#endif
  highpass1.setMode(OnePoleFilter::HIGHPASS);
  highpass1.setCutoff(44.486f);
  highpass2.setMode(OnePoleFilter::HIGHPASS);
  highpass2.setCutoff(24.167f);
  allpass.setMode(OnePoleFilter::ALLPASS);
  allpass.setCutoff(14.008f);
  ampDeclicker.setMode(BiquadFilter::LOWPASS12);
  ampDeclicker.setGain( amp2dB(sqrt(0.5f)) );
  ampDeclicker.setFrequency(200.0f);
  filtDeclicker.setMode(BiquadFilter::LOWPASS12);
  filtDeclicker.setGain( amp2dB(sqrt(0.5f)) );
  filtDeclicker.setFrequency(200.0f);
  notch.setMode(BiquadFilter::BANDREJECT);
  notch.setFrequency(7.5164f);
  notch.setBandwidth(4.7f);
}


inline float SynthVoice::getSample() {
  
    float samp = 0.0f, filtEnv = 0.0f, ampEnv = 0.0f, final_cut = 0.0f;
    filtEnv = FltEnv.process();
    
    ampEnv = AmpEnv.process() * _k_acc;
    
    if (AmpEnv.isRunning()) {
      samp = osc.process();
    } else {
      samp = 0.0f;
    }
    final_cut = (float)_filter_freq_cut * (0.8f + (_envMod+0.1f) * (3*filtEnv - 0.3f) * (_accentation + 0.2f) );
    Filter.SetCutoff( final_cut );    

    /*
     decimator++;
     if (decimator % 128 == 0 && _index == 0) {
      DEBF("%f\r\n", samp);
     }
    */
    
    samp = highpass1.getSample(samp);         // pre-filter highpass, following open303
    
    samp = allpass.getSample(samp);           // phase correction, following open303
   
    samp = Filter.Process(samp);              // main filter
    
    samp = highpass2.getSample(samp);         // post-filtering, following open303
    
    samp = notch.getSample(samp);             // post-filtering, following open303
    
    samp = Drive.Process(samp);               // overdrive
    
    samp = Distortion.Process(samp);          // distortion
    
    samp *= ampEnv;                           // amp envelope


    _compens = _volume * 8.0f * _fx_compens ; // * _flt_compens;

    _compens = ampDeclicker.getSample(_compens);
   
    samp *=  _compens;


    //synth_buf[_index][i] = fast_shape(samp); // mono limitter
    return  samp;  
}


inline void SynthVoice::SetCutoff(float normalized_val)  {
  _cutoff = normalized_val;
  _filter_freq = knobMap( normalized_val, MIN_CUTOFF_FREQ, MAX_CUTOFF_FREQ);
  _filter_freq_mod = knobMap( normalized_val, MIN_CUTOFF_FREQ_MOD, MAX_CUTOFF_FREQ_MOD);
  _filter_freq_cut = knobMap( _envMod, _filter_freq, _filter_freq_mod);
#ifdef DEBUG_SYNTH
  DEBF("Synth %d cutoff=%0.3f freq=%0.3f\r\n" , _index, _cutoff, _filter_freq);
#endif
}


inline void SynthVoice::SetEnvModLevel(float normalized_val) {
  _envMod = normalized_val;
  _filter_freq_cut = knobMap( normalized_val, _filter_freq, _filter_freq_mod);
};


inline void SynthVoice::PitchBend(int number) {
  float semi = ((((float)number + 8191.5f) * (float)TWO_DIV_16383 ) - 1.0f ) * 12.0f;
  _pitchbend = powf(1.059463f, semi);
  osc.setPitchbendMod(_pitchbend);
}

inline void SynthVoice::ParseCC(uint8_t cc_number , uint8_t cc_value) {
  float tmp = 0.0f;
  switch (cc_number) {

    case CC_303_PORTATIME:
      _slideMs = (float)cc_value;
      break;
    case CC_303_VOLUME:
      _volume = (float)cc_value * MIDI_NORM;
      break;
    case CC_303_PAN:
      _pan = (float)cc_value * MIDI_NORM;
      break;
    case CC_303_PORTAMENTO:
      _portamento = (cc_value >= 64);
      break;
    case CC_303_WAVEFORM:
      if (cc_value > 63) {
        osc.setWaveform(MipmapOscillator::Waveform::Saw);
      } else {
        osc.setWaveform(MipmapOscillator::Waveform::Square);
      }
      break;
    case CC_303_RESO:
      _reso = cc_value * MIDI_NORM ;
      _flt_compens = one_div( bilinearLookup(norm1_tbl, _cutoff * 127.0f, cc_value ));
      SetReso(_reso);
      break;
    case CC_303_DECAY: // Env release
      tmp = (float)cc_value * MIDI_NORM;
      _filterDecayMs = knobMap(tmp, 200.0f, 2000.0f);
      //_ampDecayMs = knobMap(tmp, 15.0f, 7500.0f);
      break;
    case CC_303_ATTACK: // Env attack
      tmp = (float)cc_value * MIDI_NORM;
      _filterAttackMs = knobMap(tmp, 3.0f, 100.0f);
      _ampAttackMs =  knobMap(tmp, 0.1f, 500.0f);
      break;
    case CC_303_CUTOFF:
      _cutoff = (float)cc_value * MIDI_NORM;
      _flt_compens = one_div( bilinearLookup(norm1_tbl, cc_value, _reso * 127.0f));
      SetCutoff(_cutoff);
      break;
    case CC_303_DELAY_SEND:
      _sendDelay = (float)cc_value * MIDI_NORM;
      break;
    case CC_303_REVERB_SEND:
      _sendReverb = (float)cc_value * MIDI_NORM;
      break;
    case CC_303_ENVMOD_LVL:
      SetEnvModLevel ( (float)cc_value * MIDI_NORM ) ;
      break;
    case CC_303_ACCENT_LVL:
      _accentLevel = (float)cc_value * MIDI_NORM;
      break;
    case CC_303_DISTORTION:
      _gain = (float)cc_value * MIDI_NORM ;
      _fx_compens = one_div( bilinearLookup(norm2_tbl, _drive * 127.0f,  cc_value));
      SetDistortionLevel(_gain);
      break;
    case CC_303_OVERDRIVE:
      _drive = (float)cc_value * MIDI_NORM ;
      _fx_compens = one_div( bilinearLookup(norm2_tbl, cc_value, _gain * 127.0f));
      SetOverdriveLevel(_drive);
      break;
    case CC_303_SATURATOR:
      _saturator = (float)cc_value * MIDI_NORM;
      Filter.SetDrive(_saturator);
      break;
    case CC_303_TUNING:
      _tuning = tuning[cc_value];
      osc.setTuningMod(_tuning);
      break;
  }
}


// The following code initially written by Anton Savov,
// is taken from http://antonsavov.net/cms/projects/303andmidi.html
// Monophonic Voice Allocator (with Accent, suitable for the 303)
// "Newest" note-priority rule
// Modified version, allows multiple Notes with the same pitch

inline void SynthVoice::on_midi_noteON(uint8_t note, uint8_t velocity)
{
  mva_alloc(note, (velocity >= 80));

  bool slide = (mvaStack.n > 1);
  bool accent = (mvaStack.accents[0]);
  note = mvaStack.notes[0] ;
  note_on(note, slide, accent);
}

inline void SynthVoice::on_midi_noteOFF(uint8_t note, uint8_t velocity)
{
  if (mvaStack.n == 0) {
    return;
  }
  uint8_t tmp_note = mvaStack.notes[0];
  uint8_t tmp_accent = mvaStack.accents[0];
  mva_free(note);

  if (mvaStack.n > 0)
  {
    if (mvaStack.notes[0] != tmp_note)
    {
      bool accent = (mvaStack.accents[0] );
      bool slide = 1;
      note = mvaStack.notes[0];

      note_on(note, slide, accent);
    }
  }
  else {
    note_off();
  }
}

void SynthVoice::mva_alloc(uint8_t note, uint8_t accent)
{
  uint8_t s = 0;
  uint8_t i = 0;

  // shift all notes back
  uint8_t m = mvaStack.n + 1;
  m = (m > MIDI_MVA_SZ ? MIDI_MVA_SZ : m);
  s = m;
  i = m;
  while (i > 0)
  {
    --s;
    mvaStack.notes[i] = mvaStack.notes[s];
    mvaStack.accents[i] = mvaStack.accents[s];
    i = s;
  }
  // put the new note first
  mvaStack.notes[0] = note;
  mvaStack.accents[0] = accent;
  // update the voice counter
  mvaStack.n = m;
}

void SynthVoice::mva_free(uint8_t note)
{
  uint8_t s = 0;

  // find if the note is actually in the buffer
  uint8_t m = mvaStack.n;
  uint8_t i = m;
  while (i) // count backwards (oldest notes first)
  {
    --i;
    if (note == mvaStack.notes[i] )
    {
      // found it!
      if (i < (mvaStack.n - 1)) // don't shift if this was the last note..
      {
        // remove it now.. just shift everything after it
        s = i;
        while (i < m)
        {
          ++s;
          mvaStack.notes[i] = mvaStack.notes[s];
          mvaStack.accents[i] = mvaStack.accents[s];
          i = s;
        }
      }
      // update the voice counter
      if (m > 0) {
        mvaStack.n = m - 1;
      }
      break;
    }
  }
}

void SynthVoice::mva_reset() {
  mvaStack.n = 0;
}

void  SynthVoice::note_on(uint8_t midiNote, bool slide, bool accent) {
  _accent = accent;
  _slide = slide || _portamento;
  if (mvaStack.n == 1) {
    if (_accent) {
      _accentation = _accentLevel; 
      AmpEnv.setReleaseTimeMs(_ampReleaseMs * 50.0f);
      FltEnv.setDecayTimeMs(_filterAccentDecayMs );
      FltEnv.setAttackTimeMs(_filterAccentAttackMs );
    } else {
      _accentation = 0.0f;
      AmpEnv.setReleaseTimeMs( _ampReleaseMs );
      FltEnv.setDecayTimeMs(_filterDecayMs );
      FltEnv.setAttackTimeMs(_filterAttackMs );
    }
  }
  AmpEnv.setAttackTimeMs(_ampAttackMs);
  AmpEnv.setDecayTimeMs(_ampDecayMs);
  osc.setGlide(_slide);
  osc.setFrequency(midi_pitches[midiNote]);
  if (!_slide) {
    osc.resetPhase();
    AmpEnv.retrigger(Adsr::END_FAST);
    FltEnv.retrigger(false);    
  }
  _k_acc = (1.0f + 0.6f * _accentation);
 // DEBF ("ampRelease %f \t ampDecay %f \t \r\n", _ampReleaseMs, _ampDecayMs );
}

void  SynthVoice::note_off() {
  AmpEnv.end(Adsr::END_REGULAR);
 // FltEnv.end(false);
}

#define _USE_MATH_DEFINES

#include <cmath>
#include <queue>
#include <atomic>
#include <thread>
#include <string>
#include <vector>
#include <map>
#include <kiss_fftr.h>
#include <SoundTouchDLL.h>
#include <SoundTouch.h>
#include <piper.h>
#include "unnu_tts/cxx-api.h"

#if defined(_WIN32)
    #include <windows.h>
    #define UNNUTTS_LOAD_LIB(name) LoadLibraryA(name)
    #define UNNUTTS_GET_PROC(lib, name) GetProcAddress((HMODULE)lib, name)
    #define UNNUTTS_LIB_HANDLE HMODULE
#elif defined(__linux__) || defined(__APPLE__)
    #include <dlfcn.h>
    #define UNNUTTS_LOAD_LIB(name) dlopen(name, RTLD_NOW | RTLD_LOCAL)
    #define UNNUTTS_GET_PROC(lib, name) dlsym(lib, name)
    #define UNNUTTS_LIB_HANDLE void*
#endif

#define NUM_EMOTIONS 15

#define SAMPLE_RATE 22050

typedef void* STProc_HANDLE;

typedef struct piper_synthesizer_deleter {
	void operator()(piper_synthesizer* synth) {
		if (synth != NULL) {
			piper_free(synth);
		}
	}
} piper_synthesizer_deleter_t;

typedef std::unique_ptr<piper_synthesizer, piper_synthesizer_deleter_t> piper_synthesizer_ptr;

typedef struct ut_audio_sample_deleter {
	void operator()(ut_audio_sample_t* audio) {
		ut_audio_sample_free(audio);
	}
} ut_audio_sample_deleter_t;

typedef std::unique_ptr<ut_audio_sample_t, ut_audio_sample_deleter_t> audio_sample_ptr;

typedef struct speaker_state {
    int32_t speaker;
	STProc_HANDLE stEmotions;
	STProc_HANDLE stPitch;
	STProc_HANDLE stFormant;
	int32_t sampleRate;
    EmotionDSPParams_t blendedParams;
    float currentPitch, currentFormantShift, currentEQFreq, currentEQGain;
    float currentCompThreshold, currentCompRatio, currentReverbAmount, currentDistortionAmount;
} speaker_state_t;

void ut_speaker_state_free(speaker_state_t* state) {
    if (state != NULL) {
        if (state->stEmotions != NULL) {
            soundtouch_destroyInstance(state->stEmotions);
        }
        if (state->stPitch != NULL) {
            soundtouch_destroyInstance(state->stPitch);
        }
        if (state->stFormant != NULL) {
            soundtouch_destroyInstance(state->stFormant);
        }
        free(state);
    }
}

typedef struct ut_speaker_state_deleter {
    void operator()(speaker_state_t* state) {
        ut_speaker_state_free(state);
    }
} ut_speaker_state_deleter_t;

typedef std::unique_ptr<speaker_state_t, ut_speaker_state_deleter_t> speaker_state_ptr;

typedef struct ut_speaker {
	std::string name;
	piper_synthesize_options options;
    speaker_state_ptr state;
    unnutts::piper_synthesizer_ptr synthesizer;
} ut_speaker_t;

void ut_speaker_free(ut_speaker_t* speaker){
    if(speaker != NULL){
        if(speaker->state != nullptr){
            speaker->state.reset(nullptr);
        }
        if(speaker->synthesizer != nullptr){
			speaker->synthesizer.reset(nullptr);
        }
        free(speaker);
    }
}

typedef struct ut_speaker_deleter {
    void operator()(ut_speaker_t* speaker) {
        ut_speaker_free(speaker);
    }
} ut_speaker_deleter_t;

typedef std::unique_ptr<ut_speaker, ut_speaker_deleter_t> ut_speaker_ptr;

// 2. Get OrtGetApiBase function pointer
typedef const OrtApiBase* (*OrtGetApiBaseFn)();

static OrtGetApiBaseFn GetApiBase = nullptr;

static const OrtApi* ut_load_ort(OrtApiBase* BaseApi)
{
    if (BaseApi)
    {
        auto result = BaseApi->GetApi(ORT_API_VERSION);
        return result;
    }
	if(GetApiBase)
	{
        auto result = GetApiBase()->GetApi(ORT_API_VERSION);
		return result;
	}
	// 1. Load ONNX Runtime shared library manually
    #if defined(_WIN32)
        UNNUTTS_LIB_HANDLE ortLib = UNNUTTS_LOAD_LIB("onnxruntime.dll");
    #elif defined(__APPLE__)
        UNNUTTS_LIB_HANDLE ortLib = UNNUTTS_LOAD_LIB("libonnxruntime.dylib");
    #else
        UNNUTTS_LIB_HANDLE ortLib = UNNUTTS_LOAD_LIB("libonnxruntime.so");
    #endif
	if (!ortLib) {
		// throw std::runtime_error("Failed to load ONNX Runtime library.");
		return nullptr;
	}
	GetApiBase = reinterpret_cast<OrtGetApiBaseFn>(UNNUTTS_GET_PROC(ortLib, "OrtGetApiBase"));
	if (!GetApiBase) {
		// throw std::runtime_error("Failed to get OrtGetApiBase function.");
		return nullptr;
	}
	// 3. Initialize ONNX Runtime API
    auto result = GetApiBase()->GetApi(ORT_API_VERSION);
	return result;
}

static std::map<int32_t, ut_speaker_ptr> g_speakers;

EmotionDSPParams_t emotionPresets[NUM_EMOTIONS] = {
    // Happy
    { +2.0f, 1.05f, 3500.0f, 1.5f, 0.6f, 3.0f, 0.2f, 0.0f },
    // Sad
    { -2.0f, 0.95f, 200.0f, 1.3f, 0.5f, 2.0f, 0.4f, 0.0f },
    // Afraid
    { +3.0f, 1.10f, 4000.0f, 1.6f, 0.5f, 4.0f, 0.1f, 0.1f },
    // Surprise
    { +4.0f, 1.15f, 4500.0f, 1.7f, 0.5f, 4.0f, 0.05f, 0.0f },
    // Disgust
    { -1.0f, 0.90f, 250.0f, 1.4f, 0.6f, 3.0f, 0.3f, 0.05f },
    // Excited
    { +3.0f, 1.08f, 3800.0f, 1.5f, 0.5f, 4.0f, 0.05f, 0.1f },
    // Joyous
    { +2.5f, 1.06f, 3600.0f, 1.5f, 0.6f, 3.0f, 0.1f, 0.0f },
    // Distressed
    { +1.0f, 0.97f, 3000.0f, 1.4f, 0.5f, 4.0f, 0.05f, 0.1f },
    // Depressed
    { -3.0f, 0.90f, 180.0f, 1.3f, 0.4f, 2.0f, 0.5f, 0.0f },
    // Bored
    { -1.5f, 0.95f, 250.0f, 1.2f, 0.5f, 2.0f, 0.4f, 0.0f },
    // Sleepy
    { -2.0f, 0.92f, 200.0f, 1.2f, 0.4f, 2.0f, 0.6f, 0.0f },
    // Calm
    { 0.0f, 1.00f, 2500.0f, 1.3f, 0.5f, 2.0f, 0.3f, 0.0f },
    // Relaxed
    { 0.0f, 1.00f, 2400.0f, 1.3f, 0.5f, 2.0f, 0.4f, 0.0f },
    // Trust
    { +0.5f, 1.02f, 2600.0f, 1.4f, 0.5f, 2.0f, 0.2f, 0.0f },
    // Content
    { +0.5f, 1.01f, 2500.0f, 1.3f, 0.5f, 2.0f, 0.2f, 0.0f }
};

typedef struct EmotionCoord {
    EEMOTION_t name;
    float valence; // -1.0 to +1.0
    float arousal; // -1.0 to +1.0
} EmotionCoord_t;

EmotionCoord_t emotionCoords[NUM_EMOTIONS] = {
    {EMOTION_HAPPY,     0.8f,  0.7f},
    {EMOTION_SAD,      -0.8f, -0.6f},
    {EMOTION_AFRAID,   -0.7f,  0.8f},
    {EMOTION_SURPRISE,  0.5f,  0.9f},
    {EMOTION_DISGUST,  -0.9f,  0.4f},
    {EMOTION_EXCITED,   0.9f,  0.9f},
    {EMOTION_JOY,       1.0f,  0.8f},
    {EMOTION_DISTRESS, -0.8f,  0.7f},
    {EMOTION_DEPRESSED,-1.0f, -0.8f},
    {EMOTION_BORED,    -0.5f, -0.7f},
    {EMOTION_SLEEPY,    0.0f, -0.9f},
    {EMOTION_CALM,      0.6f, -0.5f},
    {EMOTION_RELAXED,   0.7f, -0.4f},
    {EMOTION_TRUST,     0.8f, -0.2f},
    {EMOTION_CONTENT,   0.9f, -0.3f}
};

// Normalize audio
void normalize(std::vector<float> &audio) {
    float maxVal = 0.0f;
    for (auto &s : audio) maxVal =  maxVal > std::fabs(s) ? maxVal : std::fabs(s);
    if (maxVal > 0.0f) {
        for (auto &s : audio) s /= maxVal;
    }
}

// Apply short metallic delay
void addShortDelay(std::vector<float> &audio, int sampleRate, float delayMs, float mix) {
    int delaySamples = static_cast<int>((delayMs / 1000.0f) * sampleRate);
    std::vector<float> delayed(audio.size(), 0.0f);

    for (size_t i = delaySamples; i < audio.size(); ++i) {
        delayed[i] = audio[i - delaySamples];
    }

    for (size_t i = 0; i < audio.size(); ++i) {
        audio[i] = (1.0f - mix) * audio[i] + mix * delayed[i];
    }
}

void ut_audio_sample_free(ut_audio_sample_t* sample){
	if(sample != NULL){
		if(sample->samples != NULL){
			free(sample->samples);
		}
		free(sample);
	}
}

ut_audio_sample_t* ut_vector_to_audio_sample(speaker_state_t* spk, std::vector<float> &processed){
	ut_audio_sample_t* sample = (ut_audio_sample_t*) malloc(sizeof(ut_audio_sample_t));
	sample->sample_rate = spk->sampleRate;
	sample->num_samples = processed.size();
	sample->samples = (float*) calloc(sizeof(float), processed.size());
	std::memcpy(sample->samples, processed.data(), sizeof(float) * processed.size());
	return sample;
}

EmotionDSPParams_t blendEmotions(float* emotionWeights) {
    EmotionDSPParams result = {0};
    float totalWeight = 0.0f;

    for (int i = 0; i < NUM_EMOTIONS; i++) {
        if (emotionWeights[i] > 0.0f) {
            result.pitchSemiTones   += emotionPresets[i].pitchSemiTones   * emotionWeights[i];
            result.formantShift     += emotionPresets[i].formantShift     * emotionWeights[i];
            result.eqFreq           += emotionPresets[i].eqFreq           * emotionWeights[i];
            result.eqGain           += emotionPresets[i].eqGain           * emotionWeights[i];
            result.compThreshold    += emotionPresets[i].compThreshold    * emotionWeights[i];
            result.compRatio        += emotionPresets[i].compRatio        * emotionWeights[i];
            result.reverbAmount     += emotionPresets[i].reverbAmount     * emotionWeights[i];
            result.distortionAmount += emotionPresets[i].distortionAmount * emotionWeights[i];
            totalWeight += emotionWeights[i];
        }
    }

    if (totalWeight > 0.0f) {
        result.pitchSemiTones   /= totalWeight;
        result.formantShift     /= totalWeight;
        result.eqFreq           /= totalWeight;
        result.eqGain           /= totalWeight;
        result.compThreshold    /= totalWeight;
        result.compRatio        /= totalWeight;
        result.reverbAmount     /= totalWeight;
        result.distortionAmount /= totalWeight;
    }

    return result;
}

EmotionDSPParams_t fromValenceArousalToBlendedEmotions(float valence, float arousal) {
    float totalWeight = 0.0f;
	float emotionWeights[NUM_EMOTIONS] = {0}; // dynamic weights
    // Compute weights based on inverse distance
    for (int i = 0; i < NUM_EMOTIONS; i++) {
        float dx = valence - emotionCoords[i].valence;
        float dy = arousal - emotionCoords[i].arousal;
        float dist = sqrtf(dx*dx + dy*dy);

        // Avoid division by zero
        float weight = (dist < 0.00001f) ? 1.0f : 1.0f / (dist + 0.00001f);

        emotionWeights[i] = weight;
        totalWeight += weight;
    }

    // Normalize weights
    for (int i = 0; i < NUM_EMOTIONS; i++) {
        emotionWeights[i] /= totalWeight;
    }

    // Blend DSP parameters
    return blendEmotions(emotionWeights);
}

// Smooth parameter transitions
float smooth(float current, float target, float alpha) {
    return current + alpha * (target - current);
}

// EQ + compression
void applyEQandCompression(float *samples, int count, int sampleRate,
                           float eqFreq, float eqGain,
                           float compThreshold, float compRatio) {
    float omega = 2.0f * M_PI * eqFreq / sampleRate;
    float alpha = sinf(omega) / (2.0f * 0.707f); // Q=0.707
    float b0 = 1 + alpha * eqGain;
    float b1 = -2 * cosf(omega);
    float b2 = 1 - alpha * eqGain;
    float a0 = 1 + alpha / eqGain;
    float a1 = -2 * cosf(omega);
    float a2 = 1 - alpha / eqGain;

    float x1 = 0, x2 = 0, y1 = 0, y2 = 0;
    for (int i = 0; i < count; i++) {
        float x0 = samples[i];
        float y0 = (b0/a0)*x0 + (b1/a0)*x1 + (b2/a0)*x2
                   - (a1/a0)*y1 - (a2/a0)*y2;

        x2 = x1; x1 = x0;
        y2 = y1; y1 = y0;
        // Compression
        float absY = fabs(y0);
        if (absY > compThreshold) {
            float excess = absY - compThreshold;
            y0 = (y0 > 0 ? 1 : -1) * (compThreshold + excess / compRatio);
        }
        samples[i] = y0;
        
    }
}

// Formant shifting with overlap-add & interpolation
void shiftFormants(float *samples, int count, float shiftFactor) {
    kiss_fft_cfg cfg = kiss_fft_alloc(count, 0, nullptr, nullptr);
    std::vector<kiss_fft_cpx> in(count), out(count);

    for (int i = 0; i < count; i++) {
        in[i].r = samples[i];
        in[i].i = 0;
    }

    kiss_fft(cfg, in.data(), out.data());

    int shiftBins = (int)((count / 2) * (shiftFactor - 1.0f));
    std::vector<kiss_fft_cpx> shifted(count);
    for (int i = 0; i < count; i++) {
        int src = i - shiftBins;
        if (src >= 0 && src < count)
            shifted[i] = out[src];
        else
            shifted[i] = {0, 0};
    }

    kiss_fft(cfg, shifted.data(), in.data()); // inverse FFT
    for (int i = 0; i < count; i++)
        samples[i] = in[i].r / count;

    free(cfg);
}

// Simple reverb
void applyReverb(float *samples, int count, float amount) {
    if (amount <= 0.0f) return;
    static float delayBuffer[22050] = {0};
    static int delayIndex = 0;
    int delaySamples = 1000;
    for (int i = 0; i < count; i++) {
        float delayed = delayBuffer[delayIndex];
        delayBuffer[delayIndex] = samples[i] + delayed * 0.5f;
        samples[i] += delayed * amount;
        delayIndex = (delayIndex + 1) % delaySamples;
    }
}

// Simple distortion
void applyDistortion(float *samples, int count, float amount) {
    if (amount <= 0.0f) return;
    for (int i = 0; i < count; i++) {
        float s = samples[i] * (1.0f + amount * 5.0f);
        if (s > 1.0f) s = 1.0f;
        if (s < -1.0f) s = -1.0f;
        samples[i] = s;
    }
}

EmotionDSPParams_t ut_get_emotion_settings(EEMOTION_t setting){
	float emotionWeights[NUM_EMOTIONS] = {0.0f};
	if(setting != EEMOTION::EMOTION_NEUTRAL){
		emotionWeights[setting] = 1.0f;
	}
	return blendEmotions(emotionWeights);
}
static std::map<std::pair<int32_t, bool>, EEMOTION_t> blendedParamsCache;

void ut_set_speaker_emotion(int32_t speaker_id, bool is_robot, EEMOTION_t setting) {
    std::pair<int32_t, bool> cacheKey = { speaker_id, is_robot };
    blendedParamsCache[cacheKey] = setting;
}

void ut_update_sfx(int32_t speaker_id, EmotionDSPParams_t blendedParams, float alpha){
    speaker_state_t* state = g_speakers[speaker_id]->state.get();
	// Smooth params
    state->currentPitch        = smooth(state->currentPitch, blendedParams.pitchSemiTones, alpha);
    state->currentFormantShift = smooth(state->currentFormantShift, blendedParams.formantShift, alpha);
    state->currentEQFreq       = smooth(state->currentEQFreq, blendedParams.eqFreq, alpha);
    state->currentEQGain       = smooth(state->currentEQGain, blendedParams.eqGain, alpha);
    state->currentCompThreshold= smooth(state->currentCompThreshold, blendedParams.compThreshold, alpha);
    state->currentCompRatio    = smooth(state->currentCompRatio, blendedParams.compRatio, alpha);
    state->currentReverbAmount = smooth(state->currentReverbAmount, blendedParams.reverbAmount, alpha);
    state->currentDistortionAmount = smooth(state->currentDistortionAmount, blendedParams.distortionAmount, alpha);
}

// Apply emotions
ut_audio_sample_t* ut_apply_sfx(speaker_state_t* state, float *samples, int count, bool is_robot){
	// Pitch shift
	soundtouch_setPitchSemiTones(state->stEmotions, state->currentPitch);
	soundtouch_putSamples(state->stEmotions, samples, count);
    std::vector<float> processed(count);
	int received = soundtouch_receiveSamples(state->stEmotions, processed.data(), count);

	// Formant shift
	shiftFormants(processed.data(), received, state->currentFormantShift);

	// EQ + compression
	applyEQandCompression(processed.data(), received, state->sampleRate,
        state->blendedParams.eqFreq, state->blendedParams.eqGain,
        state->blendedParams.compThreshold, state->blendedParams.compRatio);

	// Reverb + distortion
	applyReverb(processed.data(), received, state->blendedParams.reverbAmount);
	applyDistortion(processed.data(), received, state->blendedParams.distortionAmount);
	
	if (is_robot) {
		soundtouch_putSamples(state->stPitch, processed.data(), received);
		std::vector<float> pitched(received);
		int receivedPitched = soundtouch_receiveSamples(state->stPitch, pitched.data(), received);
		
		soundtouch_putSamples(state->stFormant, pitched.data(), receivedPitched);
		processed.clear(); processed.resize(receivedPitched);
		received = soundtouch_receiveSamples(state->stFormant, processed.data(), receivedPitched);
		
		// Add short metallic delay (~15ms) for C-3PO style
		addShortDelay(processed, state->sampleRate, 15.0f, 0.35f);
		
		// Normalize
		normalize(processed);
	}
	
	processed.resize(received);
	
	return ut_vector_to_audio_sample(state, processed);
}

void ut_add_speaker(const char* model_path, int32_t voice_id, int32_t speaker_id, const char* actor_name){ // sid speaker id, vid voice id
	auto getApiBase = ut_load_ort(nullptr);
	if (!getApiBase)
	{
		return;
	}
	ut_speaker_t* speaker = (ut_speaker_t *) malloc(sizeof(ut_speaker_t));
	speaker->synthesizer = unnutts::piper_synthesizer_ptr(piper_create(model_path, NULL, NULL));

	speaker->name = std::string(actor_name);
	speaker->options = piper_default_synthesize_options(speaker->synthesizer.get());
	speaker->options.speaker_id = voice_id;
	
    speaker_state_t*  state = (speaker_state_t*) std::malloc(sizeof(speaker_state_t));
	state->speaker = speaker_id;
	state->sampleRate = SAMPLE_RATE;
	state->stEmotions = soundtouch_createInstance();
	soundtouch_setChannels(state->stEmotions, 1);
	soundtouch_setSampleRate(state->stEmotions, SAMPLE_RATE);
	soundtouch_setSetting(state->stEmotions, SETTING_USE_QUICKSEEK, 0);
	soundtouch_setSetting(state->stEmotions, SETTING_USE_AA_FILTER, 1);
	
	state->stPitch = soundtouch_createInstance();
	soundtouch_setChannels(state->stPitch, 1);
	soundtouch_setSampleRate(state->stPitch, SAMPLE_RATE);
	soundtouch_setTempo(state->stPitch, 1.0f);
	soundtouch_setPitchSemiTones(state->stPitch, 2.0f);
	
	state->stFormant = soundtouch_createInstance();
	soundtouch_setChannels(state->stFormant, 1);
	soundtouch_setSampleRate(state->stFormant, SAMPLE_RATE);
	soundtouch_setTempo(state->stFormant, 1.05f);
	soundtouch_setPitchSemiTones(state->stFormant, 0.0f);
	speaker->state = speaker_state_ptr(state);

	// Use emplace to avoid copy/move assignment of unnu_speaker_t
	g_speakers.emplace(speaker_id, std::move(ut_speaker_ptr(speaker)));
}


void ut_rm_speaker(int32_t speaker_id) {
	auto search = g_speakers.find(speaker_id);
	if (search != g_speakers.end()){
		search->second.reset(nullptr);
		g_speakers.erase(speaker_id);
	}
}

int32_t ut_get_speaker_id(const char* actor_name){
	std::string name(actor_name);
	for (const auto& [key, value] : g_speakers){
		if (value->name.compare(name) == 0){
			return key;
		}
	}
	return -1;
}

void ut_terminate() {
	for (auto& [key, value] : g_speakers){
        value.reset(nullptr); // This will call the deleter for ut_speaker_t, which in turn calls ut_speaker_free
	}
	g_speakers.clear();
}

ut_audio_sample_t* unnu_tts(int32_t speaker_id, EEMOTION_t emotion, bool is_robot, const char* text){
	auto& blendedparams = ut_get_emotion_settings(emotion);
	ut_speaker_t* speaker = g_speakers[speaker_id].get();
	ut_update_sfx(speaker_id, blendedparams, 1.0f);
	piper_audio_chunk chunk;
	std::vector<float> _audio;
	piper_synthesizer* synth = speaker->synthesizer.get();
	piper_synthesize_start(synth, text,
						   &(speaker->options) /* NULL for defaults */);
	while (piper_synthesize_next(synth, &chunk) != PIPER_DONE) {	
		_audio.insert(_audio.end(), chunk.samples, chunk.samples + chunk.num_samples);
	}
	return ut_apply_sfx(speaker->state.get(), _audio.data(), _audio.size(), is_robot);
}

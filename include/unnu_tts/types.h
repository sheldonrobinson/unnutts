#ifndef _UNNU_TTS_TYPES_H
#define _UNNU_TTS_TYPES_H

#ifdef __cplusplus
	#include <cstdint>
	#include <cstdbool>
#else // __cplusplus - Objective-C or other C platform
	#include <stdint.h>
	#include <stdbool.h>
#endif

#ifdef __cplusplus
	#include <cstdint>
	#include <cstdbool>
#else // __cplusplus - Objective-C or other C platform
	#include <stdint.h>
	#include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct ut_audio_sample {
  /**
   * \brief Raw samples returned from the voice model.
   */
  float *samples;

  /**
   * \brief Number of samples in the audio chunk.
   */
  size_t num_samples;

  /**
   * \brief Sample rate in Hertz.
   */
  int sample_rate;
} ut_audio_sample_t;

typedef enum EEMOTION {
	EMOTION_HAPPY = 0,
	EMOTION_SAD = 1,
	EMOTION_AFRAID = 2,
	EMOTION_SURPRISE = 3,
	EMOTION_DISGUST = 4,
	EMOTION_EXCITED = 5,
	EMOTION_JOY = 6,
	EMOTION_DISTRESS = 7,
	EMOTION_DEPRESSED = 8,
	EMOTION_BORED = 9,
	EMOTION_SLEEPY = 10,
	EMOTION_CALM = 11,
	EMOTION_RELAXED = 12,
	EMOTION_TRUST = 13,
	EMOTION_CONTENT = 14,
	EMOTION_NEUTRAL = 15
} EEMOTION_t ;


typedef struct EmotionDSPParams {
    float pitchSemiTones;
    float formantShift;
    float eqFreq;
    float eqGain;
    float compThreshold;
    float compRatio;
    float reverbAmount;
    float distortionAmount;
} EmotionDSPParams_t;

#ifdef __cplusplus
}
#endif


	


#endif // _UNNU_TTS_TYPES_H
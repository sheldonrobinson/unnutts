#ifndef _UNNU_TTS_H
#define _UNNU_TTS_H

#ifdef UNNU_TTS_SHARED
#ifdef __cplusplus
#    if defined(_WIN32) && !defined(__MINGW32__)
#        ifdef UNNU_TTS_BUILD
#            define UNNU_TTS_API extern "C" __declspec(dllexport)
#        else
#            define UNNU_TTS_API extern "C" __declspec(dllimport)
#        endif
#    else
#        define UNNU_TTS_API extern "C" __attribute__((visibility("default"))) __attribute__((used))
#    endif
#else
#    if defined(_WIN32) && !defined(__MINGW32__)
#        ifdef UNNU_TTS_BUILD
#            define UNNU_TTS_API __declspec(dllexport)
#        else
#            define UNNU_TTS_API __declspec(dllimport)
#        endif
#    else
#        define UNNU_TTS_API __attribute__((visibility("default"))) __attribute__((used))
#    endif	
#endif
#else
#define UNNU_TTS_API
#endif

#include "types.h"

UNNU_TTS_API void ut_terminate(); 

UNNU_TTS_API void ut_add_speaker(const char* model_path, int32_t voice_id, int32_t speaker_id, const char* name); // sid speaker id, vid voice id

UNNU_TTS_API void ut_rm_speaker(const char* name); 

UNNU_TTS_API int32_t ut_get_speaker_id(const char* name); 

UNNU_TTS_API EmotionDSPParams_t ut_get_emotion_settings(EEMOTION_t setting);

UNNU_TTS_API void ut_set_speaker_emotion(int32_t speaker_id, bool is_robot, EEMOTION_t setting);

UNNU_TTS_API void ut_update_sfx(int32_t speaker_id, EmotionDSPParams_t blendedParams, float alpha);

UNNU_TTS_API ut_audio_sample_t* unnu_tts(int32_t speaker_id, EEMOTION_t emotion, bool is_robot, const char* text);

UNNU_TTS_API void ut_audio_sample_free(ut_audio_sample_t* audio);

#endif // _UNNU_TTS_H
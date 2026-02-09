#pragma once

#include "c-api.h"
#include <string>
#include <piper.h>

namespace unnutts
{

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
	
	
	static EEMOTION emotion_from_string(const std::string & emotion_str) {
		if (emotion_str == "happy") {
			return EEMOTION::EMOTION_HAPPY;
		}
		if (emotion_str == "sad") {
			return EEMOTION::EMOTION_SAD;
		}
		if (emotion_str == "afraid") {
			return EEMOTION::EMOTION_AFRAID;
		}
		if (emotion_str == "afraid") {
			return EEMOTION::EMOTION_SURPRISE;
		}
		if (emotion_str == "disgust") {
			return EEMOTION::EMOTION_DISGUST;
		}
		if (emotion_str == "excited") {
			return EEMOTION::EMOTION_EXCITED;
		}
		if (emotion_str == "joy") {
			return EEMOTION::EMOTION_JOY;
		}
		if (emotion_str == "distress") {
			return EEMOTION::EMOTION_DISTRESS;
		}
		if (emotion_str == "depressed") {
			return EEMOTION::EMOTION_DEPRESSED;
		}
		if (emotion_str == "bored") {
			return EEMOTION::EMOTION_BORED;
		}
		if (emotion_str == "sleepy") {
			return EEMOTION::EMOTION_SLEEPY;
		}
		if (emotion_str == "calm") {
			return EEMOTION::EMOTION_CALM;
		}
		if (emotion_str == "relaxed") {
			return EEMOTION::EMOTION_RELAXED;
		}
		if (emotion_str == "trust") {
			return EEMOTION::EMOTION_TRUST;
		}
		if (emotion_str == "content") {
			return EEMOTION::EMOTION_CONTENT;
		}
		if (emotion_str == "neutral") {
			return EEMOTION::EMOTION_NEUTRAL;
		}
		return EEMOTION::EMOTION_NEUTRAL;
	}
		
	static std::string emotion_to_string(EEMOTION type) {
		switch (type) {
			case EMOTION_HAPPY: return "happy";
			case EMOTION_SAD: return "sad";
			case EMOTION_AFRAID: return "afraid";
			case EMOTION_SURPRISE: return "surprise";
			case EMOTION_DISGUST: return "disgust";
			case EMOTION_EXCITED: return "excited";
			case EMOTION_JOY: return "joy";
			case EMOTION_DISTRESS: return "distress";
			case EMOTION_DEPRESSED: return "depressed";
			case EMOTION_BORED: return "bored";
			case EMOTION_SLEEPY: return "sleepy";
			case EMOTION_CALM: return "calm";
			case EMOTION_RELAXED: return "relaxed";
			case EMOTION_TRUST: return "trust";
			case EMOTION_CONTENT: return "content";
			case EMOTION_NEUTRAL: return "neutral";
			default:  return "neutral";
		}
	}

}
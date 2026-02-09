#pragma once

#include "c-api.h"

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

}
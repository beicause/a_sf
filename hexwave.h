#pragma once
#include "core/object/ref_counted.h"
#define malloc(s) memalloc(s) //0
#define free(s) memfree(s) //((void) 0)
#define realloc(s, sz) memrealloc(s, sz) //0
#include "stb/stb_hexwave.h"

using namespace godot;

class MHexWave :public RefCounted {
	GDCLASS(MHexWave, RefCounted);
	HexWave hex;

protected:
	static void _bind_methods();

public:
	static void init(int width, int oversample);
	//         width: size of BLEP, from 4..64, larger is slower & more memory but less aliasing
	//    oversample: 2+, number of subsample positions, larger uses more memory but less noise
	//   user_buffer: optional, if provided the library will perform no allocations.
	//                16*width*(oversample+1) bytes, must stay allocated as long as library is used
	//                technically it only needs:   8*( width * (oversample  + 1))
	//                                           + 8*((width *  oversample) + 1)  bytes
	//
	// width can be larger than 64 if you define STB_HEXWAVE_MAX_BLEP_LENGTH to a larger value

	static void shutdown();
	//       user_buffer: pass in same parameter as passed to hexwave_init

	static Ref<MHexWave> create(int reflect, float peak_time, float half_height, float zero_wait);
	// see docs above for description
	//
	//   reflect is tested as 0 or non-zero
	//   peak_time is clamped to 0..1
	//   half_height is not clamped
	//   zero_wait is clamped to 0..1

	void change(int reflect, float peak_time, float half_height, float zero_wait);
	// see docs

	PackedFloat32Array generate_samples(int num_samples, float freq);
	//            output: buffer where the library will store generated floating point audio samples
	// number_of_samples: the number of audio samples to generate
	//               osc: pointer to a Hexwave initialized with 'hexwave_create'
	//   oscillator_freq: frequency of the oscillator divided by the sample rate
};

#undef malloc
#undef free
#undef realloc

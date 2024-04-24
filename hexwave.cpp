#define STB_HEXWAVE_IMPLEMENTATION
#include "hexwave.h"

void MHexWave::init(int width, int oversample) {
	hexwave_init(width, oversample, nullptr);
}
void MHexWave::shutdown() {
	hexwave_shutdown(nullptr);
}
Ref<MHexWave> MHexWave::create(int reflect, float peak_time, float half_height, float zero_wait) {
	Ref<MHexWave> ret;
	ret.instantiate();
	hexwave_create(&ret->hex, reflect, peak_time, half_height, zero_wait);
	return ret;
}

void MHexWave::change(int reflect, float peak_time, float half_height, float zero_wait) {
	hexwave_change(&hex, reflect, peak_time, half_height, zero_wait);
}

PackedFloat32Array MHexWave::generate_samples(int num_samples, float freq) {
	PackedFloat32Array ret;
	ret.resize(num_samples);
	hexwave_generate_samples(ret.ptrw(), num_samples, &hex, freq);
	return ret;
}

void MHexWave::_bind_methods() {
	ClassDB::bind_static_method("MHexWave", D_METHOD("init", "width", "oversample"), &MHexWave::init);
	ClassDB::bind_static_method("MHexWave", D_METHOD("shutdown"), &MHexWave::shutdown);
	ClassDB::bind_static_method("MHexWave", D_METHOD("create", "reflect", "peak_time", "half_height", "zero_wait"), &MHexWave::create);
	ClassDB::bind_method(D_METHOD("change", "reflect", "peak_time", "half_height", "zero_wait"), &MHexWave::change);
	ClassDB::bind_method(D_METHOD("generate_samples", "num_samples", "freq"), &MHexWave::generate_samples);
}

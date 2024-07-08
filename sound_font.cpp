
#include <core/os/memory.h>

static void free_safe(void *ptr) {
	if (ptr != nullptr) {
		memfree(ptr);
	}
}
#define TSF_IMPLEMENTATION
#define TSF_NO_STDIO
#define TSF_MALLOC memalloc
#define TSF_REALLOC memrealloc
#define TSF_FREE free_safe
#include "sound_font.h"

static Ref<SoundFont> new_from_tsf(tsf *f) {
	Ref<SoundFont> sf = memnew(SoundFont);
	sf->set_tsf(f);
	sf->set_output(SoundFont::OUTPUT_MONO, 44100);
	return sf;
}
Ref<SoundFont> SoundFont::create_from_path(const String &p_path) {
	return new_from_tsf(load_from_path_tsf(p_path));
}

Ref<SoundFont> SoundFont::create_from_file(Ref<FileAccess> file) {
	return new_from_tsf(load_from_file_tsf(file));
}

Ref<SoundFont> SoundFont::create_from_memory(const PackedByteArray &buffer) {
	return new_from_tsf(load_from_memory_tsf(buffer));
}

int SoundFont::get_preset_num() {
	return _tsf->presetNum;
}
int SoundFont::get_voice_num() {
	return _tsf->voiceNum;
}
int SoundFont::get_max_voice_num() {
	return _tsf->maxVoiceNum;
}
SoundFont::OutputMode SoundFont::get_output_mode() {
	return (SoundFont::OutputMode)_tsf->outputmode;
}
float SoundFont::get_out_sample_rate() {
	return _tsf->outSampleRate;
}
float SoundFont::get_global_gain_db() {
	return _tsf->globalGainDB;
}
void SoundFont::set_preset_num(int preset_num) {
	_tsf->presetNum = preset_num;
}
void SoundFont::set_voice_num(int voice_num) {
	_tsf->voiceNum = voice_num;
}
void SoundFont::set_max_voice_num(int max_voice_num) {
	_tsf->maxVoiceNum = max_voice_num;
}
void SoundFont::set_output_mode(OutputMode output_mode) {
	_tsf->outputmode = (TSFOutputMode)output_mode;
}
void SoundFont::set_out_sample_rate(float out_sample_rate) {
	_tsf->outSampleRate = out_sample_rate;
}
void SoundFont::set_global_gain_db(float global_gain_db) {
	_tsf->globalGainDB = global_gain_db;
}

// Copy a tsf instance from an existing one, use close to close it as well.
// All copied tsf instances and their original instance are linked, and share the underlying soundfont.
// This allows loading a soundfont only once, but using it for multiple independent playbacks.
// (This function isn't thread-safe without locking.)
Ref<SoundFont> SoundFont::copy() {
	Ref<SoundFont> new_sf = memnew(SoundFont);
	tsf *cp = tsf_copy(_tsf);
	new_sf->set_tsf(cp);
	return new_sf;
}

// Stop all playing notes immediately and reset all channel parameters
void SoundFont::reset() {
	tsf_reset(_tsf);
};

// Returns the preset index from a bank and preset number, or -1 if it does not exist in the loaded SoundFont
int SoundFont::get_preset_index(int bank, int preset_number) {
	return tsf_get_presetindex(_tsf, bank, preset_number);
}

// Returns the number of presets in the loaded SoundFont
int SoundFont::get_preset_count() {
	return tsf_get_presetcount(_tsf);
}

// Returns the name of a preset index >= 0 and < get_presetcount()
String SoundFont::get_preset_name(int preset_index) {
	return String(tsf_get_presetname(_tsf, preset_index));
}

// Returns the name of a preset by bank and preset number
String SoundFont::bank_get_preset_name(int bank, int preset_number) {
	return String(tsf_bank_get_presetname(_tsf, bank, preset_number));
}

// Thread safety:
//
// 1. Rendering / voices:
//
// Your audio output which calls the render* functions will most likely
// run on a different thread than where the playback note* functions
// are called. In which case some sort of concurrency control like a
// mutex needs to be used so they are not called at the same time.
// Alternatively, you can pre-allocate a maximum number of voices that can
// play simultaneously by calling set_max_voices after loading.
// That way memory re-allocation will not happen during note_on and
// TSF should become mostly thread safe.
// There is a theoretical chance that ending notes would negatively influence
// a voice that is rendering at the time but it is hard to say.
// Also be aware, this has not been tested much.
//
// 2. Channels:
//
// Calls to channel_set_... functions may allocate new channels
// if no channel with that number was previously used. Make sure to
// create all channels at the beginning as required if you call render*
// from a different thread.

// Setup the parameters for the voice render methods
//   outputmode: if mono or stereo and how stereo channel data is ordered
//   samplerate: the number of samples per second (output frequency)
//   global_gain_db: volume gain in decibels (>0 means higher, <0 means lower)
void SoundFont::set_output(OutputMode output_mode, int sample_rate, float global_gain_db) {
	tsf_set_output(_tsf, (TSFOutputMode)output_mode, sample_rate, global_gain_db);
}

// Set the global gain as a volume factor
//   global_gain: the desired volume where 1.0 is 100%
void SoundFont::set_volume(float global_gain) {
	tsf_set_volume(_tsf, global_gain);
}

// Set the maximum number of voices to play simultaneously
// Depending on the soundfond, one note can cause many new voices to be started,
// so don't keep this number too low or otherwise sounds may not play.
//   max_voices: maximum number to pre-allocate and set the limit to
//   (set_max_voices returns 0 if allocation failed, otherwise 1)
int SoundFont::set_max_voices(int max_voices) {
	return tsf_set_max_voices(_tsf, max_voices);
}

// Start playing a note
//   preset_index: preset index >= 0 and < get_presetcount()
//   key: note value between 0 and 127 (60 being middle C)
//   vel: velocity as a float between 0.0 (equal to note off) and 1.0 (full)
//   bank: instrument bank number (alternative to preset_index)
//   preset_number: preset number (alternative to preset_index)
//   (note_on returns 0 if the allocation of a new voice failed, otherwise 1)
//   (bank_note_on returns 0 if preset does not exist or allocation failed, otherwise 1)
int SoundFont::note_on(int preset_index, int key, float vel) {
	return tsf_note_on(_tsf, preset_index, key, vel);
}
int SoundFont::bank_note_on(int bank, int preset_number, int key, float vel) {
	return tsf_bank_note_on(_tsf, bank, preset_number, key, vel);
}

// Stop playing a note
//   (bank_note_off returns 0 if preset does not exist, otherwise 1)
void SoundFont::note_off(int preset_index, int key) {
	tsf_note_off(_tsf, preset_index, key);
}
int SoundFont::bank_note_off(int bank, int preset_number, int key) {
	return tsf_bank_note_off(_tsf, bank, preset_number, key);
}

// Stop playing all notes (end with sustain and release)
void SoundFont::note_off_all() {
	tsf_note_off_all(_tsf);
}

// Returns the number of active voices
int SoundFont::active_voice_count() {
	return tsf_active_voice_count(_tsf);
}

// Render output samples into a buffer
// You can either render as signed 16-bit values (render_short) or
// as 32-bit float values (render_float)
//   buffer: target buffer of size samples * output_channels * sizeof(type)
//   samples: number of samples to render
//   flag_mixing: if 0 clear the buffer first, otherwise mix into existing data
PackedByteArray SoundFont::render_short(int samples) {
	int flag_mixing = 0;
	int size = samples * (_tsf->outputmode == TSFOutputMode::TSF_MONO ? 1 : 2);
	Vector<short> buffer;
	buffer.resize(size);
	tsf_render_short(_tsf, buffer.ptrw(), samples, flag_mixing);
	// 16bit short buffer to byte array
	PackedByteArray array = PackedByteArray();
	array.resize(size * 2);
	for (int i = 0; i < size; i += 2) {
		array.set(i, buffer[i] & 0xff);
		array.set(i + 1, buffer[i] >> 8);
	}
	return array;
}
PackedFloat32Array SoundFont::render_float(int samples) {
	int flag_mixing = 0;
	int size = samples * (_tsf->outputmode == TSFOutputMode::TSF_MONO ? 1 : 2);
	PackedFloat32Array array = PackedFloat32Array();
	array.resize(size);
	tsf_render_float(_tsf, array.ptrw(), samples, flag_mixing);
	return array;
}

// Higher level channel based functions, set up channel parameters
//   channel: channel number
//   preset_index: preset index >= 0 and < get_presetcount()
//   preset_number: preset number (alternative to preset_index)
//   flag_mididrums: 0 for normal channels, otherwise apply MIDI drum channel rules
//   bank: instrument bank number (alternative to preset_index)
//   pan: stereo panning value from 0.0 (left) to 1.0 (right) (default 0.5 center)
//   volume: linear volume scale factor (default 1.0 full)
//   pitch_wheel: pitch wheel position 0 to 16383 (default 8192 unpitched)
//   pitch_range: range of the pitch wheel in semitones (default 2.0, total +/- 2 semitones)
//   tuning: tuning of all playing voices in semitones (default 0.0, standard (A440) tuning)
//   (set_preset_number and set_bank_preset return 0 if preset does not exist, otherwise 1)
//   (channel_set_... return 0 if a new channel needed allocation and that failed, otherwise 1)
int SoundFont::channel_set_preset_index(int channel, int preset_index) {
	return tsf_channel_set_presetindex(_tsf, channel, preset_index);
}
int SoundFont::channel_set_preset_number(int channel, int preset_number, int flag_midi_drums) {
	return tsf_channel_set_presetnumber(_tsf, channel, preset_number, flag_midi_drums);
}
int SoundFont::channel_set_bank(int channel, int bank) {
	return tsf_channel_set_bank(_tsf, channel, bank);
}
int SoundFont::channel_set_bank_preset(int channel, int bank, int preset_number) {
	return tsf_channel_set_bank_preset(_tsf, channel, bank, preset_number);
}
int SoundFont::channel_set_pan(int channel, float pan) {
	return tsf_channel_set_pan(_tsf, channel, pan);
}
int SoundFont::channel_set_volume(int channel, float volume) {
	return tsf_channel_set_volume(_tsf, channel, volume);
}
int SoundFont::channel_set_pitch_wheel(int channel, int pitch_wheel) {
	return tsf_channel_set_pitchwheel(_tsf, channel, pitch_wheel);
}
int SoundFont::channel_set_pitch_range(int channel, float pitch_range) {
	return tsf_channel_set_pitchrange(_tsf, channel, pitch_range);
}
int SoundFont::channel_set_tuning(int channel, float tuning) {
	return tsf_channel_set_tuning(_tsf, channel, tuning);
}

// Start or stop playing notes on a channel (needs channel preset to be set)
//   channel: channel number
//   key: note value between 0 and 127 (60 being middle C)
//   vel: velocity as a float between 0.0 (equal to note off) and 1.0 (full)
//   (channel_note_on returns 0 on allocation failure of new voice, otherwise 1)
int SoundFont::channel_note_on(int channel, int key, float vel) {
	return tsf_channel_note_on(_tsf, channel, key, vel);
}
void SoundFont::channel_note_off(int channel, int key) {
	tsf_channel_note_off(_tsf, channel, key);
}
void SoundFont::channel_note_off_all(int channel) {
	tsf_channel_note_off_all(_tsf, channel);
} //end with sustain and release
void SoundFont::channel_sounds_off_all(int channel) {
	tsf_channel_sounds_off_all(_tsf, channel);
} //end immediately

// Apply a MIDI control change to the channel (not all controllers are supported!)
//    (channel_midi_control returns 0 on allocation failure of new channel, otherwise 1)
int SoundFont::channel_midi_control(int channel, int controller, int control_value) {
	return tsf_channel_midi_control(_tsf, channel, controller, control_value);
}

// Get current values set on the channels
int SoundFont::channel_get_preset_index(int channel) {
	return tsf_channel_get_preset_index(_tsf, channel);
}
int SoundFont::channel_get_preset_bank(int channel) {
	return tsf_channel_get_preset_bank(_tsf, channel);
}
int SoundFont::channel_get_preset_number(int channel) {
	return tsf_channel_get_preset_number(_tsf, channel);
}
float SoundFont::channel_get_pan(int channel) {
	return tsf_channel_get_pan(_tsf, channel);
}
float SoundFont::channel_get_volume(int channel) {
	return tsf_channel_get_volume(_tsf, channel);
}
int SoundFont::channel_get_pitch_wheel(int channel) {
	return tsf_channel_get_pitchwheel(_tsf, channel);
}
float SoundFont::channel_get_pitch_range(int channel) {
	return tsf_channel_get_pitchrange(_tsf, channel);
}
float SoundFont::channel_get_tuning(int channel) {
	return tsf_channel_get_tuning(_tsf, channel);
}
SoundFont::~SoundFont() {
	tsf_close(_tsf);
}
void SoundFont::_bind_methods() {
	BIND_ENUM_CONSTANT(OUTPUT_STEREO_INTERLEAVED);
	BIND_ENUM_CONSTANT(OUTPUT_STEREO_UNWEAVED);
	BIND_ENUM_CONSTANT(OUTPUT_MONO);

	ClassDB::bind_static_method("SoundFont", D_METHOD("load_path", "path"), &SoundFont::create_from_path);
	ClassDB::bind_static_method("SoundFont", D_METHOD("load_file", "file"), &SoundFont::create_from_file);
	ClassDB::bind_static_method("SoundFont", D_METHOD("load_memory", "buffer"), &SoundFont::create_from_memory);

	ClassDB::bind_method(D_METHOD("get_preset_num"), &SoundFont::get_preset_num);
	ClassDB::bind_method(D_METHOD("get_voice_num"), &SoundFont::get_voice_num);
	ClassDB::bind_method(D_METHOD("get_max_voice_num"), &SoundFont::get_max_voice_num);
	ClassDB::bind_method(D_METHOD("get_output_mode"), &SoundFont::get_output_mode);
	ClassDB::bind_method(D_METHOD("get_out_sample_rate"), &SoundFont::get_out_sample_rate);
	ClassDB::bind_method(D_METHOD("get_global_gain_db"), &SoundFont::get_global_gain_db);
	ClassDB::bind_method(D_METHOD("set_preset_num", "preset_num"), &SoundFont::set_preset_num);
	ClassDB::bind_method(D_METHOD("set_voice_num", "voice_num"), &SoundFont::set_voice_num);
	ClassDB::bind_method(D_METHOD("set_max_voice_num", "max_voice_num"), &SoundFont::set_max_voice_num);
	ClassDB::bind_method(D_METHOD("set_output_mode", "outputmode"), &SoundFont::set_output_mode);
	ClassDB::bind_method(D_METHOD("set_out_sample_rate", "out_sample_rate"), &SoundFont::set_out_sample_rate);
	ClassDB::bind_method(D_METHOD("set_global_gain_db", "global_gain_db"), &SoundFont::set_global_gain_db);

	ClassDB::bind_method(D_METHOD("copy"), &SoundFont::copy);
	ClassDB::bind_method(D_METHOD("reset"), &SoundFont::reset);
	ClassDB::bind_method(D_METHOD("get_preset_index", "bank", "preset_number"), &SoundFont::get_preset_index);
	ClassDB::bind_method(D_METHOD("get_preset_count"), &SoundFont::get_preset_count);
	ClassDB::bind_method(D_METHOD("get_preset_name", "preset_index"), &SoundFont::get_preset_name);
	ClassDB::bind_method(D_METHOD("bank_get_preset_name", "bank", "preset_number"), &SoundFont::bank_get_preset_name);
	ClassDB::bind_method(D_METHOD("set_output", "outputmode", "sample_rate", "global_gain_db"), &SoundFont::set_output, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("set_volume", "global_gain"), &SoundFont::set_volume);
	ClassDB::bind_method(D_METHOD("set_max_voices", "max_voices"), &SoundFont::set_max_voices);
	ClassDB::bind_method(D_METHOD("note_on", "preset_index", "key", "vel"), &SoundFont::note_on);
	ClassDB::bind_method(D_METHOD("bank_note_on", "bank", "preset_number", "key", "vel"), &SoundFont::bank_note_on);
	ClassDB::bind_method(D_METHOD("note_off", "preset_index", "key"), &SoundFont::note_off);
	ClassDB::bind_method(D_METHOD("bank_note_off", "bank", "preset_number", "key"), &SoundFont::bank_note_off);
	ClassDB::bind_method(D_METHOD("note_off_all"), &SoundFont::note_off_all);
	ClassDB::bind_method(D_METHOD("active_voice_count"), &SoundFont::active_voice_count);
	ClassDB::bind_method(D_METHOD("render_short", "samples"), &SoundFont::render_short);
	ClassDB::bind_method(D_METHOD("render_float", "samples"), &SoundFont::render_float);
	ClassDB::bind_method(D_METHOD("channel_set_preset_index", "channel", "preset_index"), &SoundFont::channel_set_preset_index);
	ClassDB::bind_method(D_METHOD("channel_set_preset_number", "channel", "preset_number", "flag_mididrums"), &SoundFont::channel_set_preset_number, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("channel_set_bank", "channel", "bank"), &SoundFont::channel_set_bank);
	ClassDB::bind_method(D_METHOD("channel_set_bank_preset", "channel", "bank", "preset_number"), &SoundFont::channel_set_bank_preset);
	ClassDB::bind_method(D_METHOD("channel_set_pan", "channel", "pan"), &SoundFont::channel_set_pan);
	ClassDB::bind_method(D_METHOD("channel_set_volume", "channel", "volume"), &SoundFont::channel_set_volume);
	ClassDB::bind_method(D_METHOD("channel_set_pitch_wheel", "channel", "pitch_wheel"), &SoundFont::channel_set_pitch_wheel);
	ClassDB::bind_method(D_METHOD("channel_set_pitch_range", "channel", "pitch_range"), &SoundFont::channel_set_pitch_range);
	ClassDB::bind_method(D_METHOD("channel_set_tuning", "channel", "tuning"), &SoundFont::channel_set_tuning);
	ClassDB::bind_method(D_METHOD("channel_note_on", "channel", "key", "vel"), &SoundFont::channel_note_on);
	ClassDB::bind_method(D_METHOD("channel_note_off", "channel", "key"), &SoundFont::channel_note_off);
	ClassDB::bind_method(D_METHOD("channel_note_off_all", "channel"), &SoundFont::channel_note_off_all);
	ClassDB::bind_method(D_METHOD("channel_sounds_off_all", "channel"), &SoundFont::channel_sounds_off_all);
	ClassDB::bind_method(D_METHOD("channel_midi_control", "channel", "control", "value"), &SoundFont::channel_midi_control);
	ClassDB::bind_method(D_METHOD("channel_get_preset_index", "channel"), &SoundFont::channel_get_preset_index);
	ClassDB::bind_method(D_METHOD("channel_get_preset_bank", "channel"), &SoundFont::channel_get_preset_bank);
	ClassDB::bind_method(D_METHOD("channel_get_preset_number", "channel"), &SoundFont::channel_get_preset_number);
	ClassDB::bind_method(D_METHOD("channel_get_pan", "channel"), &SoundFont::channel_get_pan);
	ClassDB::bind_method(D_METHOD("channel_get_volume", "channel"), &SoundFont::channel_get_volume);
	ClassDB::bind_method(D_METHOD("channel_get_pitch_wheel", "channel"), &SoundFont::channel_get_pitch_wheel);
	ClassDB::bind_method(D_METHOD("channel_get_pitch_range", "channel"), &SoundFont::channel_get_pitch_range);
	ClassDB::bind_method(D_METHOD("channel_get_tuning", "channel"), &SoundFont::channel_get_tuning);
}

Ref<Resource> ResourceFormatLoaderSoundFont::load(const String &p_path, const String &p_original_path, Error *r_error, bool p_use_sub_threads, float *r_progress, CacheMode p_cache_mode) {
	if (r_error) {
		*r_error = ERR_FILE_CANT_OPEN;
	}

	if (!FileAccess::exists(p_path)) {
		*r_error = ERR_FILE_NOT_FOUND;
		return Ref<Resource>();
	}

	Ref<SoundFont> sf = SoundFont::create_from_path(p_path);
	if (r_error) {
		*r_error = OK;
	}
	return sf;
}

void ResourceFormatLoaderSoundFont::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back("sf2");
	p_extensions->push_back("sf3");
}

bool ResourceFormatLoaderSoundFont::handles_type(const String &p_type) const {
	return (p_type == "SoundFont");
}

String ResourceFormatLoaderSoundFont::get_resource_type(const String &p_path) const {
	String el = p_path.get_extension().to_lower();
	if (el == "sf2" || el == "sf3") {
		return "SoundFont";
	}
	return "";
}

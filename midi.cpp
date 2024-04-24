#include <core/os/memory.h>

static void free_safe(void *ptr) {
	if (ptr != nullptr) {
		memfree(ptr);
	}
}
#define TML_IMPLEMENTATION
#define TML_NO_STDIO
#define TML_MALLOC memalloc
#define TML_REALLOC memrealloc
#define TML_FREE free_safe
#include "midi.h"

static Ref<Midi> new_from_tml(tml_message *tml) {
	Ref<Midi> mid = memnew(Midi);
	mid->_set_tml_raw(tml);
	mid->tml_header = true;
	return mid;
}
Ref<Midi> Midi::load_path(const String &p_path) {
	return new_from_tml(load_from_path_tml(p_path));
}

Ref<Midi> Midi::load_file(Ref<FileAccess> file) {
	return new_from_tml(load_from_file_tml(file));
}

Ref<Midi> Midi::load_memory(const PackedByteArray &buffer) {
	return new_from_tml(load_from_memory_tml(buffer));
}

Ref<Midi> Midi::load_dicts(const Array &dicts) {
	if (dicts.size() == 0)
		return nullptr;
	tml_message *tml = (tml_message *)memalloc(dicts.size() * sizeof(tml_message));
	for (int i = 0; i < dicts.size(); i++) {
		tml_message *curr = &tml[i];
		Dictionary dict = dicts[i];
		if (dict.has("channel"))
			curr->channel = dict["channel"];
		if (dict.has("time"))
			curr->time = dict["time"];
		if (dict.has("type"))
			curr->type = dict["type"];
		if (dict.has("channel_pressure"))
			curr->channel_pressure = (int)dict["channel_pressure"];
		if (dict.has("key_pressure"))
			curr->key_pressure = (int)dict["key_pressure"];
		if (dict.has("control"))
			curr->control = (int)dict["control"];
		if (dict.has("control_value"))
			curr->control_value = (int)dict["control_value"];
		if (dict.has("key"))
			curr->key = (int)dict["key"];
		if (dict.has("program"))
			curr->program = (int)dict["program"];
		if (dict.has("velocity"))
			curr->velocity = (int)dict["velocity"];
		else if (curr->type == TML_NOTE_ON)
			curr->velocity = 100;
		if (dict.has("pitch_bend"))
			curr->pitch_bend = dict["pitch_bend"];
		curr->next = i + 1 < dicts.size() ? &tml[i + 1] : nullptr;
	}
	return new_from_tml(tml);
}

Ref<Midi> Midi::load_simple_array(const PackedByteArray &arr, int duration_ms, int channel, int vel) {
	if (arr.size() == 0)
		return nullptr;
	tml_message *tml = (tml_message *)memalloc(arr.size() * 2 * sizeof(tml_message));
	for (int i = 0; i < arr.size(); i++) {
		tml_message *curr = &tml[i * 2];
		tml_message *next = &tml[i * 2 + 1];
		uint8_t key = arr[i];
		curr->key = key;
		curr->type = NOTE_ON;
		curr->velocity = vel;
		curr->channel = channel;
		curr->time = duration_ms * i;
		curr->next = next;
		next->key = key;
		next->type = NOTE_OFF;
		next->channel = channel;
		next->time = curr->time + duration_ms;
		next->next = i + 1 < arr.size() ? &tml[i * 2 + 2] : nullptr;
	}
	return new_from_tml(tml);
}

Ref<Midi> Midi::load_simple_time_array(const PackedByteArray &notes, const PackedInt32Array &times, int duration_ms, int channel, int vel) {
	if (notes.size() == 0)
		return nullptr;
	tml_message *tml = (tml_message *)memalloc(notes.size() * 2 * sizeof(tml_message));
	for (int i = 0; i < notes.size(); i++) {
		tml_message *curr = &tml[i * 2];
		tml_message *next = &tml[i * 2 + 1];
		uint8_t key = notes[i];
		curr->key = key;
		curr->type = NOTE_ON;
		curr->velocity = vel;
		curr->channel = channel;
		curr->time = times[i];
		curr->next = next;
		next->key = key;
		next->type = NOTE_OFF;
		next->channel = channel;
		next->time = curr->time + duration_ms;
		next->next = i + 1 < notes.size() ? &tml[i * 2 + 2] : nullptr;
	}
	return new_from_tml(tml);
}

Array Midi::to_dicts() {
	Array res;
	for (tml_message *curr = _tml; curr != nullptr; curr = curr->next) {
		Dictionary dict;
		dict["channel"] = curr->channel;
		dict["time"] = curr->time;
		dict["type"] = curr->type;
		dict["channel_pressure"] = curr->channel_pressure;
		dict["key_pressure"] = curr->key_pressure;
		dict["control"] = curr->control;
		dict["control_value"] = curr->control_value;
		dict["key"] = curr->key;
		dict["program"] = curr->program;
		dict["velocity"] = curr->velocity;
		dict["pitch_bend"] = curr->pitch_bend;
		res.append(dict);
	}
	return res;
}

Array Midi::to_simple_array(int channel) {
	Array res;
	PackedByteArray notes;
	PackedInt32Array times;
	for (tml_message *curr = _tml; curr != nullptr; curr = curr->next)
		if (curr->type == NOTE_ON && (channel < 0 || curr->channel == channel)) {
			notes.append(curr->key);
			times.append(curr->time);
		}
	res.append(notes);
	res.append(times);
	return res;
}
// Get infos about this loaded MIDI file, returns the note count
// NULL can be passed for any output value pointer if not needed.
//   used_channels:   Will be set to how many channels play notes
//                    (i.e. 1 if channel 15 is used but no other)
//   used_programs:   Will be set to how many different programs are used
//   total_notes:     Will be set to the total number of note on messages
//   time_first_note: Will be set to the time of the first note on message
//   time_length:     Will be set to the total time in milliseconds
Dictionary Midi::get_info() {
	int used_channels = -1;
	int used_programs = -1;
	int total_notes = -1;
	unsigned int time_first_note = -1;
	unsigned int time_length = -1;
	int note_count = tml_get_info(_tml, &used_channels, &used_programs, &total_notes, &time_first_note, &time_length);
	Dictionary res = Dictionary();
	res["used_channels"] = used_channels;
	res["used_programs"] = used_programs;
	res["total_notes"] = total_notes;
	res["time_first_note"] = time_first_note;
	res["time_length"] = time_length;
	res["note_count"] = note_count;
	return res;
}

// Read the tempo (microseconds per quarter note) value from a message with the type TML_SET_TEMPO
int Midi::get_tempo_value() {
	return tml_get_tempo_value(_tml);
}

Dictionary Midi::read_msg() {
	int time = _tml->time;
	int type = _tml->type;
	int channel = _tml->channel;
	char channel_pressure = _tml->channel_pressure;
	char key_pressure = _tml->key_pressure;
	char control = _tml->control;
	char control_value = _tml->control_value;
	char key = _tml->key;
	char program = _tml->program;
	char velocity = _tml->velocity;
	unsigned short pitch_bend = _tml->pitch_bend;
	Dictionary res = Dictionary();
	res["time"] = time;
	res["type"] = type;
	res["channel"] = channel;
	res["channel_pressure"] = channel_pressure;
	res["key_pressure"] = key_pressure;
	res["control"] = control;
	res["control_value"] = control_value;
	res["key"] = key;
	res["program"] = program;
	res["velocity"] = velocity;
	res["pitch_bend"] = pitch_bend;
	return res;
}

Ref<Midi> Midi::next() {
	if (!_tml->next) {
		return nullptr;
	}
	Ref<Midi> n = memnew(Midi);
	n->_set_tml_raw(_tml->next);
	return n;
}

tml_message *Midi::render_current_raw(tml_message *t, Ref<SoundFont> sf, PackedFloat32Array *buffer) {
	if (t == nullptr) {
		return nullptr;
	}
	for (; t != nullptr; t = t->next) {
		switch (t->type) {
			case TML_PROGRAM_CHANGE: //channel program (preset) change (special handling for 10th MIDI channel with drums)
				sf->channel_set_preset_number(t->channel, t->program, (t->channel == 9));
				break;
			case TML_NOTE_ON: //play a note
				sf->channel_note_on(t->channel, t->key, t->velocity / 127.0f);
				break;
			case TML_NOTE_OFF: //stop a note
				sf->channel_note_off(t->channel, t->key);
				break;
			case TML_PITCH_BEND: //pitch wheel modification
				sf->channel_set_pitch_wheel(t->channel, t->pitch_bend);
				break;
			case TML_CONTROL_CHANGE: //MIDI controller messages
				sf->channel_midi_control(t->channel, t->control, t->control_value);
				break;
		}
		if (t->next == nullptr) {
			break;
		}
		int block_size = (t->next->time - t->time) * sf->get_out_sample_rate() / 1000;
		// UtilityFunctions::print("next time: ", t->next->time, " time: ", t->time, " block_size: ", block_size);
		if (block_size <= 0) {
			continue;
		}
		int prev_size = buffer->size();
		buffer->resize(buffer->size() + block_size);
		sf->render_float_raw(sf->get_tsf(), buffer->ptrw() + prev_size, block_size, 0);
		break;
	}
	return t;
}

Dictionary Midi::render_current(Ref<SoundFont> sf) {
	Dictionary res = Dictionary();
	PackedFloat32Array buffer = PackedFloat32Array();
	tml_message *t = render_current_raw(_tml, sf, &buffer);
	res["buffer"] = buffer;
	Ref<Midi> current = memnew(Midi);
	// Ref<Midi> next = memnew(Midi);
	current->_set_tml_raw(t);
	// next->_set_tml_raw(t->next);
	res["current"] = current;
	// res["next"] = next;
	return res;
}
PackedFloat32Array Midi::render_all(Ref<SoundFont> sf) {
	PackedFloat32Array res = PackedFloat32Array();
	for (tml_message *t = _tml; t != nullptr; t = t->next) {
		switch (t->type) {
			case TML_PROGRAM_CHANGE: //channel program (preset) change (special handling for 10th MIDI channel with drums)
				sf->channel_set_preset_number(t->channel, t->program, (t->channel == 9));
				break;
			case TML_NOTE_ON: //play a note
				sf->channel_note_on(t->channel, t->key, t->velocity / 127.0f);
				break;
			case TML_NOTE_OFF: //stop a note
				sf->channel_note_off(t->channel, t->key);
				break;
			case TML_PITCH_BEND: //pitch wheel modification
				sf->channel_set_pitch_wheel(t->channel, t->pitch_bend);
				break;
			case TML_CONTROL_CHANGE: //MIDI controller messages
				sf->channel_midi_control(t->channel, t->control, t->control_value);
				break;
		}
		if (t->next == nullptr)
			break;
		int block_size = (t->next->time - t->time) * sf->get_out_sample_rate() / 1000;
		if (block_size <= 0)
			continue;
		int prev_size = res.size();
		res.resize(res.size() + block_size);
		sf->render_float_raw(sf->get_tsf(), res.ptrw() + prev_size, block_size, 0);
	}
	return res;
}

Midi::~Midi() {
	if (is_tml_header())
		tml_free(_tml);
};

void Midi::_bind_methods() {
	BIND_ENUM_CONSTANT(NOTE_OFF);
	BIND_ENUM_CONSTANT(NOTE_ON);
	BIND_ENUM_CONSTANT(KEY_PRESSURE);
	BIND_ENUM_CONSTANT(CONTROL_CHANGE);
	BIND_ENUM_CONSTANT(PROGRAM_CHANGE);
	BIND_ENUM_CONSTANT(CHANNEL_PRESSURE);
	BIND_ENUM_CONSTANT(PITCH_BEND);
	BIND_ENUM_CONSTANT(SET_TEMPO);

	BIND_ENUM_CONSTANT(BANK_SELECT_MSB);
	BIND_ENUM_CONSTANT(MODULATIONWHEEL_MSB);
	BIND_ENUM_CONSTANT(BREATH_MSB);
	BIND_ENUM_CONSTANT(FOOT_MSB);
	BIND_ENUM_CONSTANT(PORTAMENTO_TIME_MSB);
	BIND_ENUM_CONSTANT(DATA_ENTRY_MSB);
	BIND_ENUM_CONSTANT(VOLUME_MSB);
	BIND_ENUM_CONSTANT(BALANCE_MSB);
	BIND_ENUM_CONSTANT(PAN_MSB);
	BIND_ENUM_CONSTANT(EXPRESSION_MSB);
	BIND_ENUM_CONSTANT(EFFECTS1_MSB);
	BIND_ENUM_CONSTANT(EFFECTS2_MSB);
	BIND_ENUM_CONSTANT(GPC1_MSB);
	BIND_ENUM_CONSTANT(GPC2_MSB);
	BIND_ENUM_CONSTANT(GPC3_MSB);
	BIND_ENUM_CONSTANT(GPC4_MSB);
	BIND_ENUM_CONSTANT(BANK_SELECT_LSB);
	BIND_ENUM_CONSTANT(MODULATIONWHEEL_LSB);
	BIND_ENUM_CONSTANT(BREATH_LSB);
	BIND_ENUM_CONSTANT(FOOT_LSB);
	BIND_ENUM_CONSTANT(PORTAMENTO_TIME_LSB);
	BIND_ENUM_CONSTANT(DATA_ENTRY_LSB);
	BIND_ENUM_CONSTANT(VOLUME_LSB);
	BIND_ENUM_CONSTANT(BALANCE_LSB);
	BIND_ENUM_CONSTANT(PAN_LSB);
	BIND_ENUM_CONSTANT(EXPRESSION_LSB);
	BIND_ENUM_CONSTANT(EFFECTS1_LSB);
	BIND_ENUM_CONSTANT(EFFECTS2_LSB);
	BIND_ENUM_CONSTANT(GPC1_LSB);
	BIND_ENUM_CONSTANT(GPC2_LSB);
	BIND_ENUM_CONSTANT(GPC3_LSB);
	BIND_ENUM_CONSTANT(GPC4_LSB);
	BIND_ENUM_CONSTANT(SUSTAIN_SWITCH);
	BIND_ENUM_CONSTANT(PORTAMENTO_SWITCH);
	BIND_ENUM_CONSTANT(SOSTENUTO_SWITCH);
	BIND_ENUM_CONSTANT(SOFT_PEDAL_SWITCH);
	BIND_ENUM_CONSTANT(LEGATO_SWITCH);
	BIND_ENUM_CONSTANT(HOLD2_SWITCH);
	BIND_ENUM_CONSTANT(SOUND_CTRL1);
	BIND_ENUM_CONSTANT(SOUND_CTRL2);
	BIND_ENUM_CONSTANT(SOUND_CTRL3);
	BIND_ENUM_CONSTANT(SOUND_CTRL4);
	BIND_ENUM_CONSTANT(SOUND_CTRL5);
	BIND_ENUM_CONSTANT(SOUND_CTRL6);
	BIND_ENUM_CONSTANT(SOUND_CTRL7);
	BIND_ENUM_CONSTANT(SOUND_CTRL8);
	BIND_ENUM_CONSTANT(SOUND_CTRL9);
	BIND_ENUM_CONSTANT(SOUND_CTRL10);
	BIND_ENUM_CONSTANT(GPC5);
	BIND_ENUM_CONSTANT(GPC6);
	BIND_ENUM_CONSTANT(GPC7);
	BIND_ENUM_CONSTANT(GPC8);
	BIND_ENUM_CONSTANT(PORTAMENTO_CTRL);
	BIND_ENUM_CONSTANT(FX_REVERB);
	BIND_ENUM_CONSTANT(FX_TREMOLO);
	BIND_ENUM_CONSTANT(FX_CHORUS);
	BIND_ENUM_CONSTANT(FX_CELESTE_DETUNE);
	BIND_ENUM_CONSTANT(FX_PHASER);
	BIND_ENUM_CONSTANT(DATA_ENTRY_INCR);
	BIND_ENUM_CONSTANT(DATA_ENTRY_DECR);
	BIND_ENUM_CONSTANT(NRPN_LSB);
	BIND_ENUM_CONSTANT(NRPN_MSB);
	BIND_ENUM_CONSTANT(RPN_LSB);
	BIND_ENUM_CONSTANT(RPN_MSB);
	BIND_ENUM_CONSTANT(ALL_SOUND_OFF);
	BIND_ENUM_CONSTANT(ALL_CTRL_OFF);
	BIND_ENUM_CONSTANT(LOCAL_CONTROL);
	BIND_ENUM_CONSTANT(ALL_NOTES_OFF);
	BIND_ENUM_CONSTANT(OMNI_OFF);
	BIND_ENUM_CONSTANT(OMNI_ON);
	BIND_ENUM_CONSTANT(POLY_OFF);
	BIND_ENUM_CONSTANT(POLY_ON);

	ClassDB::bind_static_method("Midi", D_METHOD("load_path", "path"), &Midi::load_path);
	ClassDB::bind_static_method("Midi", D_METHOD("load_file", "file"), &Midi::load_file);
	ClassDB::bind_static_method("Midi", D_METHOD("load_memory", "buffer"), &Midi::load_memory);
	ClassDB::bind_static_method("Midi", D_METHOD("load_dicts", "dicts"), &Midi::load_dicts);
	ClassDB::bind_static_method("Midi", D_METHOD("load_simple_array", "arr", "duration_ms", "channel", "vel"), &Midi::load_simple_array, DEFVAL(600), DEFVAL(0), DEFVAL(100));
	ClassDB::bind_static_method("Midi", D_METHOD("load_simple_time_array", "notes", "times", "duration_ms", "channel", "vel"), &Midi::load_simple_time_array, DEFVAL(600), DEFVAL(0), DEFVAL(100));

	ClassDB::bind_method(D_METHOD("to_dicts"), &Midi::to_dicts);
	ClassDB::bind_method(D_METHOD("to_simple_array", "selected_channel"), &Midi::to_simple_array, DEFVAL(-1));

	ClassDB::bind_method(D_METHOD("get_info"), &Midi::get_info);
	ClassDB::bind_method(D_METHOD("get_tempo_value"), &Midi::get_tempo_value);
	ClassDB::bind_method(D_METHOD("read_msg"), &Midi::read_msg);
	ClassDB::bind_method(D_METHOD("next"), &Midi::next);

	ClassDB::bind_method(D_METHOD("render_all", "sf"), &Midi::render_all);
	ClassDB::bind_method(D_METHOD("render_current", "sf"), &Midi::render_current);
	ClassDB::bind_method(D_METHOD("is_tml_header"), &Midi::is_tml_header);
}

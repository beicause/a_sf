#include "register_types.h"
#include "midi.h"
#include "midi_buffer.h"
#include "sound_font.h"

void initialize_a_my_sf_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	ClassDB::register_class<SoundFont>();
	ClassDB::register_class<Midi>();
	ClassDB::register_class<MidiBuffer>();
}

void uninitialize_a_my_sf_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}

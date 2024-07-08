#include "register_types.h"
#include "midi.h"
#include "midi_buffer.h"
#include "sound_font.h"

Ref<ResourceFormatLoaderSoundFont> resource_loader_sf;
Ref<ResourceFormatLoaderMidi> resource_loader_mid;

void initialize_a_sf_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	ClassDB::register_class<SoundFont>();
	ClassDB::register_class<Midi>();
	ClassDB::register_class<MidiBuffer>();

	resource_loader_sf.instantiate();
	resource_loader_mid.instantiate();

	ResourceLoader::add_resource_format_loader(resource_loader_sf);
	ResourceLoader::add_resource_format_loader(resource_loader_mid);
}

void uninitialize_a_sf_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	ResourceLoader::remove_resource_format_loader(resource_loader_sf);
	ResourceLoader::remove_resource_format_loader(resource_loader_mid);

	resource_loader_sf.unref();
	resource_loader_mid.unref();
}

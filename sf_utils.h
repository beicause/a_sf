#pragma once

// for .sf3
#include "ogg/stb_vorbis.h"
#include "tml.h"
#include "tsf.h"

#include <core/io/file_access.h>
#include <core/os/memory.h>

using namespace godot;

int read_file_access(Ref<FileAccess> file, void *ptr, unsigned int size);
int skip_file_access(Ref<FileAccess> file, unsigned int count);

inline tsf *load_from_memory_tsf(PackedByteArray buffer) {
	return tsf_load_memory(buffer.ptr(), buffer.size());
}

inline tsf *load_from_file_tsf(Ref<FileAccess> file) {
	tsf_stream stream = { &file, (int (*)(void *, void *, unsigned int)) & read_file_access, (int (*)(void *, unsigned int)) & skip_file_access };
	return tsf_load(&stream);
}

inline tsf *load_from_path_tsf(const String &p_path) {
	Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::ModeFlags::READ);
	return load_from_file_tsf(file);
}

inline tml_message *load_from_memory_tml(PackedByteArray buffer) {
	return tml_load_memory(buffer.ptr(), buffer.size());
}

inline tml_message *load_from_file_tml(Ref<FileAccess> file) {
	tml_stream stream = { &file, (int (*)(void *, void *, unsigned int)) & read_file_access };
	return tml_load(&stream);
}

inline tml_message *load_from_path_tml(const String &p_path) {
	Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::ModeFlags::READ);
	return load_from_file_tml(file);
}

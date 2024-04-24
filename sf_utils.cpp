#include "sf_utils.h"

int read_file_access(Ref<FileAccess> file, void *ptr, unsigned int size) {
	PackedByteArray buf = file->get_buffer(size);
	memcpy(ptr, buf.ptr(), buf.size());
	return buf.size();
}
int skip_file_access(Ref<FileAccess> file, unsigned int count) {
	if (count > file->get_length() - file->get_position()) {
		return 0;
	}
	file->seek(file->get_position() + count);
	return 1;
}

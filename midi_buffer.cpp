#include "midi_buffer.h"

Ref<MidiBuffer> MidiBuffer::new_with_args(Ref<SoundFont> sf, Ref<Midi> midi) {
	Ref<MidiBuffer> buf = memnew(MidiBuffer);
	if (midi.is_valid()) {
		buf->set_midi(midi);
	}
	if (sf.is_valid()) {
		buf->set_sf(sf);
	}
	return buf;
}

void MidiBuffer::set_midi(Ref<Midi> midi) {
	this->midi = midi;
	_tml = midi->_get_tml_raw();
	ring_buffer.clear();
}
void MidiBuffer::set_sf(Ref<SoundFont> sf) {
	this->sf = sf;
}
void MidiBuffer::push_buffer(int length) {
	if (!midi.is_valid() || !sf.is_valid()) {
		return;
	}
	PackedFloat32Array buffer = PackedFloat32Array();
	while (buffer.size() < length) {
		_tml = midi->render_current_raw(_tml, sf, &buffer);
		if (_tml == nullptr) {
			break;
		}
		_tml = _tml->next;
	}
	if (ring_buffer.space_left() + 1 < buffer.size()) {
		spin_lock.lock();
		ring_buffer.resize(msb(ring_buffer.data_left() + buffer.size()));
		spin_lock.unlock();
	}
	ring_buffer.write(buffer.ptr(), buffer.size());
}

PackedFloat32Array MidiBuffer::get_buffer(int length) {
	PackedFloat32Array res;
	res.resize(length);
	spin_lock.lock();
	size_t num = ring_buffer.read(res.ptrw(), length);
	spin_lock.unlock();
	res.resize(num);
	return res;
}

bool MidiBuffer::fill_audio_buffer(Ref<AudioStreamGeneratorPlayback> playback, int length) {
	if (length == -1) {
		length = playback->get_frames_available();
	}
	if ((running_id == -1 || WorkerThreadPool::get_singleton()->is_task_completed(running_id)) && ring_buffer.data_left() < rb_target_size) {
		running_id = WorkerThreadPool::get_singleton()->add_template_task(this, &MidiBuffer::push_buffer, rb_target_size);
	}
	PackedFloat32Array buffer = get_buffer(length);
	PackedVector2Array b = PackedVector2Array();
	b.resize(buffer.size());
	for (int i = 0; i < buffer.size(); i++) {
		b.set(i, Vector2(buffer[i], buffer[i]));
	}
	playback->push_buffer(b);
	return _tml != nullptr || !buffer.is_empty();
}

void MidiBuffer::reset_tml() {
	_tml = midi->_get_tml_raw();
}

void MidiBuffer::stop() {
	_tml = nullptr;
	ring_buffer.clear();
}

void MidiBuffer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_buffer", "length"), &MidiBuffer::get_buffer);
	ClassDB::bind_method(D_METHOD("push_buffer", "length"), &MidiBuffer::push_buffer);
	ClassDB::bind_method(D_METHOD("fill_audio_buffer", "playback", "length"), &MidiBuffer::fill_audio_buffer, DEFVAL(-1));
	ClassDB::bind_method(D_METHOD("reset_tml"), &MidiBuffer::reset_tml);
	ClassDB::bind_method(D_METHOD("set_midi", "midi"), &MidiBuffer::set_midi);
	ClassDB::bind_method(D_METHOD("set_sf", "sf"), &MidiBuffer::set_sf);
	ClassDB::bind_method(D_METHOD("stop"), &MidiBuffer::stop);
	ClassDB::bind_method(D_METHOD("get_rb_init_capacity"), &MidiBuffer::get_rb_init_capacity);
	ClassDB::bind_method(D_METHOD("set_rb_init_capacity", "v"), &MidiBuffer::set_rb_init_capacity);
	ClassDB::bind_method(D_METHOD("get_rb_target_size"), &MidiBuffer::get_rb_target_size);
	ClassDB::bind_method(D_METHOD("set_rb_target_size", "v"), &MidiBuffer::set_rb_target_size);

	ClassDB::bind_static_method("MidiBuffer", D_METHOD("new_with_args", "sf", "midi"), &MidiBuffer::new_with_args, DEFVAL(Ref<Midi>()), DEFVAL(Ref<SoundFont>()));
	ADD_PROPERTY(PropertyInfo(Variant::INT, "rb_init_capacity"), "set_rb_init_capacity", "get_rb_init_capacity");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "rb_target_size"), "set_rb_target_size", "get_rb_target_size");
}

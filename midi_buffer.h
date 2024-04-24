#pragma once

#include "midi.h"

#include <core/object/ref_counted.h>
#include <core/object/worker_thread_pool.h>
#include <servers/audio/effects/audio_stream_generator.h>
#include <core/templates/ring_buffer.h>

using namespace godot;

inline unsigned msb(unsigned x) {
	x |= 1;
	return sizeof(unsigned) * CHAR_BIT - __builtin_clz(x);
}

class MidiBuffer : public RefCounted {
	GDCLASS(MidiBuffer, RefCounted);

	Ref<Midi> midi = nullptr;
	Ref<SoundFont> sf = nullptr;
	tml_message *_tml = nullptr;
	int rb_init_capacity = 44100 / 2;
	int rb_target_size = rb_init_capacity * 2;
	WorkerThreadPool::TaskID running_id = -1;
	RingBuffer<float> ring_buffer = RingBuffer<float>(msb(rb_init_capacity));
	SpinLock spin_lock;

protected:
	static void _bind_methods();

public:
	static Ref<MidiBuffer> new_with_args(Ref<SoundFont> sf = Ref<SoundFont>(), Ref<Midi> midi = Ref<Midi>());
	void set_midi(Ref<Midi> midi);
	void set_sf(Ref<SoundFont> sf);
	int push_buffer(int length);
	PackedFloat32Array get_buffer(int length);
	int fill_audio_buffer(Ref<AudioStreamGeneratorPlayback> playback, int length = -1);
	void reset_tml();
	void stop();
	int get_rb_init_capacity() { return rb_init_capacity; }
	void set_rb_init_capacity(int value) { rb_init_capacity = value; }
	int get_rb_target_size() { return rb_target_size; }
	void set_rb_target_size(int value) { rb_target_size = value; }
};

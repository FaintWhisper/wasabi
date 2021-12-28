#ifndef WASABI_WASAPI_HPP
#define WASABI_WASAPI_HPP

#include <iostream>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <cstdint>

class WASAPI {
private:
	IMMDevice* output_device;
	WAVEFORMATEX* format;
	IAudioClient* audio_client;
	IAudioRenderClient* audio_render_client;
	ISimpleAudioVolume* audio_volume_interface;

	void set_concurrency_mode();

	void get_default_audio_endpoint();

	void create_audio_client();

	void find_best_mix_format(int& sample_rate, int& num_channels, int& bit_depth);

	void set_mix_format();

	void initialize_audio_client();

	void get_audio_render_client();

	void get_audio_volume_interface();

public:
	WASAPI(int rendering_endpoint_buffer_duration);

	~WASAPI();

	int buffer_duration;

	void write_chunk(BYTE* chunk, uint32_t chunk_size, bool stop);

	void start();

	void stop();

	float get_volume();

	void set_volume(float volume);
};


#endif //WASABI_WASAPI_HPP

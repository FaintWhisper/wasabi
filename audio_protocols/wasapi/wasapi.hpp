#ifndef WASABI_WASAPI_HPP
#define WASABI_WASAPI_HPP

#include <iostream>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <cstdint>


class WASAPI {
private:
    IAudioClient *audio_client;
    IMMDevice *output_device;
    WAVEFORMATEX *format;
    int rendering_endpoint_buffer_duration;

    void set_concurrency_mode();

    void get_default_audio_endpoint();

    void get_mix_format();

    void set_mix_format();

    void initialize_audio_client();

    void initialize_audio_render_client();

public:
    WASAPI(int rendering_endpoint_buffer_duration);

    ~WASAPI();

    void write_data(uint32_t *chunk);

    void start();

    void stop();

    float get_volume();

    void set_volume();
};


#endif //WASABI_WASAPI_HPP

#include "wasapi.hpp"
#include <comdef.h>

#undef KSDATAFORMAT_SUBTYPE_PCM
#define SAFE_RELEASE(pointer) if ((pointer) != NULL) {(pointer)->Release(); (pointer) = NULL;}

WASAPI::WASAPI(int buffer_duration) {
    this->buffer_duration = buffer_duration;
    this->output_device = nullptr;
    this->format = (WAVEFORMATEX *) malloc(sizeof(WAVEFORMATEX));
    this->audio_client = nullptr;
    this->audio_render_client = nullptr;
    this->audio_volume_interface = nullptr;

    this->set_concurrency_mode();
    this->get_default_audio_endpoint();
    this->create_audio_client();
    this->set_mix_format();
    this->initialize_audio_client();
    this->get_audio_render_client();
    this->get_audio_volume_interface();
}

WASAPI::~WASAPI() {
    // Frees all allocated memory
    SAFE_RELEASE(this->output_device);
    SAFE_RELEASE(this->audio_client);
    SAFE_RELEASE(this->audio_render_client);
    SAFE_RELEASE(this->audio_volume_interface);
}

void WASAPI::get_default_audio_endpoint() {
    // Creates and initializes a device enumerator object and gets a reference to the interface that will be used to communicate with that object
    const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
    const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);

    IMMDeviceEnumerator *device_enumerator;

    CoCreateInstance(CLSID_MMDeviceEnumerator, nullptr, CLSCTX_ALL, IID_IMMDeviceEnumerator,
                     (void **) &device_enumerator);

    // Gets the reference to interface of the default audio endpoint
    device_enumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &this->output_device);

    // Frees allocated memory
    SAFE_RELEASE(device_enumerator);
}

void WASAPI::set_concurrency_mode() {
    // Initializes the COM library for use by the calling thread and sets the thread's concurrency model
    DWORD concurrency_model = COINIT_MULTITHREADED;

    CoInitializeEx(nullptr, concurrency_model);
}

void WASAPI::create_audio_client() {
    // Creates a COM object of the default audio endpoint with the audio client interface activated
    this->output_device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void **) &this->audio_client);
}

void WASAPI::find_best_mix_format(int &sample_rate, int &num_channels, int &bit_depth) {
    // Gets the audio format used by the audio client interface (mmsys.cpl -> audio endpoint properties -> advanced options)

    // TODO

    sample_rate = 48000;
    num_channels = 2;
    bit_depth = 16;
}

void WASAPI::set_mix_format() {
    const GUID KSDATAFORMAT_SUBTYPE_PCM = {0x00000001, 0x0000, 0x0010,
                                           {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};

    //
    int sample_rate;
    int num_channels;
    int bit_depth;

    this->find_best_mix_format(sample_rate, num_channels, bit_depth);

    //
    WAVEFORMATEXTENSIBLE mix_format;

    mix_format.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    mix_format.Format.nSamplesPerSec = sample_rate;
    mix_format.Format.nChannels = num_channels;
    mix_format.Format.wBitsPerSample = bit_depth;
    mix_format.Format.nBlockAlign = (num_channels * bit_depth) / 8;
    mix_format.Format.nAvgBytesPerSec = sample_rate * mix_format.Format.nBlockAlign;
    mix_format.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
    mix_format.Samples.wValidBitsPerSample = bit_depth;
    mix_format.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
    mix_format.dwChannelMask = KSAUDIO_SPEAKER_STEREO;

    std::cout << "[Checking supported mix format]" << std::endl;

    WAVEFORMATEX *closest_format = nullptr;

    HRESULT result = this->audio_client->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, (WAVEFORMATEX *) &mix_format,
                                                           &closest_format);

    if (result == S_OK) {
        std::cout << "\nThe mix format is supported!" << std::endl;

        this->format->wFormatTag = WAVE_FORMAT_PCM;
        this->format->nSamplesPerSec = mix_format.Format.nSamplesPerSec;
        this->format->nChannels = mix_format.Format.nChannels;
        this->format->wBitsPerSample = mix_format.Format.wBitsPerSample;
        this->format->nBlockAlign = (mix_format.Format.nChannels * mix_format.Format.wBitsPerSample) / 8;
        this->format->nAvgBytesPerSec = mix_format.Format.nSamplesPerSec * mix_format.Format.nBlockAlign;
        this->format->cbSize = 0;
    } else if (result == S_FALSE) {
        std::cout
                << "WARNING: The requested mix format is not supported, a closest match will be tried instead (it may not work)."
                << std::endl;

        this->format->wFormatTag = WAVE_FORMAT_PCM;
        this->format->nSamplesPerSec = closest_format->nSamplesPerSec;
        this->format->nChannels = closest_format->nChannels;
        this->format->wBitsPerSample = closest_format->wBitsPerSample;
        this->format->nBlockAlign = (closest_format->nChannels * closest_format->wBitsPerSample) / 8;
        this->format->nAvgBytesPerSec = closest_format->nSamplesPerSec * closest_format->nBlockAlign;
        this->format->cbSize = 0;
    } else {
        std::cerr << "ERROR: Unable to establish a supported mix format." << std::endl;

        exit(EXIT_FAILURE);
    }

    // Frees allocated memory
    CoTaskMemFree(closest_format);
}

void WASAPI::initialize_audio_client() {
    const int REFTIMES_PER_SEC = 10000000;
    REFERENCE_TIME req_duration = this->buffer_duration * REFTIMES_PER_SEC;

    // Initializes the audio client interface
    HRESULT result = this->audio_client->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, req_duration, 0, this->format, nullptr);

    if (result != S_OK) {
        std::cerr << "ERROR: Unable to initialize audio client." << std::endl;

        exit(EXIT_FAILURE);
    }
}

void WASAPI::get_audio_render_client() {
    // Gets a reference to the audio render client interface of the audio client
    this->audio_client->GetService(__uuidof(IAudioRenderClient), (void **) &this->audio_render_client);
}

void WASAPI::write_chunk(BYTE *chunk, uint32_t chunk_size, bool stop) {
    // Declares rendering endpoint buffer flags
    DWORD buffer_flags = 0;

    // Gets the number of audio frames available in the rendering endpoint buffer
    uint32_t num_buffer_frames;

    this->audio_client->GetBufferSize(&num_buffer_frames);

    // Gets the amount of valid data that is currently stored in the buffer but hasn't been read yet
    uint32_t num_padding_frames;

    this->audio_client->GetCurrentPadding(&num_padding_frames);

    // Gets the amount of available space in the buffer
    num_buffer_frames -= num_padding_frames;

    // Retrieves a pointer to the next available memory space in the rendering endpoint buffer
    BYTE *buffer;

    this->audio_render_client->GetBuffer(num_buffer_frames, &buffer);

    //
    memcpy(buffer, chunk, chunk_size);
    free(chunk);
    this->audio_client->GetCurrentPadding(&num_padding_frames);

    if (stop) {
        buffer_flags = AUDCLNT_BUFFERFLAGS_SILENT;
    }

    this->audio_render_client->ReleaseBuffer(num_buffer_frames, buffer_flags);
}

void WASAPI::start() {
    this->audio_client->Start();
}

void WASAPI::stop() {
    this->audio_client->Stop();
}

void WASAPI::get_audio_volume_interface() {
    // Gets a reference to the session volume control interface of the audio client
    this->audio_client->GetService(__uuidof(ISimpleAudioVolume), (void **) &this->audio_volume_interface);
}

float WASAPI::get_volume() {
    float current_volume;

    // Gets the session master volume of the audio client
    this->audio_volume_interface->GetMasterVolume(&current_volume);

    return current_volume;
}

void WASAPI::set_volume(float volume) {
    // Sets the session master volume of the audio client
    this->audio_volume_interface->SetMasterVolume(volume, nullptr);
}
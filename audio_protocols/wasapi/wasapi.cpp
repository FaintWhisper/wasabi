#include "WASAPI.hpp"

#undef KSDATAFORMAT_SUBTYPE_PCM
const GUID KSDATAFORMAT_SUBTYPE_PCM = {0x00000001, 0x0000, 0x0010,
                                       {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};

#define SAFE_RELEASE(pointer) if ((pointer) != NULL) {(pointer)->Release(); (pointer) = NULL;}

WASAPI::WASAPI(int rendering_endpoint_buffer_duration) {
    this->rendering_endpoint_buffer_duration = rendering_endpoint_buffer_duration;

    this->get_default_audio_endpoint();
    this->set_concurrency_mode();
    this->set_mix_format();
    this->initialize_audio_client();
    this->initialize_audio_render_client();
}

WASAPI::~WASAPI() {
    // Frees all allocated memory
    CoTaskMemFree(closest_format);
    SAFE_RELEASE(device_enumerator);
    SAFE_RELEASE(device);
    SAFE_RELEASE(audio_client);
    SAFE_RELEASE(audio_render_client);
}

void WASAPI::get_default_audio_endpoint() {
    const int REFTIMES_PER_SEC = 10000000;
    const int REFTIMES_PER_MILLISEC = 10000;

    // Creates and initializes a device enumerator object and gets a reference to the interface that will be used to communicate with that object
    const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
    const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
    const IID IID_IAudioClient = __uuidof(IAudioClient);
    const IID IID_IAudioRenderClient = __uuidof(IAudioRenderClient);
    const IID IID_ISimpleAudioVolume = __uuidof(ISimpleAudioVolume);

    IMMDeviceEnumerator *device_enumerator = nullptr;

    CoCreateInstance(CLSID_MMDeviceEnumerator, nullptr, CLSCTX_ALL, IID_IMMDeviceEnumerator,
                     (void **) &device_enumerator);

    // Gets the reference to interface of the default audio endpoint
    device_enumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &device);
}

void WASAPI::set_concurrency_mode() {
    // Initializes the COM library for use by the calling thread and sets the thread's concurrency model
    DWORD concurrency_model = COINIT_MULTITHREADED;

    CoInitializeEx(nullptr, concurrency_model);
}

void WASAPI::get_mix_formats() {
    // Gets the audio format used by the audio client interface (mmsys.cpl -> audio endpoint properties -> advanced options)

    // TODO
}

void WASAPI::set_mix_format() {
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

    std::cout << "[Checking supported mix format]";

    WAVEFORMATEX *closest_format = nullptr;

    HRESULT result = this->audio_client->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, (WAVEFORMATEX *) &mix_format,
                                                     &closest_format);

    if (result == S_OK) {
        std::cout << "\nThe mix format is supported!" << std::endl;

        format = (WAVEFORMATEX *) &mix_format;
    } else if (result == S_FALSE) {
        std::cout
                << "WARNING: The requested mix format is not supported, a closest match will be tried instead (it may not work)."
                << std::endl;

        format = closest_format;
    } else {
        std::cerr << "ERROR: Unable to establish a supported mix format." << std::endl;

        exit(EXIT_FAILURE);
    }
}

void WASAPI::initialize_audio_client() {
    // Creates a COM object of the default audio endpoiint with the audio client interface activated
    this->output_device->Activate(IID_IAudioClient, CLSCTX_ALL, nullptr, (void **) &audio_client);

    // Initializes the audio client interface
    REFERENCE_TIME req_duration = rendering_endpoint_buffer_duration * REFTIMES_PER_SEC;

    HRESULT result = this->audio_client->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, req_duration, 0, format, nullptr);

    if (result != S_OK) {
        std::cerr << "ERROR: Unable to initialize audio client." << std::endl;

        exit(EXIT_FAILURE);
    }
}

void WASAPI::initialize_audio_render_client() {
    // Gets a reference to the audio render client interface of the audio client
    IAudioRenderClient *audio_render_client;

    this->audio_client->GetService(IID_IAudioRenderClient, (void **) &audio_render_client);

    // Declares audio buffer related variables
    BYTE *buffer;
    UINT32 num_buffer_frames;
    UINT32 num_padding_frames;
    DWORD buffer_audio_duration;

    // Gets the number of audio frames available in the rendering endpoint buffer
    this->audio_client->GetBufferSize(&num_buffer_frames);
}

void WASAPI::write_data(uint32 *chunk) {
    // Gets the amount of valid data that is currently stored in the buffer but hasn't been read yet
    this->audio_client->GetCurrentPadding(&num_padding_frames);

    // Gets the amount of available space in the buffer
    num_buffer_frames -= num_padding_frames;

    // Retrieves a pointer to the next available memory space in the rendering endpoint buffer
    audio_render_client->GetBuffer(num_buffer_frames, &buffer);

    // TODO

    // Get the rendering endpoint  buffer duration
    buffer_audio_duration =
            (DWORD) (num_buffer_frames / format->nSamplesPerSec) *
            (REFTIMES_PER_SEC / REFTIMES_PER_MILLISEC);

    if (stop) {
        buffer_flags = AUDCLNT_BUFFERFLAGS_SILENT;
    }

    audio_render_client->ReleaseBuffer(num_buffer_frames, buffer_flags);
}

void WASAPI::start() {
    result = this->audio_client->Start();
}

void WASAPI::stop() {
    this->audio_client->Stop();
}

float WASAPI::get_volume() {
    ISimpleAudioVolume *audio_volume;
    float current_volume;

    // Gets a reference to the session volume control interface of the audio client
    this->audio_client->GetService(IID_ISimpleAudioVolume, (void **) &audio_volume);

    // Gets the session master volume of the audio client
    audio_volume->GetMasterVolume(&current_volume);

    return current_volume;
}

void WASAPI::set_volume() {
    ISimpleAudioVolume *audio_volume;

    // Gets a reference to the session volume control interface of the audio client
    this->audio_client->GetService(IID_ISimpleAudioVolume, (void **) &audio_volume);

    // Sets the session master volume of the audio client
    float volume = 0.2;
    audio_volume->SetMasterVolume(volume, nullptr);
}
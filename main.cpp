#include <iostream>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include "wav_reader.hpp"

#undef KSDATAFORMAT_SUBTYPE_PCM
const GUID KSDATAFORMAT_SUBTYPE_PCM = {0x00000001, 0x0000, 0x0010,
                                       {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};

using namespace std;

#define SAFE_RELEASE(pointer) if ((pointer) != NULL) {(pointer)->Release(); (pointer) = NULL;}

VOID play_audio_stream(string file_path, int rendering_endpoint_buffer_duration) {
    const int REFTIMES_PER_SEC = 10000000;
    const int REFTIMES_PER_MILLISEC = 10000;

    // Initializes the COM library for use by the calling thread and sets the thread's concurrency model
    DWORD concurrency_model = COINIT_MULTITHREADED;

    CoInitializeEx(nullptr, concurrency_model);

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
    IMMDevice *device = nullptr;

    device_enumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &device);

    // Creates a COM object of the default audio endpoiint with the audio client interface activated
    IAudioClient *audio_client;

    device->Activate(IID_IAudioClient, CLSCTX_ALL, nullptr, (void **) &audio_client);

    // Gets the audio format used by the audio client interface
    WAVEFORMATEX *format;
    WAVEFORMATEX *closest_format = nullptr;

    // Set audio endpoint default format parameters (mmsys.cpl -> audio endpoint properties -> advanced options)
    int num_channels = 2;
    int bit_depth = 16;
    int sample_rate = 48000;

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

    HRESULT result = audio_client->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, (WAVEFORMATEX *) &mix_format,
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

    // Initializes the audio client interface
    REFERENCE_TIME req_duration = rendering_endpoint_buffer_duration * REFTIMES_PER_SEC;

    result = audio_client->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, req_duration, 0, format, nullptr);

    if (result != S_OK) {
        std::cerr << "ERROR: Unable to initialize audio client." << std::endl;

        exit(EXIT_FAILURE);
    } else {
        // Gets a reference to the audio render client interface of the audio client
        IAudioRenderClient *audio_render_client;

        audio_client->GetService(IID_IAudioRenderClient, (void **) &audio_render_client);

        ISimpleAudioVolume *audio_volume;
        // Gets a reference to the session volume control interface of the audio client
        audio_client->GetService(IID_ISimpleAudioVolume, (void **) &audio_volume);

        // Sets the session master volume of the audio client
        float volume = 0.2;
        audio_volume->SetMasterVolume(volume, nullptr);

        // Declares audio buffer related variables
        BYTE *buffer;
        UINT32 num_buffer_frames;
        UINT32 num_padding_frames;
        DWORD buffer_audio_duration;

        // Gets the number of audio frames available in the rendering endpoint buffer
        audio_client->GetBufferSize(&num_buffer_frames);

        // Declares variables to control the playback
        bool stop = FALSE;
        bool playing = FALSE;

        // Declares rendering endpoint buffer flags
        DWORD buffer_flags = 0;

        // Instantiates a WAV format reader
        WAVReader wav_reader = WAVReader();
        wav_reader.load_file(&file_path);

        // Declares and initializes the variables that will keep track of the playing time
        int current_minutes = 0;
        int current_seconds = 0;

        while (stop == FALSE) {
            if ((current_seconds + 1) % rendering_endpoint_buffer_duration == 0) {
                // Gets the amount of valid data that is currently stored in the buffer but hasn't been read yet
                audio_client->GetCurrentPadding(&num_padding_frames);

                // Gets the amount of available space in the buffer
                num_buffer_frames -= num_padding_frames;

                // Retrieves a pointer to the next available memory space in the rendering endpoint buffer
                audio_render_client->GetBuffer(num_buffer_frames, &buffer);

                // Load the audio data in the rendering endpoint buffer
                stop = wav_reader.write_data(buffer);

                // Get the rendering endpoint  buffer duration
                buffer_audio_duration =
                        (DWORD) (num_buffer_frames / format->nSamplesPerSec) *
                        (REFTIMES_PER_SEC / REFTIMES_PER_MILLISEC);

                if (stop) {
                    buffer_flags = AUDCLNT_BUFFERFLAGS_SILENT;
                }

                audio_render_client->ReleaseBuffer(num_buffer_frames, buffer_flags);

                if (playing == FALSE) {
                    result = audio_client->Start();

                    if (result == S_OK) {
                        cout << endl << "[Starting to play the file]" << endl;
                        cout << "Number of available frames in rendering buffer: " << num_buffer_frames << endl;
                        cout << "Buffer duration: " << buffer_audio_duration << " ms" << endl;

                        float duration = wav_reader.data_subchunk_size / wav_reader.byte_rate;
                        int minutes = (int) (duration / 60);
                        int seconds = (int) duration - (minutes * 60);

                        printf("Audio duration: %dm %.2ds\n", minutes, seconds);

                        playing = TRUE;
                    } else {
                        exit(EXIT_FAILURE);
                    }
                }
            }

            printf("\rCurrente time: %dm %.2ds", current_minutes, current_seconds);
            fflush(stdout);

            current_seconds += 1;

            if (current_seconds == 60) {
                current_minutes += 1;
                current_seconds = 0;
            }

            // Sleep exactly one second
            Sleep(1000);
        }

        audio_client->Stop();

        SAFE_RELEASE(audio_render_client);
    }

    // Free all allocated memory
    CoTaskMemFree(closest_format);
    SAFE_RELEASE(device_enumerator);
    SAFE_RELEASE(device);
    SAFE_RELEASE(audio_client);
}

void parse_args(int argc, char *argv[], string *file_path, int *rendering_endpoint_buffer_duration) {
    // Checks if all parameters are provided, if not, initializes all required but non defined parameters with their default values
    int file_pos = -1;
    int rendering_endpoint_buffer_duration_pos = -1;

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "--file") == TRUE) {
            if ((i + 1) < (argc - 1)) {
                file_pos = i + 1;
            }
        } else if (strcmp(argv[i], "--rendering_endpoint_buffer_duration") == TRUE) {
            if ((i + 1) < (argc - 1)) {
                rendering_endpoint_buffer_duration_pos = i + 1;
            }
        }
    }

    if (file_pos != -1) {
        *file_path = strtol(argv[file_pos], nullptr, 10);
    } else {
        string input_file_path;

        std::cout << "Input file: ";
        std::getline(std::cin, input_file_path);

        *file_path = input_file_path;
    }

    if (rendering_endpoint_buffer_duration_pos != -1) {
        *rendering_endpoint_buffer_duration = strtol(argv[rendering_endpoint_buffer_duration_pos], nullptr, 10);
    } else {
        *rendering_endpoint_buffer_duration = 1;
    }
}

int main(int argc, char *argv[]) {
    string file;
    int rendering_endpoint_buffer_duration;

    parse_args(argc, argv, &file, &rendering_endpoint_buffer_duration);
    play_audio_stream(file, rendering_endpoint_buffer_duration);

    return 0;
}
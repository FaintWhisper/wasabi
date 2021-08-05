#include <comdef.h>
#include <iostream>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include "wav_reader.hpp"

#undef KSDATAFORMAT_SUBTYPE_PCM
const GUID KSDATAFORMAT_SUBTYPE_PCM = { 0x00000001, 0x0000, 0x0010,
                                        {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71} };

using namespace std;

#define MAX_PATH_LENGTH 300
#define SAFE_RELEASE(punk) if ((punk) != NULL) {(punk)->Release(); (punk) = NULL;}

VOID PlayAudioStream(char audio_file[]) {
    const int REFTIMES_PER_SEC = 10000000;
    const int REFTIMES_PER_MILLISEC = 10000;

    // Initializes the COM library for use by the calling thread and sets the thread's concurrency model
    DWORD concurrency_model = COINIT_MULTITHREADED;

    CoInitializeEx(NULL, concurrency_model);

    // Creates and initializes a device enumerator object and gets a reference to the interface that will be used to communicate with that object
    const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
    const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
    const IID IID_IAudioClient = __uuidof(IAudioClient);
    const IID IID_IAudioRenderClient = __uuidof(IAudioRenderClient);

    IMMDeviceEnumerator *device_enumerator = NULL;

    CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void **) &device_enumerator);

    // Gets the reference to interface of the default audio endpoint
    IMMDevice *device = NULL;

    device_enumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &device);

    // Creates a COM object of the default audio endpoiint with the audio client interface activated
    IAudioClient *audio_client;

    device->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void **) &audio_client);

    // Gets the audio format used by the audio client interface
    WAVEFORMATEX *format = NULL;
    WAVEFORMATEX *closest_format = NULL;

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

    HRESULT result = audio_client->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, (WAVEFORMATEX*) &mix_format, &closest_format);

    if (result == S_OK) {
        std::cout << "The mix format is supported!" << std::endl;

        format = (WAVEFORMATEX*) &mix_format;
    } else if (result == S_FALSE) {
        std::cout << "The requested mix format is not supported, a closest match was provided." << std::endl;

        format = closest_format;
    } else {
        std::cout << "ERROR: Unable to establish a supported mix format." << std::endl;
        exit(EXIT_FAILURE);
    }

    // Initializes the audio client interface
    REFERENCE_TIME req_duration = REFTIMES_PER_SEC;

    result = audio_client->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, req_duration, 0, format, NULL);

    // Gets a reference to the audio render client interface of the audio client
    IAudioRenderClient *audio_render_client;

    audio_client->GetService(IID_IAudioRenderClient, (void **) &audio_render_client);

    // Declares audio buffer related variables
    BYTE *buffer;
    UINT32 num_buffer_frames;
    UINT32 num_padding_frames;
    double buffer_audio_duration;

    // Gets the number of audio frames available in the rendering endpoint buffer
    audio_client->GetBufferSize(&num_buffer_frames);

    // Declares variables to control the playback
    bool stop = FALSE;
    bool playing = FALSE;

    // Declares rendering endpoint buffer flags
    DWORD buffer_flags = 0;

    // Instantiates a WAV format reader
    WAVReader wav_reader;
    wav_reader.load_file(audio_file);

    // Declares and initializes the variables that will keep track of the playing time
    int current_minutes = 0;
    int current_seconds = 0;

    while (stop == FALSE) {
        audio_client->GetCurrentPadding(&num_padding_frames);
        num_buffer_frames -= num_padding_frames;

        // Retrieves a pointer to the next available memory space in the rendering endpoint buffer
        audio_render_client->GetBuffer(num_buffer_frames, &buffer);

        // Load the audio data in the rendering endpoint buffer
        stop = wav_reader.write_data(buffer, num_buffer_frames, format);

        if (stop) {
            buffer_flags = AUDCLNT_BUFFERFLAGS_SILENT;
        }

        audio_render_client->ReleaseBuffer(num_buffer_frames, buffer_flags);

        buffer_audio_duration =
                (DWORD) (num_buffer_frames / format->nSamplesPerSec) * (REFTIMES_PER_SEC / REFTIMES_PER_MILLISEC);

        if (playing == FALSE) {
            HRESULT result = audio_client->Start();

            cout << endl << "Starting to play the file" << endl;
            cout << "Number of Available Frames in Rendering Buffer: " << num_buffer_frames << endl;
            cout << "Audio Buffer Size: " << wav_reader.audio_buffer_size << " bytes" << endl;
            cout << "Buffer Duration: " << buffer_audio_duration << " ms" << endl;

            float duration = wav_reader.data_subchunk_size / wav_reader.byte_rate;
            int minutes = (int) (duration / 60);
            int seconds = (int) duration - (minutes * 60);
            printf("Audio Duration: %dm %.2ds\n", minutes, seconds);

            playing = TRUE;
        }

        printf("\rCurrente Time: %dm %.2ds", current_minutes, current_seconds);
        fflush(stdout);

        Sleep(buffer_audio_duration);
        current_seconds += 1;

        if (current_seconds == 60) {
            current_minutes += 1;
            current_seconds = 0;
        }
    }

    audio_client->Stop();

    CoTaskMemFree(format);
    SAFE_RELEASE(device_enumerator);
    SAFE_RELEASE(device);
    SAFE_RELEASE(audio_client);
    SAFE_RELEASE(audio_render_client);
}

BOOL main(int argc, char *argv[]) {
    string input_path;

    if (argv[1] == NULL) {
        char path[MAX_PATH_LENGTH];

        std::cout << "Input file: ";
        std::getline(std::cin, input_path);
        strcpy(path, input_path.c_str());

        PlayAudioStream(path);
    } else {
        PlayAudioStream(argv[1]);
    }

    return TRUE;
}
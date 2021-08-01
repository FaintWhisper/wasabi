#include <comdef.h>
#include <iostream>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include "wav_reader.hpp"

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

    audio_client->GetMixFormat(&format);

    // Initializes the audio client interface
    REFERENCE_TIME req_duration = REFTIMES_PER_SEC;

    HRESULT result = audio_client->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, req_duration, 0, format, NULL);

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

    // Instantiates a WAV format reader
    WAVReader wav_reader;
    wav_reader.load_file(audio_file);

    while (stop == FALSE) {
        audio_client->GetCurrentPadding(&num_padding_frames);
        num_buffer_frames -= num_padding_frames;
        cout << endl << "Number of Frames: " << num_buffer_frames << endl;

        // Retrieves a pointer to the next available memory space in the rendering endpoint buffer
        audio_render_client->GetBuffer(num_buffer_frames, &buffer);

        // Load the audio data in the rendering endpoint buffer
        stop = wav_reader.write_data(buffer, num_buffer_frames, format);

        audio_render_client->ReleaseBuffer(num_buffer_frames, AUDCLNT_BUFFERFLAGS_SILENT);

        if (playing == FALSE) {
            HRESULT result = audio_client->Start();
            playing = TRUE;
        }

        buffer_audio_duration =
                (DWORD) (num_buffer_frames / format->nSamplesPerSec) * (REFTIMES_PER_SEC / REFTIMES_PER_MILLISEC);

        cout << "Buffer Duration: " << buffer_audio_duration << endl;

        Sleep(buffer_audio_duration);
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
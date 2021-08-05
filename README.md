## wasabi
A WASAPI audio stream renderer integrated with a WAV format specification checker and data reader.

For now, it only works with LPCM encoded WAV audio files that have the same parameters as the ones specified by the format of the default audio output device (you can check them in the Sound Control Panel, mmsys.cpl -> audio endpoint properties -> advanced options). In the future, the default format will be checked and the audio will be converted accordingly.

TODO:
- Load audio in chunks from a separate thread rather than doing it all at once, thus reducing the wait time until the playback begins.
- Control the audio endpoint volume.
- Add audio playback control.
- Convert the audio to the same format that the default audio output device is using.
- Support more audio formats (such as other PCM types and non PCM encoded WAV files, FLAC, ALAC, AIFF, MP3, etc).
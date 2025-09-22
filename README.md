## wasabi
A WASAPI audio stream renderer integrated with a WAV format specification checker and data reader.

---

#### Compatibility
For now, it only works with LPCM encoded WAV audio files that have the same parameters as the ones specified by the format of the default audio output device (you can check them in the Sound Control Panel, mmsys.cpl -> audio endpoint properties -> advanced options). In the future, the default format will be checked and the audio will be converted accordingly.

#### TODO:
- Implement WAVReader class constructor (rather than relying on default initialization) and destructor.
- Add parameter validation.
- Extend playback control by adding the possibility to skip forward and backward.
- Register a callback to receive notifications when the volume of the audio session has been changed using the Volume Mixer so that it is correctly updated when displayed on screen.
- Convert the audio to the same format that the default audio output device is using.
- Support more audio formats (such as other PCM types and non PCM encoded WAV files, FLAC, ALAC, AIFF, MP3, etc).
- DONE:
  - Stream audio data progressively through background threading, enabling async chunked decoding and immediate playback. This minimizes initial buffering delays while consuming much less memory by incremental data processing instead of loading the complete file in memory upfront.
  - Add audio session volume control.
  - Add audio playback control.

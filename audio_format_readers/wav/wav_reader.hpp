#ifndef WAVReader_H
#define WAVReader_H

#include <fstream>
#include <sstream>
#include <windows.h>

#define MAX_FILE_PATH_SIZE 40

class WAVReader {
private:
    char file_path[MAX_FILE_PATH_SIZE];
    uint32_t audio_buffer_size;
    BYTE *audio_buffer;
public:
    WAVReader();

    ~WAVReader();

    void load_file(const char path[]);

    void check_riff_header(std::ifstream *file);

    void load_fmt_subchunk(std::ifstream *file);

    void load_data_subchunk(std::ifstream *file);

    bool write_data(BYTE *buffer, int num_buffer_frames, WAVEFORMATEX *format);
};

#endif
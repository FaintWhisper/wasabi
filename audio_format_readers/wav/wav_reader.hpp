#ifndef WAVReader_H
#define WAVReader_H

#include <fstream>
#include <sstream>
#include <windows.h>

#define MAX_FILE_PATH_SIZE 40

class WAVReader {
private:
    BYTE *audio_buffer;
public:
    char file_path[MAX_FILE_PATH_SIZE];
    char chunk_id[4];
    uint32_t chunk_size;
    char format_descriptor[4];
    char fmt_subchunk_id[4];
    uint32_t fmt_subchunk_size;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bit_depth;
    char data_subchunk_id[4];
    uint32_t data_subchunk_size;
    uint32_t audio_buffer_size;
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
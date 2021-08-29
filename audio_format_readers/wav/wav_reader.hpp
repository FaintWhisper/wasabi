#ifndef WASABI_WAVReader_H
#define WASABI_WAVReader_H

#include <fstream>
#include <string>
#include <condition_variable>
#include <mutex>
#include <windows.h>

#define MAX_AUDIO_BUFFER_CHUNKS 5

typedef struct AUDIO_BUFFER_CHUNK {
    BYTE *data{};
    uint32_t size{};
    bool is_written{};
    bool is_eof{};
} AUDIO_BUFFER_CHUNK;

class WAVReader {
private:
    AUDIO_BUFFER_CHUNK audio_buffer_chunks[MAX_AUDIO_BUFFER_CHUNKS]{};
    int current_audio_buffer_chunk{};
    bool is_audio_buffer_ready{};
    bool is_playback_started{};
    std::condition_variable *cv{};
    std::mutex mtx;

    void check_riff_header(std::shared_ptr<std::ifstream> file);

    void load_fmt_subchunk(std::shared_ptr<std::ifstream> file);

    void load_data_subchunk(std::shared_ptr<std::ifstream> file);

    void load_data(std::shared_ptr<std::ifstream> file);

public:
    WAVReader();

    WAVReader(WAVReader const &reader);

    ~WAVReader();

    std::string audio_file_path{};
    char chunk_id[4]{};
    uint32_t chunk_size{};
    char format_descriptor[4]{};
    char fmt_subchunk_id[4]{};
    uint32_t fmt_subchunk_size{};
    uint16_t audio_format{};
    uint16_t num_channels{};
    uint32_t sample_rate{};
    uint32_t byte_rate{};
    uint16_t block_align{};
    uint16_t bit_depth{};
    char data_subchunk_id[4]{};
    uint32_t data_subchunk_size{};
    uint32_t audio_buffer_chunk_size{};
    struct audio_duration {
        int minutes;
        int seconds;
    } audio_duration;

    void load_file(std::string *file_path);

    bool get_chunk(BYTE **chunk, uint32_t &chunk_size);
};

#endif //WASABI_WAVReader_H
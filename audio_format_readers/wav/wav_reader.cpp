#include "wav_reader.hpp"
#include <iostream>
#include <string>
#include <cstdint>
#include <cstring>
#include <thread>
#include <windows.h>

WAVReader::WAVReader() = default;

WAVReader::WAVReader(const WAVReader &reader) {}

WAVReader::~WAVReader() = default;

void WAVReader::load_file(std::string *file_path) {
    if (!file_path->empty()) {
        this->audio_file_path = *file_path;
        std::shared_ptr<std::ifstream> file = std::make_shared<std::ifstream>(this->audio_file_path,
                                                                              std::ios::in | std::ios::binary);

        if (file != nullptr && file->is_open()) {
            std::cout << "\n[Loaded \"" << *file_path << "\"]" << std::flush;

            check_riff_header(file);
            load_fmt_subchunk(file);
            load_data_subchunk(file);

            std::thread data_loader(&WAVReader::load_data, this, file);
            data_loader.detach();
        } else {
            std::cerr << "ERROR: The provided file path doesn't point to an existing file" << std::endl;
        }
    } else {
        std::cerr << "ERROR: A file path must be provided" << std::endl;
    }
};

void WAVReader::check_riff_header(std::shared_ptr<std::ifstream> file) {
    char file_chunk_id[5];
    uint32_t file_chunk_size;
    char file_format[5];

    std::cout << "\nChecking the file header..." << std::endl;

    // Checks the chunk ID
    file->read(file_chunk_id, sizeof(file_chunk_id) - 1);

    file_chunk_id[sizeof(file_chunk_id) - 1] = '\0';

    if (strncmp(file_chunk_id, "RIFF", sizeof(file_chunk_id) - 1) == 0) {
        std::cout << "Chunk ID: Ok." << std::endl;

        strcpy(this->chunk_id, file_chunk_id);
    } else {
        std::cerr << "ERROR: Invalid chunk ID." << std::endl;

        exit(EXIT_FAILURE);
    }

    // Gets the chunk size (in bytes)
    file->read((char *) &file_chunk_size, sizeof(file_chunk_size));

    if (file_chunk_size >= 36) {
        std::cout << "Chunk size: Ok (" << file_chunk_size << " bytes)." << std::endl;

        this->chunk_size = file_chunk_size;
    } else {
        std::cerr << "ERROR: Bad chunk size." << std::endl;

        exit(EXIT_FAILURE);
    }

    // Checks the format descriptor
    file->read(file_format, sizeof(file_format) - 1);

    file_chunk_id[sizeof(file_chunk_id) - 1] = '\0';

    if (strncmp(file_format, "WAVE", sizeof(file_format) - 1) == 0) {
        std::cout << "Format descriptor: Ok." << std::endl;

        strcpy(this->format_descriptor, file_format);
    } else {
        std::cerr << "ERROR: Invalid format descriptor." << std::endl;

        exit(EXIT_FAILURE);
    }
};

void WAVReader::load_fmt_subchunk(std::shared_ptr<std::ifstream> file) {
    char file_fmt_id[5];
    uint32_t file_fmt_size;
    uint16_t file_fmt_audio_format;
    uint16_t file_fmt_num_channels;
    uint32_t file_fmt_sample_rate;
    uint32_t file_fmt_byte_rate;
    uint16_t file_fmt_block_align;
    uint16_t file_fmt_bit_depth;

    std::cout << "\nReading the 'fmt' subchunk..." << std::endl;

    // Checks the fmt subchunk ID
    file->read(file_fmt_id, sizeof(file_fmt_id) - 1);

    file_fmt_id[sizeof(file_fmt_id) - 1] = '\0';

    if (strncmp(file_fmt_id, "fmt", sizeof(file_fmt_id) - 2) == 0) {
        std::cout << "fmt subchunk ID: Ok." << std::endl;

        strcpy(this->chunk_id, file_fmt_id);
    } else {
        std::cerr << "Error: Invalid fmt subchunk ID." << std::endl;

        exit(EXIT_FAILURE);
    }

    // Gets the fmt subchunk size and checks the audio encoding
    file->read((char*) &file_fmt_size, sizeof(file_fmt_size));

    if (file_fmt_size == 16) {
        std::cout << "fmt subchunk size: Ok (" << file_fmt_size << " bytes)." << std::endl;

        this->fmt_subchunk_size = file_fmt_size;
    } else {
        std::cerr << "Error: Bad encoding, only PCM encoded wav audio files are currently supported." << std::endl;

        exit(EXIT_FAILURE);
    }

    // Checks the audio format
    file->read(reinterpret_cast<char *> (&file_fmt_audio_format), sizeof(file_fmt_audio_format));

    if (file_fmt_audio_format == 1) {
        std::cout << "Audio format: Ok." << std::endl;

        this->audio_format = file_fmt_audio_format;
    } else {
        std::cerr << "Error: Bad audio format, only linear PCM encoded wav audio files are currently supported."
                  << std::endl;

        exit(EXIT_FAILURE);
    }

    // Gets the number of channels
    file->read(reinterpret_cast<char *> (&file_fmt_num_channels), sizeof(file_fmt_num_channels));

    if (file_fmt_num_channels == 1 || file_fmt_num_channels == 2) {
        if (file_fmt_num_channels == 1) {
            std::cout << "Number of channels: 1 (mono)" << std::endl;
        } else {
            std::cout << "Number of channels: 2 (stereo)" << std::endl;
        }

        this->num_channels = file_fmt_num_channels;
    } else {
        std::cerr << "Error: Bad number of channels, only mono and stereo audio files are currently supported."
                  << std::endl;

        exit(EXIT_FAILURE);
    }

    // Gets the sample rate
    file->read(reinterpret_cast<char *> (&file_fmt_sample_rate), sizeof(file_fmt_sample_rate));

    if (file_fmt_sample_rate == 44100 || file_fmt_sample_rate == 48000) {
        std::cout << "Sample rate: Ok (" << file_fmt_sample_rate << " Hz)." << std::endl;

        this->sample_rate = file_fmt_sample_rate;
    } else {
        std::cerr << "Error: Bad sample rate, only audio files sampled at 44100 or 48000 Hz are currently supported."
                  << std::endl;

        exit(EXIT_FAILURE);
    }

    // Gets the byte rate
    file->read(reinterpret_cast<char *> (&file_fmt_byte_rate), sizeof(file_fmt_byte_rate));

    // Gets the block alignment
    file->read(reinterpret_cast<char *> (&file_fmt_block_align), sizeof(file_fmt_block_align));

    // Gets the size of each sample (in bits)
    file->read(reinterpret_cast<char *> (&file_fmt_bit_depth), sizeof(file_fmt_bit_depth));

    // Checks the byte rate
    if (file_fmt_byte_rate ==
        (file_fmt_sample_rate * file_fmt_num_channels * (file_fmt_bit_depth / 8))) {
        std::cout << "Byte rate: Ok (" << file_fmt_byte_rate << " bytes)." << std::endl;

        this->byte_rate = file_fmt_byte_rate;
    } else {
        std::cerr << "Error: Bad byte rate" << std::endl;

        exit(EXIT_FAILURE);
    }

    // Checks the block alignment
    if (file_fmt_block_align == (file_fmt_num_channels * (file_fmt_bit_depth / 8))) {
        std::cout << "Block alignment: Ok (" << file_fmt_block_align << " bytes)." << std::endl;

        this->byte_rate = file_fmt_byte_rate;
    } else {
        std::cerr << "Error: Bad block alignment" << std::endl;

        exit(EXIT_FAILURE);
    }

    // Checks the bit depth
    if (file_fmt_bit_depth % 8 == 0) {
        std::cout << "Bit Depth: Ok (" << file_fmt_bit_depth << " bits)." << std::endl;

        this->bit_depth = file_fmt_bit_depth;
    } else {
        std::cerr << "Error: Invalid bit depth" << std::endl;

        exit(EXIT_FAILURE);
    }
};

void WAVReader::load_data_subchunk(std::shared_ptr<std::ifstream> file) {
    char file_data_id[5];
    uint32_t file_data_size;

    std::cout << "\nReading the 'data' subchunk..." << std::endl;

    // Checks the data sbuchunk ID
    file->read(file_data_id, sizeof(file_data_id) - 1);

    file_data_id[sizeof(file_data_id) - 1] = '\0';

    if (strncmp(file_data_id, "data", sizeof(file_data_id)) == 0) {
        std::cout << "data subchunk ID: Ok." << std::endl;

        strcpy(this->data_subchunk_id, file_data_id);
    } else {
        std::cerr << "Error: Invalid 'data' subchunk ID." << std::endl;

        exit(EXIT_FAILURE);
    }

    // Gets the data subchunk size
    file->read(reinterpret_cast<char *> (&file_data_size), sizeof(file_data_size));

    if (file_data_size >= 0) {
        std::cout << "data subchunk size: Ok (" << file_data_size << " bytes)." << std::endl;

        this->data_subchunk_size = file_data_size;
    } else {
        std::cerr << "Error: Bad 'data' subchunk size." << std::endl;

        exit(EXIT_FAILURE);
    }

    float duration = this->data_subchunk_size / this->byte_rate;

    this->audio_duration.minutes = (int) (duration / 60);
    this->audio_duration.seconds = (int) duration - (this->audio_duration.minutes * 60);
};

void WAVReader::load_data(std::shared_ptr<std::ifstream> file) {
    int current_file_chunk = 0;

    // Initializes audio buffer chunk size to be equal to the file byte rate
    this->audio_buffer_chunk_size = this->byte_rate;

    while (file->good() && !file->eof()) {
        if (!this->is_audio_buffer_ready || this->audio_buffer_chunks[current_file_chunk].is_written) {
            // Allocates memory for the buffered chunks
            this->audio_buffer_chunks[current_file_chunk].data = (BYTE *) malloc(this->audio_buffer_chunk_size);

            file->read(reinterpret_cast<char *> (this->audio_buffer_chunks[current_file_chunk].data),
                       this->audio_buffer_chunk_size);

            this->audio_buffer_chunks[current_file_chunk].size = file->gcount() * sizeof(BYTE);
            this->audio_buffer_chunks[current_file_chunk].is_written = FALSE;
            this->audio_buffer_chunks[current_file_chunk].is_eof = file->eof();

            if (!this->is_playback_started && (current_file_chunk == (MAX_AUDIO_BUFFER_CHUNKS - 1) || this->audio_buffer_chunks[current_file_chunk].is_eof)) {
                this->is_audio_buffer_ready = TRUE;
                this->cv.notify_one();
            }

            current_file_chunk += 1;

            if (current_file_chunk == MAX_AUDIO_BUFFER_CHUNKS) {
                current_file_chunk = 0;
            }
        } else {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    file->close();
}

bool WAVReader::get_chunk(BYTE **chunk, uint32_t &chunk_size) {
    bool stop = this->audio_buffer_chunks[this->current_audio_buffer_chunk].is_eof;
    std::unique_lock<std::mutex> lck(this->mtx);

    while (!this->is_audio_buffer_ready) {
        this->cv.wait(lck);
    }

    while (this->audio_buffer_chunks[this->current_audio_buffer_chunk].data == nullptr) {
        std::cout << "\nINFO: Writer blocked, waiting for new data to be loaded" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    if (this->is_playback_started == FALSE) {
        this->is_playback_started = TRUE;
    }

    *chunk = (BYTE *) malloc(this->audio_buffer_chunks[this->current_audio_buffer_chunk].size);
    chunk_size = this->audio_buffer_chunks[this->current_audio_buffer_chunk].size;

    memcpy(*chunk, this->audio_buffer_chunks[this->current_audio_buffer_chunk].data,
           this->audio_buffer_chunks[this->current_audio_buffer_chunk].size);
    free(this->audio_buffer_chunks[this->current_audio_buffer_chunk].data);

    this->audio_buffer_chunks[this->current_audio_buffer_chunk].is_written = TRUE;
    this->current_audio_buffer_chunk += 1;

    if (this->current_audio_buffer_chunk == MAX_AUDIO_BUFFER_CHUNKS) {
        // Reinitializes the current audio buffer chunk so it points to the first chunk of the audio buffer
        this->current_audio_buffer_chunk = 0;
    }

    return stop;
}
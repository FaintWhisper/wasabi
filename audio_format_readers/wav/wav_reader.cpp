#include "wav_reader.hpp"
#include <iostream>
#include <fstream>
#include <cstdint>
#include <windows.h>

WAVReader::WAVReader() {
}

WAVReader::~WAVReader() {
}

void WAVReader::load_file(const char path[]) {
    strcpy(file_path, path);

    if (file_path != NULL) {
        std::ifstream file(file_path, std::ios::in | std::ios::binary);

        if (file) {
            std::cout << "Loaded \"" << file_path << "\"" << std::endl;

            check_riff_header(&file);
            load_fmt_subchunk(&file);
            load_data_subchunk(&file);

            file.close();
        } else {
            std::cerr << "ERROR: The provided file path doesn't point to an existing file->" << std::endl;
        }
    } else {
        std::cerr << "ERROR: A file path must be provided" << std::endl;
    }
};

void WAVReader::check_riff_header(std::ifstream *file) {
    char chunk_id[4];
    uint32_t chunk_size;
    char format[4];

    std::cout << "\nChecking the file header..." << std::endl;

// Checks the chunk ID
    file->read(chunk_id, sizeof(chunk_id));

    chunk_id[sizeof(chunk_id)] = '\0';

    if (strcmp(chunk_id, "RIFF") == 0) {
        std::cout << "Chunk ID: Ok." << std::endl;
    } else {
        std::cerr << "ERROR: Invalid chunk ID." << std::endl;

        exit(EXIT_FAILURE);
    }

// Gets the chunk size (in bytes)
    file->read((char *) &chunk_size, sizeof(chunk_size));

    if (chunk_size >= 36) {
        std::cerr << "Chunk size: Ok (" << chunk_size << " bytes)." << std::endl;
    } else {
        std::cerr << "ERROR: Bad chunk size." << std::endl;

        exit(EXIT_FAILURE);
    }

// Checks the format descriptor
    file->read(format, sizeof(format));

    format[sizeof(format)] = '\0';

    if (strcmp(format, "WAVE") == 0) {
        std::cout << "Format descriptor: Ok." << std::endl;
    } else {
        std::cerr << "ERROR: Invalid format descriptor." << std::endl;

        exit(EXIT_FAILURE);
    }

    file->tellg();
};

void WAVReader::load_fmt_subchunk(std::ifstream *file) {
    char fmt_subchunk_id[4];
    uint32_t fmt_subchunk_size;
    uint16_t fmt_subchunk_audio_format;
    uint16_t fmt_subchunk_num_channels;
    uint32_t fmt_subchunk_sample_rate;
    uint32_t fmt_subchunk_byte_rate;
    uint16_t fmt_subchunk_block_align;
    uint16_t fmt_subchunk_bits_per_sample;

    std::cout << "\nReading the 'fmt' subchunk..." << std::endl;

// Checks the fmt sbuchunk ID
    file->read(fmt_subchunk_id, sizeof(fmt_subchunk_id));

// Null character added replacing the last byte because it's not used
    fmt_subchunk_id[sizeof(fmt_subchunk_id) - 1] = '\0';

    if (strcmp(fmt_subchunk_id, "fmt") == 0) {
        std::cout << "fmt subchunk ID: Ok." << std::endl;
    } else {
        std::cout << "Error: Invalid 'fmt' subchunk ID." << std::endl;

        exit(EXIT_FAILURE);
    }

// Gets the fmt subchunk size and checks the audio encoding
    file->read(reinterpret_cast<char *> (&fmt_subchunk_size), sizeof(fmt_subchunk_size));

    if (fmt_subchunk_size == 16) {
        std::cerr << "fmt subchunk size: Ok (" << fmt_subchunk_size << " bytes)." << std::endl;
    } else {
        std::cout << "Error: Bad encoding, only PCM encoded WAV audio files are currently supported." << std::endl;

        exit(EXIT_FAILURE);
    }

// Checks the audio format
    file->read(reinterpret_cast<char *> (&fmt_subchunk_audio_format), sizeof(fmt_subchunk_audio_format));

    if (fmt_subchunk_audio_format == 1) {
        std::cerr << "Audio format: Ok." << std::endl;
    } else {
        std::cout << "Error: Bad audio format, only linear PCM encoded WAV audio files are currently supported."
                  << std::endl;

        exit(EXIT_FAILURE);
    }

// Gets the number of channels
    file->read(reinterpret_cast<char *> (&fmt_subchunk_num_channels), sizeof(fmt_subchunk_num_channels));

    if (fmt_subchunk_num_channels == 1) {
        std::cerr << "Number of channels: Ok (mono)" << std::endl;
    } else if (fmt_subchunk_num_channels == 2) {
        std::cerr << "Number of channels: Ok (stereo)" << std::endl;
    } else {
        std::cout << "Error: Bad number of channels, only mono and stereo audio files are currently supported."
                  << std::endl;

        exit(EXIT_FAILURE);
    }

// Gets the sample rate
    file->read(reinterpret_cast<char *> (&fmt_subchunk_sample_rate), sizeof(fmt_subchunk_sample_rate));

    if (fmt_subchunk_sample_rate == 22050 || fmt_subchunk_sample_rate == 44100) {
        std::cerr << "Sample rate: Ok (" << fmt_subchunk_sample_rate << " Hz)." << std::endl;
    } else {
        std::cout << "Error: Bad sample rate, only audio files sampled at 22050 or 44100 Hz are currently supported."
                  << std::endl;

        exit(EXIT_FAILURE);
    }

// Gets the byte rate
    file->read(reinterpret_cast<char *> (&fmt_subchunk_byte_rate), sizeof(fmt_subchunk_byte_rate));

// Gets the block alignment
    file->read(reinterpret_cast<char *> (&fmt_subchunk_block_align), sizeof(fmt_subchunk_block_align));

// Gets the size of each sample (in bits)
    file->read(reinterpret_cast<char *> (&fmt_subchunk_bits_per_sample), sizeof(fmt_subchunk_bits_per_sample));

// Checks the byte rate
    if (fmt_subchunk_byte_rate ==
        (fmt_subchunk_sample_rate * fmt_subchunk_num_channels * (fmt_subchunk_bits_per_sample / 8))) {
        std::cerr << "Byte rate: Ok (" << fmt_subchunk_byte_rate << " bytes)." << std::endl;
    } else {
        std::cout << "Error: Bad byte rate" << std::endl;

        exit(EXIT_FAILURE);
    }

// Checks the block alignment
    if (fmt_subchunk_block_align == (fmt_subchunk_num_channels * (fmt_subchunk_bits_per_sample / 8))) {
        std::cerr << "Block alignment: Ok (" << fmt_subchunk_block_align << " bytes)." << std::endl;
    } else {
        std::cout << "Error: Bad block alignment" << std::endl;

        exit(EXIT_FAILURE);
    }

    // Checks the bit depth
    if (fmt_subchunk_bits_per_sample == 24 || fmt_subchunk_bits_per_sample == 32) {
        std::cerr << "Bit Depth: Ok (" << fmt_subchunk_bits_per_sample << " bits)." << std::endl;
    } else {
        std::cout << "Error: Unsupported bit depth" << std::endl;

        exit(EXIT_FAILURE);
    }
};

void WAVReader::load_data_subchunk(std::ifstream *file) {
    char data_subchunk_id[4];
    uint32_t data_subchunk_size;

    std::cout << "\nReading the 'data' subchunk..." << std::endl;

    data_subchunk_id[sizeof(data_subchunk_id)] = '\0';

// Checks the data sbuchunk ID
    file->read(data_subchunk_id, sizeof(data_subchunk_id));

    if (strcmp(data_subchunk_id, "data") == 0) {
        std::cout << "data subchunk ID: Ok." << std::endl;
    } else {
        std::cout << "Error: Invalid 'data' subchunk ID." << std::endl;

        exit(EXIT_FAILURE);
    }

// Gets the data subchunk size
    file->read(reinterpret_cast<char *> (&data_subchunk_size), sizeof(data_subchunk_size));

    if (data_subchunk_size >= 0) {
        std::cerr << "data subchunk size: Ok (" << data_subchunk_size << " bytes)." << std::endl;
    } else {
        std::cout << "Error: Bad 'data' subchunk size." << std::endl;

        exit(EXIT_FAILURE);
    }

    float buffer[data_subchunk_size];

    file->read(reinterpret_cast<char *> (buffer), (data_subchunk_size / 8) * sizeof(float));

    this->audio_buffer = buffer;
    this->audio_buffer_size = data_subchunk_size;
};

bool WAVReader::write_data(BYTE *dst_audio_buffer, int num_buffer_frames, WAVEFORMATEX *format) {
    int buffer_size = num_buffer_frames * format->nChannels * (format->wBitsPerSample / 8);

    std::cout << "Buffer Size: " << buffer_size << std::endl;

    if (this->audio_buffer_size < buffer_size) {
        buffer_size = this->audio_buffer_size;
    }

    memcpy(dst_audio_buffer, this->audio_buffer, buffer_size);

    this->audio_buffer += buffer_size;
    this->audio_buffer_size -= buffer_size;

    return this->audio_buffer_size <= 0;
};
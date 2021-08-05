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
            std::cout << "\nLoaded \"" << file_path << "\"" << std::flush;

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
    char file_chunk_id[4];
    uint32_t file_chunk_size;
    char file_format[4];

    std::cout << "\nChecking the file header..." << std::endl;

// Checks the chunk ID
    file->read(file_chunk_id, sizeof(file_chunk_id));

    file_chunk_id[sizeof(file_chunk_id)] = '\0';

    if (strcmp(file_chunk_id, "RIFF") == 0) {
        std::cout << "Chunk ID: Ok." << std::endl;
        
        strcpy(this->chunk_id, file_chunk_id);
    } else {
        std::cerr << "ERROR: Invalid chunk ID." << std::endl;

        exit(EXIT_FAILURE);
    }

// Gets the chunk size (in bytes)
    file->read((char *) &file_chunk_size, sizeof(file_chunk_size));

    if (file_chunk_size >= 36) {
        std::cerr << "Chunk size: Ok (" << file_chunk_size << " bytes)." << std::endl;
        
        this->chunk_size = file_chunk_size;
    } else {
        std::cerr << "ERROR: Bad chunk size." << std::endl;

        exit(EXIT_FAILURE);
    }

// Checks the format descriptor
    file->read(file_format, sizeof(file_format));

    file_format[sizeof(file_format)] = '\0';

    if (strcmp(file_format, "WAVE") == 0) {
        std::cout << "Format descriptor: Ok." << std::endl;

        strcpy(this->format_descriptor, file_format);
    } else {
        std::cerr << "ERROR: Invalid format descriptor." << std::endl;

        exit(EXIT_FAILURE);
    }

    file->tellg();
};

void WAVReader::load_fmt_subchunk(std::ifstream *file) {
    char file_fmt_id[4];
    uint32_t file_fmt_size;
    uint16_t file_fmt_audio_format;
    uint16_t file_fmt_num_channels;
    uint32_t file_fmt_sample_rate;
    uint32_t file_fmt_byte_rate;
    uint16_t file_fmt_block_align;
    uint16_t file_fmt_bit_depth;

    std::cout << "\nReading the 'fmt' subchunk..." << std::endl;

// Checks the fmt subchunk ID
    file->read(file_fmt_id, sizeof(file_fmt_id));

// Null character added replacing the last byte because it's not used
    file_fmt_id[sizeof(file_fmt_id) - 1] = '\0';

    if (strcmp(file_fmt_id, "fmt") == 0) {
        std::cout << "fmt subchunk ID: Ok." << std::endl;
        
        strcpy(this->chunk_id, file_fmt_id);
    } else {
        std::cout << "Error: Invalid fmt subchunk ID." << std::endl;

        exit(EXIT_FAILURE);
    }

// Gets the fmt subchunk size and checks the audio encoding
    file->read(reinterpret_cast<char *> (&file_fmt_size), sizeof(file_fmt_size));

    if (file_fmt_size == 16) {
        std::cerr << "fmt subchunk size: Ok (" << file_fmt_size << " bytes)." << std::endl;
        
        this->fmt_subchunk_size = file_fmt_size;
    } else {
        std::cout << "Error: Bad encoding, only PCM encoded WAV audio files are currently supported." << std::endl;

        exit(EXIT_FAILURE);
    }

// Checks the audio format
    file->read(reinterpret_cast<char *> (&file_fmt_audio_format), sizeof(file_fmt_audio_format));

    if (file_fmt_audio_format == 1) {
        std::cerr << "Audio format: Ok." << std::endl;
        
        this->audio_format = file_fmt_audio_format;
    } else {
        std::cout << "Error: Bad audio format, only linear PCM encoded WAV audio files are currently supported."
                  << std::endl;

        exit(EXIT_FAILURE);
    }

// Gets the number of channels
    file->read(reinterpret_cast<char *> (&file_fmt_num_channels), sizeof(file_fmt_num_channels));

    if (file_fmt_num_channels == 1 || file_fmt_num_channels == 2) {
        if (file_fmt_num_channels == 1) {
            std::cerr << "Number of channels: 1 (mono)" << std::endl;
        } else {
            std::cerr << "Number of channels: 2 (stereo)" << std::endl;
        }

        this->num_channels = file_fmt_num_channels;
    } else {
        std::cout << "Error: Bad number of channels, only mono and stereo audio files are currently supported."
                  << std::endl;

        exit(EXIT_FAILURE);
    }

// Gets the sample rate
    file->read(reinterpret_cast<char *> (&file_fmt_sample_rate), sizeof(file_fmt_sample_rate));

    if (file_fmt_sample_rate == 44100 || file_fmt_sample_rate == 48000) {
        std::cerr << "Sample rate: Ok (" << file_fmt_sample_rate << " Hz)." << std::endl;
        
        this->sample_rate = file_fmt_sample_rate;
    } else {
        std::cout << "Error: Bad sample rate, only audio files sampled at 44100 or 48000 Hz are currently supported."
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
        std::cerr << "Byte rate: Ok (" << file_fmt_byte_rate << " bytes)." << std::endl;
        
        this->byte_rate = file_fmt_byte_rate;
    } else {
        std::cout << "Error: Bad byte rate" << std::endl;

        exit(EXIT_FAILURE);
    }

// Checks the block alignment
    if (file_fmt_block_align == (file_fmt_num_channels * (file_fmt_bit_depth / 8))) {
        std::cerr << "Block alignment: Ok (" << file_fmt_block_align << " bytes)." << std::endl;
        
        this->byte_rate = file_fmt_byte_rate;
    } else {
        std::cout << "Error: Bad block alignment" << std::endl;

        exit(EXIT_FAILURE);
    }

    // Checks the bit depth
    if (file_fmt_bit_depth % 8 == 0) {
        std::cerr << "Bit Depth: Ok (" << file_fmt_bit_depth << " bits)." << std::endl;
        
        this->bit_depth = file_fmt_bit_depth;
    } else {
        std::cout << "Error: Invalid bit depth" << std::endl;

        exit(EXIT_FAILURE);
    }
};

void WAVReader::load_data_subchunk(std::ifstream *file) {
    char file_data_id[4];
    uint32_t file_data_size;

    std::cout << "\nReading the 'data' subchunk..." << std::endl;

    file_data_id[sizeof(file_data_id)] = '\0';

// Checks the data sbuchunk ID
    file->read(file_data_id, sizeof(file_data_id));

    if (strcmp(file_data_id, "data") == 0) {
        std::cout << "data subchunk ID: Ok." << std::endl;

        strcpy(this->data_subchunk_id, file_data_id);
    } else {
        std::cout << "Error: Invalid 'data' subchunk ID." << std::endl;

        exit(EXIT_FAILURE);
    }

// Gets the data subchunk size
    file->read(reinterpret_cast<char *> (&file_data_size), sizeof(file_data_size));

    if (file_data_size >= 0) {
        std::cerr << "data subchunk size: Ok (" << file_data_size << " bytes)." << std::endl;
        
        this->data_subchunk_size = file_data_size;
        this->audio_buffer_size = file_data_size; //TODO: In the future this will not be the case, the audio will be loaded in chunks.
    } else {
        std::cout << "Error: Bad 'data' subchunk size." << std::endl;

        exit(EXIT_FAILURE);
    }

    BYTE* buffer = (BYTE*) malloc(file_data_size);

    file->read(reinterpret_cast<char *> (buffer), file_data_size);

    this->audio_buffer = buffer;
};

bool WAVReader::write_data(BYTE *dst_audio_buffer, int num_buffer_frames, WAVEFORMATEX *format) {
    if (this->audio_buffer_size > 0) {
        int buffer_size = num_buffer_frames * format->nChannels * (format->wBitsPerSample / 8);

        if (this->audio_buffer_size < buffer_size) {
            buffer_size = this->audio_buffer_size;
        }

        memcpy(dst_audio_buffer, this->audio_buffer, buffer_size);

        this->audio_buffer += buffer_size;
        this->audio_buffer_size -= buffer_size;

        return FALSE;
    } else
        return TRUE;
};
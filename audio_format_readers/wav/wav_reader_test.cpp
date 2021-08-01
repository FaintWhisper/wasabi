#include <windows.h>
#include "wav_reader.hpp"

#ifndef MAX_FILE_PATH_SIZE
#define MAX_FILE_PATH_SIZE 40
#endif

BOOL main(const int argc, const char *argv[]) {
    WAVReader wav_reader;
    char file_path[MAX_FILE_PATH_SIZE];

    strcpy(file_path, argv[1]);

    wav_reader.load_file(file_path);
}
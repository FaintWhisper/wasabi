cmake_minimum_required(VERSION 3.19)
project(wasabi)

set(CMAKE_CXX_STANDARD 14)

set(WAV_FORMAT_READER ./audio_format_readers/wav)

#Include the directories and now your cpp files will recognize your headers
include_directories(${WAV_FORMAT_READER})

set(
        SOURCE_FILES
        main.cpp
        ${WAV_FORMAT_READER}/wav_reader.hpp
        ${WAV_FORMAT_READER}/wav_reader.cpp
)

add_executable(wasabi ${SOURCE_FILES})
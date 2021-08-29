cmake_minimum_required(VERSION 3.19)
project(wasabi)

set(CMAKE_CXX_STANDARD 14)

set(WAV_FORMAT_READER audio_format_readers/wav)
set(WASAPI audio_protocols/wasapi)

#Include the directories and now your cpp files will recognize your headers
include_directories(${WAV_FORMAT_READER})
include_directories(${WASAPI})

set(
        SOURCE_FILES
        main.cpp
        ${WAV_FORMAT_READER}/wav_reader.hpp
        ${WAV_FORMAT_READER}/wav_reader.cpp
        ${WASAPI}/wasapi.hpp
        ${WASAPI}/wasapi.cpp)

add_executable(wasabi ${SOURCE_FILES})
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "wasabi")
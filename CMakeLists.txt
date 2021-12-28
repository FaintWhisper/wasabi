cmake_minimum_required(VERSION 3.19)
project(wasabi)

set(CMAKE_CXX_STANDARD 14)

set(AUDIO_FORMAT_READERS audio_format_readers)
set(WAV_FORMAT_READER ${AUDIO_FORMAT_READERS}/wav)
set(AUDIO_PROTOCOLS audio_protocols)
set(WASAPI ${AUDIO_PROTOCOLS}/wasapi)
set(PLAYER player)

include_directories(${WAV_FORMAT_READER})
include_directories(${WASAPI})
include_directories(${PLAYER})

set(
        SOURCE_FILES
        ${WAV_FORMAT_READER}/wav_reader.hpp
        ${WAV_FORMAT_READER}/wav_reader.cpp
        ${PLAYER}/player.hpp
        ${PLAYER}/player.cpp
        ${WASAPI}/wasapi.hpp
        ${WASAPI}/wasapi.cpp
        "wasabi.cpp"
)

add_executable(wasabi ${SOURCE_FILES})
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "wasabi")
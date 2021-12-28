#ifndef PLAYER_HPP
#define PLAYER_HPP

#include "wasapi.hpp"
#include "wav_reader.hpp"

class Player {
private:
	void clean_line(int num_chars);
public:
	Player();
	~Player();
	void play_audio_stream(std::string file_path, int rendering_endpoint_buffer_duration);
};

#endif //PLAYER_HPP
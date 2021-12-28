#include "player.hpp"

Player::Player() = default;

Player::~Player() = default;

void Player::clean_line(int num_chars) {
	char blank_str[16];

	sprintf(blank_str, "\r%%%is\r", num_chars);
	printf(blank_str, "");
}

void Player::play_audio_stream(std::string file_path, int rendering_endpoint_buffer_duration) {
	// Declares variables to control the playback
	bool stop = FALSE;
	bool playing = FALSE;

	// Instantiates a wasapi object
	WASAPI wasapi = WASAPI(rendering_endpoint_buffer_duration);

	// Set audio session volume to the half of the current system volume
	double volume = 0.5;
	wasapi.set_volume(volume);

	std::cout << "Rendering endpoint buffer duration: " << rendering_endpoint_buffer_duration * 1000 << " ms"
		<< std::endl;
	std::cout << "Internal buffer duration: " << rendering_endpoint_buffer_duration * 5000 << " ms" << std::endl;

	// Instantiates a wav format reader object
	WAVReader wav_reader = WAVReader();
	wav_reader.load_file(&file_path);

	// Declares and initializes the variables that will keep track of the playing time
	int current_minutes = 0;
	int current_seconds = 0;
	int current_milliseconds = 0;

	// Declares and initializes the playback cycle in milliseconds
	int cycle_duration = 100;

	// Declares the variables that will hold the chunk of audio data that will be written to the rendering endpoint buffer
	BYTE* chunk = nullptr;
	uint32_t chunk_size;

	// Declares the variables that will store the status of some specific keys that are used to control the playback
	SHORT space_key = 0x0;
	SHORT up_key = 0x0;
	SHORT down_key = 0x0;

	// Declares the variables that will control the playback
	bool is_paused = FALSE;
	bool is_window_focused = FALSE;

	// Declares and initializes a variable that will hold the number of characters written to the console when updating the playback information
	int num_chars_written = 0;

	// Declares the variable that will store the playback information
	char* playback_status = (char*)malloc(42 * sizeof(char));

	CONSOLE_SCREEN_BUFFER_INFO info;
	COORD volume_cursor_position, current_time_cursor_position;

	while (stop == FALSE) {
		if (!is_paused) {
			if ((current_milliseconds + 1000) % (rendering_endpoint_buffer_duration * 1000) == 0) {
				// Load the audio data chunk in the rendering endpoint buffer
				stop = wav_reader.get_chunk(&chunk, chunk_size);

				// Write the audio data chunk in the rendering endpoint buffer
				wasapi.write_chunk(chunk, chunk_size, stop);

				if (playing == FALSE) {
					std::cout << std::endl << "[Starting to play the file]" << std::endl;
					printf("Volume: %.1f\n", volume);

					GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
					volume_cursor_position.X = 0;
					volume_cursor_position.Y = info.dwCursorPosition.Y - 1;

					current_time_cursor_position.X = 0;
					current_time_cursor_position.Y = volume_cursor_position.Y + 2;

					printf("Audio duration: %dm %.2ds\n", wav_reader.audio_duration.minutes, wav_reader.audio_duration.seconds);

					wasapi.start();

					playing = TRUE;
				}
			}

			// Print the playback information
			sprintf(playback_status, "\rCurrent time: %dm %.2ds", current_minutes, current_seconds);
			printf(playback_status, 33);
			fflush(stdout);

			if (current_milliseconds == 1000) {
				current_seconds += 1;
				current_milliseconds = 0;

				if (current_seconds == 60) {
					current_minutes += 1;
					current_seconds = 0;
				}
			}

			current_milliseconds += cycle_duration;
		}

		// Check if a key was pressed
		is_window_focused = (GetConsoleWindow() == GetForegroundWindow());

		if (is_window_focused) {
			space_key = GetAsyncKeyState(VK_SPACE);
			up_key = GetAsyncKeyState(VK_UP);
			down_key = GetAsyncKeyState(VK_DOWN);
		}

		if (space_key & 0x01) {
			if (is_paused == FALSE) {
				wasapi.stop();

				sprintf(playback_status + strlen(playback_status), " [PAUSED]", current_minutes, current_seconds);
				num_chars_written = printf(playback_status);
				fflush(stdout);

				is_paused = TRUE;
			}
			else {
				wasapi.start();

				clean_line(num_chars_written);
				sprintf(playback_status, "\rCurrent time: %dm %.2ds", current_minutes, current_seconds);
				printf(playback_status, 33);
				fflush(stdout);

				is_paused = FALSE;
			}
		}

		if (up_key & 0x01 && volume < 1.0) {
			volume += 0.1;

			if (volume > 1.0) {
				volume = 1.0;
			}

			wasapi.set_volume(volume);

			SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), volume_cursor_position);
			printf("Volume: %.1f\n", volume);
			SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), current_time_cursor_position);
		}

		if (down_key & 0x01 && volume > 0.0) {
			volume -= 0.1;

			if (volume < 0.0) {
				volume = 0.0;
			}

			wasapi.set_volume(volume);

			SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), volume_cursor_position);
			printf("Volume: %.1f\n", volume);
			SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), current_time_cursor_position);
		}

		Sleep(cycle_duration);
	}

	wasapi.stop();
}
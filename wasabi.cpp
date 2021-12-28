#include "player.hpp"
#include <iostream>

void parse_args(int argc, char* argv[], std::string* file_path, int* rendering_endpoint_buffer_duration) {
	// Checks if all parameters are provided, if not, initializes all required but non defined parameters with their default values
	int file_pos = -1;
	int rendering_endpoint_buffer_duration_pos = -1;

	for (int i = 0; i < argc; i++) {
		if (strcmp(argv[i], "--file") == TRUE) {
			if ((i + 1) < (argc - 1)) {
				file_pos = i + 1;
			}
		}
		else if (strcmp(argv[i], "--rendering_endpoint_buffer_duration") == TRUE) {
			if ((i + 1) < (argc - 1)) {
				rendering_endpoint_buffer_duration_pos = i + 1;
			}
		}
	}

	if (file_pos != -1) {
		*file_path = argv[file_pos];
	}
	else {
		std::string input_file_path;

		std::cout << "Input file: ";
		std::getline(std::cin, input_file_path);

		*file_path = input_file_path;
	}

	if (rendering_endpoint_buffer_duration_pos != -1) {
		*rendering_endpoint_buffer_duration = strtol(argv[rendering_endpoint_buffer_duration_pos], nullptr, 10);
	}
	else {
		*rendering_endpoint_buffer_duration = 1;
	}
}

void block_std_input() {
	// Block standard input to be shown in the console
	HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
	DWORD mode;

	if (GetConsoleMode(h, &mode)) {
		mode &= ~ENABLE_ECHO_INPUT;

		SetConsoleMode(h, mode);
	}
}

void hide_console_cursor() {
	HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_CURSOR_INFO info;

	info.dwSize = 100;
	info.bVisible = FALSE;

	SetConsoleCursorInfo(consoleHandle, &info);
}



int main(int argc, char* argv[]) {
	std::string file;
	int rendering_endpoint_buffer_duration;

	parse_args(argc, argv, &file, &rendering_endpoint_buffer_duration);
	block_std_input();
	hide_console_cursor();

	Player player = Player();
	player.play_audio_stream(file, rendering_endpoint_buffer_duration);

	return 0;
}
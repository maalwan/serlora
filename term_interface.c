#include "term_interface.h"

// Function to clear the line
void clear_line() {
	printf("\033[2K"); // Clear the entire line
	printf("\r");      // Move the cursor to the beginning of the line
}

// Function to display the current command line
void display_command_line(const char *command_line, int cursor_position) {
	clear_line();
	printf("Wio-E5$ %s", command_line);
	printf("\033[%dG", cursor_position + 9); // Move cursor to current position
	fflush(stdout); // Flush the output buffer
}

// Function used to display terminal UI for user
int term_interface(int (*callback)(int, char**, void*), void* ptr) {
    struct termios old, new;
    tcgetattr(STDIN_FILENO, &old);
    new = old;
    new.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new);

    char command_history[MAX_HISTORY_SIZE][MAX_COMMAND_LENGTH] = {0};
    char command_line[MAX_COMMAND_LENGTH] = {0};
    int history_size = 0;
    int history_index = 0;
    int curr_history_index = 0;
    int current_index = 0;
    int cursor_position = 0;
    int ch;
    display_command_line(command_line, cursor_position);
    while ((ch = getchar()) != 'q') {
        if (ch == '\033') { // Check for escape sequence (arrow keys)
            getchar();
            int arrow_key = getchar();
            if (arrow_key == 'A' && history_size != 0) { // Up arrow key
                // Display previous command
                clear_line();
                curr_history_index = (curr_history_index - 1 + history_size) % history_size;
                strncpy(command_line, command_history[curr_history_index], MAX_COMMAND_LENGTH);
                current_index = strlen(command_line);
                cursor_position = current_index;
                display_command_line(command_line, cursor_position);
            } else if (arrow_key == 'B' && history_size != 0) { // Down arrow key
                // Display next command
                clear_line();
                curr_history_index = (curr_history_index + 1) % history_size;
                strncpy(command_line, command_history[curr_history_index], MAX_COMMAND_LENGTH);
                current_index = strlen(command_line);
                cursor_position = current_index;
                display_command_line(command_line, current_index);
            } else if (arrow_key == 'C' && cursor_position < current_index) { // Right arrow key
                // Move cursor to the right
                cursor_position++;
                display_command_line(command_line, cursor_position);
            } else if (arrow_key == 'D' && cursor_position > 0) { // Left arrow key
                // Move cursor to the left
                cursor_position--;
                display_command_line(command_line, cursor_position);
            }
        } else if (ch == '\n') { // Enter key
            // Output           
            char out[MAX_COMMAND_LENGTH];
            int len = current_index + 1;
            strncpy(out, command_line, MAX_COMMAND_LENGTH);
            out[len - 1] = '\0';
            // Store command in history
            strncpy(command_history[history_index], command_line, MAX_COMMAND_LENGTH);
            history_index = (history_index + 1) % MAX_HISTORY_SIZE;
            history_size = history_size < MAX_HISTORY_SIZE ? (history_size + 1) : MAX_HISTORY_SIZE;
            // Clear command line for next input
            memset(command_line, 0, MAX_COMMAND_LENGTH);
            putc('\n', stdout);
            // Reset current index for new input
            cursor_position = 0;
            current_index = 0;
            curr_history_index = history_index;
            // Call callback by first tokenizing
			int argc = 0;
			char* tok = strtok(out, " ");
			while (tok != NULL) {
				argc++;
				tok = strtok(NULL, " ");
			}
			char* argv[argc];
			char* curr = out;
			int idx = 0;
			int j = 0;
			while (idx < argc) {
				if (out[j] == '\0') {
					argv[idx++] = curr;
					curr = out + j + 1;
				}
				j++; 
			}
            if (callback(argc, argv, ptr) < 0) { return -1; }
            // Reset terminal
            display_command_line(command_line, cursor_position);
        } else if (ch == 127) { // Backspace key
            // Handle backspace to delete characters from the command line
            if (current_index > 0) {
                memmove(&command_line[cursor_position - 1],
                        &command_line[cursor_position],
                        MAX_COMMAND_LENGTH - cursor_position);
                cursor_position--;
                current_index--;
                display_command_line(command_line, cursor_position);
            }
        } else if (ch >= 32 && ch <= 126) { // Printable ASCII characters
            // Add printable characters to the command line
            if (current_index < MAX_COMMAND_LENGTH - 1) {
                memmove(&command_line[cursor_position + 1],
                        &command_line[cursor_position],
                        MAX_COMMAND_LENGTH - cursor_position - 1);
                command_line[cursor_position++] = ch;
                current_index++;
                display_command_line(command_line, cursor_position);
                curr_history_index = history_index;
            }
        }
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &old);
    putc('\n', stdout);
	
	return 0;
}

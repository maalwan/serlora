#include "term_interface.h"
#include "wioe.h"

typedef struct {
    int (*callback)(int, char**, void*);
    void* callback_ptr;
    int cursor_position;
    char command_line[MAX_COMMAND_LENGTH];
    int complete;
    pthread_mutex_t lock;
} term_args;

struct term {
    term_args* data;
    pthread_t term_thread;
};

// Function to clear the line
static void clear_line() {
	printf("\033[2K"); // Clear the entire line
	printf("\r");      // Move the cursor to the beginning of the line
}

// Function to display the current command line
static void display_command_line(const char* command_line, int cursor_position) {
	clear_line();
	printf("\033[1;34m~$\033[0m %s", command_line);
	printf("\033[%dG", cursor_position + 4); // Move cursor to current position
	fflush(stdout); // Flush the output buffer
}

void* backend_term(void* args) {
    term_args* data = (term_args*) args;
    // Setting up terminal
    struct termios old, new;
    tcgetattr(STDIN_FILENO, &old);
    new = old;
    new.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new);
    // Setting up data for terminal interface
    char command_history[MAX_HISTORY_SIZE][MAX_COMMAND_LENGTH] = {0};
    //char command_line[MAX_COMMAND_LENGTH] = {0};
    int history_size = 0;
    int history_index = 0;
    int curr_history_index = 0;
    int current_index = 0;
    //int cursor_position = 0;
    int ch;
    display_command_line(data->command_line, data->cursor_position);
    while ((ch = getchar()) != 27) {
        pthread_mutex_lock(&data->lock);
        if (ch == '\033') { // Check for escape sequence (arrow keys)
            getchar();
            int arrow_key = getchar();
            if (arrow_key == 'A' && history_size != 0) { // Up arrow key
                // Display previous command
                clear_line();
                curr_history_index = (curr_history_index - 1 + history_size) % history_size;
                strncpy(data->command_line, command_history[curr_history_index], MAX_COMMAND_LENGTH);
                current_index = strlen(data->command_line);
                data->cursor_position = current_index;
                display_command_line(data->command_line, data->cursor_position);
            } else if (arrow_key == 'B' && history_size != 0) { // Down arrow key
                // Display next command
                clear_line();
                curr_history_index = (curr_history_index + 1) % history_size;
                strncpy(data->command_line, command_history[curr_history_index], MAX_COMMAND_LENGTH);
                current_index = strlen(data->command_line);
                data->cursor_position = current_index;
                display_command_line(data->command_line, current_index);
            } else if (arrow_key == 'C' && data->cursor_position < current_index) { // Right arrow key
                // Move cursor to the right
                data->cursor_position++;
                display_command_line(data->command_line, data->cursor_position);
            } else if (arrow_key == 'D' && data->cursor_position > 0) { // Left arrow key
                // Move cursor to the left
                data->cursor_position--;
                display_command_line(data->command_line, data->cursor_position);
            }
        } else if (ch == '\n') { // Enter key
            // Output           
            char out[MAX_COMMAND_LENGTH];
            int len = current_index + 1;
            strncpy(out, data->command_line, MAX_COMMAND_LENGTH);
            out[len - 1] = '\0';
            // Store command in history
            strncpy(command_history[history_index], data->command_line, MAX_COMMAND_LENGTH);
            history_index = (history_index + 1) % MAX_HISTORY_SIZE;
            history_size = history_size < MAX_HISTORY_SIZE ? (history_size + 1) : MAX_HISTORY_SIZE;
            // Clear command line for next input
            memset(data->command_line, 0, MAX_COMMAND_LENGTH);
            putc('\n', stdout);
            // Reset current index for new input
            data->cursor_position = 0;
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
            if (data->callback(argc, argv, data->callback_ptr) < 0) { return (void*) -1; }
            // Reset terminal
            display_command_line(data->command_line, data->cursor_position);
        } else if (ch == 127) { // Backspace key
            // Handle backspace to delete characters from the command line
            if (current_index > 0) {
                memmove(&data->command_line[data->cursor_position - 1],
                        &data->command_line[data->cursor_position],
                        MAX_COMMAND_LENGTH - data->cursor_position);
                data->cursor_position--;
                current_index--;
                display_command_line(data->command_line, data->cursor_position);
            }
        } else if (ch >= 32 && ch <= 126) { // Printable ASCII characters
            // Add printable characters to the command line
            if (current_index < MAX_COMMAND_LENGTH - 1) {
                memmove(&data->command_line[data->cursor_position + 1],
                        &data->command_line[data->cursor_position],
                        MAX_COMMAND_LENGTH - data->cursor_position - 1);
                data->command_line[data->cursor_position++] = ch;
                current_index++;
                display_command_line(data->command_line, data->cursor_position);
                curr_history_index = history_index;
            }
        }
        pthread_mutex_unlock(&data->lock);
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &old);
    putc('\n', stdout);
    data->complete = 1;
    wioe_cancel_recieve((wioe*) data->callback_ptr);
    return (void*) 0;
}

// Function used to display terminal UI for user
int term_interface(int (*callback)(int, char**, void*), void* ptr) {
    // Initilize args and shared command_line
    term_args* data = malloc(sizeof(term_args));
    data->callback = callback;
    data->callback_ptr = ptr;
    data->complete = 0;
    memset(data->command_line, 0, sizeof(data->command_line));
    pthread_mutex_init(&data->lock, NULL);
    void* ret = backend_term((void*) data);
    pthread_mutex_destroy(&data->lock);
    return (int) (long) ret;
}

// Function used to display terminal UI for user but async
term* term_interface_async(int (*callback)(int, char**, void*), void* ptr) {
    // Initilize args and shared command_line
    term_args* data = malloc(sizeof(term_args));
    data->callback = callback;
    data->callback_ptr = ptr;
    memset(data->command_line, 0, sizeof(data->command_line));
    pthread_mutex_init(&data->lock, NULL);
    // Initilize term struct to be handed over
    term* info = malloc(sizeof(term));
    info->data = data;
    pthread_create(&info->term_thread, NULL, backend_term, (void*) data);
    return info;
}

void term_print(term* info, char* str) {
    clear_line();
    printf("%s\n", str);
    pthread_mutex_lock(&info->data->lock);
    display_command_line(info->data->command_line, info->data->cursor_position);
    pthread_mutex_unlock(&info->data->lock);
}

int term_join(term* info) {
    void* ret;
    pthread_join(info->term_thread, (void**) &ret);
    pthread_mutex_destroy(&(info->data->lock));
    free(info->data);
    free(info);
    return (int) (long) ret;
}

int term_is_complete(term* info) {
    return info->data->complete;
}
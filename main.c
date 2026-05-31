#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <pthread.h>
#include <termios.h>
#include <unistd.h>
#include <stdbool.h>

// Defines for map width & height
#define MAP_W 8
#define MAP_H 8

// Point struct for storing 2D data
typedef struct {
	int8_t x;
	int8_t y;
} POINT;

// Mutex block for communication between threads
typedef struct {
	pthread_mutex_t mutex; // Mutex itself
	uint8_t dir; // Move direction
	uint8_t move; // Last move direction
	bool run; // Run flag
	bool game_over; // Gameover flag
	bool restart; // Restart flag
} MUTEX_DIR;

// Random generator for point on map (excluding snake points)
POINT rand_point(POINT *snake, int snake_tail) {
	POINT p = (POINT){rand() % MAP_W, rand() % MAP_H};
	for(int i = 0; i < snake_tail; ++i)
		if(snake[i].x == p.x && snake[i].y == p.y) {
			p = (POINT){rand() % MAP_W, rand() % MAP_H}; // Generate new
			i = -1; // Restart point loop
		}
	return p;
}

// IGK what is this but it is necessary for keyboard handling
void enable_raw_mode() {
	struct termios term;
	tcgetattr(STDIN_FILENO, &term);
	term.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

// Keyboard handling function for thread
void* key_handler(void* p) {
	enable_raw_mode(); // ts that AI recomended
	MUTEX_DIR *md = p; // Casting void* to MUTEX_DIR*
	// Main thread loop
	while(md->run) {
		char c;
		read(STDIN_FILENO, &c, 1); // Reading chat from input
		pthread_mutex_lock(&md->mutex); // Locking fields

		// Exit button
		if(c == 'q' || c == 'Q') {
			md->run = false;
		}
		// Restart button (if game is over)
		else if(md->game_over) {
			if(c == 'r' || c == 'R')
				md->restart = true;
		}
		// WASD
		else if((c == 'a' || c == 'A') && md->move != 2) {
			md->dir = 1;
		}
		else if((c == 'd' || c == 'D') && md->move != 1) {
			md->dir = 2;
		}
		else if((c == 'w' || c == 'W') && md->move != 4) {
			md->dir = 3;
		}
		else if((c == 's' || c == 'S') && md->move != 3) {
			md->dir = 4;
		}
		pthread_mutex_unlock(&md->mutex); // Unlocking fields back
	}
	return NULL;
}

int main() {
	srand(time(NULL)); // Random seed

	MUTEX_DIR md;
	const struct timespec sleep_time = {
		.tv_sec = 0,
		.tv_nsec = 250 * 1000000
	}; // Main thread sleep time
	pthread_t key_handler_thread; // Thread
	uint8_t snake_tail = 1; // Length of snake
	POINT *snake = calloc(MAP_W * MAP_H, sizeof(POINT)); // Snake points (head & tail)
	POINT apple = rand_point(snake, snake_tail); // Apple point
	char *out = calloc(MAP_H * (MAP_W * 2 + 1) + 1, sizeof(char)); // Output string buffer
	// Memory formula: map_height * (map_width * 2 + 1) + 1
	// 2 elements per game object (sign + space)-^   ^    ^
	//                                  newline char-|    |
	//              empty cell for string null-terminator-|

	// Initial values
	snake[0] = (POINT){1, 1};
	md.run = true;
	md.game_over = false;
	md.restart = false;
	md.dir = 0;
	md.move = 0;
	// ---------------

	// Init mutex & handle error if there is
	if(pthread_mutex_init(&md.mutex, NULL) != 0) {
		printf("Mutex initialization error. Exiting.\n");
		return -1;
	}
	// Same with thread
	else if(pthread_create(&key_handler_thread, NULL, key_handler, &md) != 0) {
		printf("Thread initialization error. Exiting.\n");
		return -1;
	}
	// Start handling
	pthread_detach(key_handler_thread);

	// Main game loop
	while (md.run) {
		// Check for gameover
		if(md.game_over) {
			// Check for restart flag raised in key_handler_thread
			if(md.restart) {
				// Reinitiate values if flag has been raised
				snake[0] = (POINT){1, 1};
				snake_tail = 1;
				apple = rand_point(snake, snake_tail);
				md.dir = 0;
				md.move = 0;
				md.game_over = false;
				md.restart = false;
			}
			// Otherwise skip logic and just draw (go to draw)
			goto draw;
		}
		// New head position
		POINT new_head = snake[0];
		new_head.x += md.dir == 1 ? -1 : md.dir == 2 ? 1 : 0;
		new_head.y += md.dir == 3 ? -1 : md.dir == 4 ? 1 : 0;

		// Limit & teleport snake on bounds
		if(new_head.x < 0)
			new_head.x = MAP_W - 1;
		else if (new_head.x >= MAP_W)
			new_head.x = 0;

		if(new_head.y < 0)
			new_head.y = MAP_H - 1;
		else if (new_head.y >= MAP_H)
			new_head.y = 0;

		// Check for eaten apple
		if(new_head.x == apple.x && new_head.y == apple.y) {
			snake_tail++;
			apple = rand_point(snake, snake_tail);
		}

		// Move tail
		for(int i = snake_tail - 1; i > 0; --i){
			snake[i] = snake[i - 1];
			if(snake[i].x == new_head.x && snake[i].y == new_head.y)
				md.game_over = true;
		}

		snake[0] = new_head; // Set new head position
		md.move = md.dir; // Set last move direction

		// Draw goto point
		draw:
		// Clear map
		for(int8_t j = 0; j < MAP_H; ++j) {
			for(int8_t i = 0; i <= MAP_W; ++i) {
				int index = i * 2 + j * (MAP_W * 2);
				out[index] = '.';
				out[index + 1] = '.';
			}
			out[MAP_W * 2 + j * (MAP_W * 2) - 1] = '\n'; // Newline for end of row

		}
		out[MAP_H * (MAP_W * 2)] = '\0'; // Null-terminator for end of the buffer (string)
		// ----------------

		out[apple.x * 2 + apple.y * (MAP_W * 2)] = 'o'; // Display apple

		// Draw snake
		for(int i = 0; i < snake_tail; ++i)
			out[snake[i].x * 2 + snake[i].y * (MAP_W * 2)] = '@';

		printf("\e[2J\e[H"); // Clear screen ANSI escape sequence
		printf("\e[7m -- CNake (C-snake) by justfollowyourdreams -- \e[0m\n"); // Title
		printf(md.dir == 0 ? "\t(WASD to move, Q to exit)\n" : md.game_over ? "Game over! Score: %d. Press R to restart, Q to exit.\n" : "Score: %d\n", snake_tail - 1); // Info bar
		printf("%s", out); // Buffer print
		fflush(stdout); // Force stdout buffer refresh
		nanosleep(&sleep_time, NULL); // Main thread sleep
	}
	free(out); // Free buffer
	free(snake); // Free snake points
	pthread_mutex_destroy(&md.mutex); // Destroy mutex
	pthread_join(key_handler_thread, NULL); // Join to thread for finalizing stuff
	return 0; // End of the program
}
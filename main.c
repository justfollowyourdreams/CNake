#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <pthread.h>
#include <termios.h>
#include <unistd.h>
#include <stdbool.h>

#define MAP_W 8
#define MAP_H 8

typedef struct {
	int8_t x;
	int8_t y;
} POINT;

typedef struct {
	pthread_mutex_t mutex;
	uint8_t dir;
	bool run;
} MUTEX_DIR;

void enable_raw_mode() {
	struct termios term;
	tcgetattr(STDIN_FILENO, &term);
	term.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

void* key_handler(void* p) {
	MUTEX_DIR *md = p; 
	enable_raw_mode();
	while(md->run) {
		char c;
		read(STDIN_FILENO, &c, 1);
		pthread_mutex_lock(&md->mutex);
		if(c == 'a') {
			md->dir = 1;
		}
		else if(c == 'd') {
			md->dir = 2;
		}
		else if(c == 'w') {
			md->dir = 3;
		}
		else if(c == 's') {
			md->dir = 4;
		}
		else if(c == 'q') {
			md->run = false;
		}
		pthread_mutex_unlock(&md->mutex);
	}
	return NULL;
}

int main() {
	MUTEX_DIR md;
	struct timespec sleep_time;
	pthread_t key_handler_thread;
	POINT snake = {1, 1};
	char *out = calloc(MAP_H * (MAP_W * 2 + 1) + 1, sizeof(char));

	sleep_time.tv_sec = 0;
	sleep_time.tv_nsec = 250 * 1000000;
	md.run = true;

	if(pthread_mutex_init(&md.mutex, NULL) != 0) {
		printf("Mutex initialization error. Exiting.\n");
		return -1;
	}
	else if(pthread_create(&key_handler_thread, NULL, key_handler, &md) != 0) {
		printf("Thread initialization error. Exiting.\n");
		return -1;
	}
	pthread_detach(key_handler_thread);

	printf("CNake (C-snake) by justfollowyourdreams.\nWASD to move, q to exit.\n");

	while (md.run) {
		snake.x += md.dir == 1 ? -1 : md.dir == 2 ? 1 : 0;
		snake.y += md.dir == 3 ? -1 : md.dir == 4 ? 1 : 0;

		if(snake.x < 0)
			snake.x = MAP_W - 1;
		else if (snake.x >= MAP_W)
			snake.x = 0;

		if(snake.y < 0)
			snake.y = MAP_H - 1;
		else if (snake.y >= MAP_H)
			snake.y = 0;

		for(int8_t j = 0; j < MAP_H; ++j) {
			for(int8_t i = 0; i <= MAP_W; ++i) {
				int index = i * 2 + j * (MAP_W * 2 + 1);
				if(i == MAP_W) 
					out[index] = '\n';
				else if(snake.x == i && snake.y == j){
					out[index] = '+';
					out[index + 1] = ' ';
				}
				else {
					out[index] = '.';
					out[index + 1] = ' ';
				}
			}
			out[MAP_H * (MAP_W * 2 + 1)] = '\0';
		}
		printf("\033[2J\033[H");
		printf("%s", out);
		fflush(stdout);
		nanosleep(&sleep_time, NULL);
	}
	free(out);
	pthread_mutex_destroy(&md.mutex);
	pthread_join(key_handler_thread, NULL);
	return 0;
}
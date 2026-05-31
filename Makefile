# Define the compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11
TARGET = build/cnake
SRC = main.c

all: $(TARGET)
	./$(TARGET)

$(TARGET): $(SRC)
	mkdir -p build/
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -rf build/
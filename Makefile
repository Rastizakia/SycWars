CC = gcc
CFLAGS = -O0 -Wall
SRC = src/main.c
OBJ = $(SRC:.c=.o)
TARGET = sycwars

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

clean:
	rm -f src/*.o $(TARGET)

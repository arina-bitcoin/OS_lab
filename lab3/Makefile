CC      = gcc
CFLAGS  = -O2 -Wall -Wextra -pthread -std=c11
LDFLAGS = -pthread
TARGET  = sort_par
SRC     = sort_par.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

run: $(TARGET)
	./$(TARGET) -n 1048576 -t 4

clean:
	rm -f $(TARGET)

.PHONY: all run clean
C_FILES := $(wildcard *.c)
HEADER_FILES := $(wildcard *.h)
OBJ_FILES := $(patsubst %.c,%.o,$(C_FILES))

CC := gcc

all: proxy

clean:
	rm $(OBJ_FILES)
	rm proxy

%.o: %.c $(HEADER_FILES)
	$(CC) -c $<

proxy: $(OBJ_FILES) 
	$(CC) -o $@ $(OBJ_FILES)

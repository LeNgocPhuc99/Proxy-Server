C_FILES := $(shell find . -iname '*.c' ! -name 'sync_server.c')
#HEADER_FILES := $(wildcard *.h)
OBJ_FILES := $(C_FILES:.c=.o)

CC := gcc

##%.o: %.c $(HEADER_FILES)
	##$(CC) -c $<

proxy: $(OBJ_FILES) 
	$(CC) $(OBJ_FILES) $(LFLAGS) -o $@

sync_server.o: sync_server.c
	$(CC) -c $<
sync_server: sync_server.o
	$(CC) sync_server.o $(LFLAGS) -o sync_server

.PHONY: clean
all: proxy sync_server
clean:
	rm $(OBJ_FILES)
	rm proxy sync_server
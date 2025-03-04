CC = g++
CFLAGS = -std=c++11 -I/usr/include -lsqlite3 -lpthread -lrt -g

all: db_to_shm shm_reader

db_to_shm: db_to_shm.cpp
	$(CC) $< -o $@ $(CFLAGS)

shm_reader: shm_reader.cpp
	$(CC) $< -o $@ $(CFLAGS)

clean:
	rm -f db_to_shm shm_reader
	#shm_unlink /config_shm
	rm -f /dev/shm/config_shm

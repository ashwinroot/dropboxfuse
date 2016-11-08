CC=gcc
LD_LIBRARY_PATH=/usr/local/include

OBJ_PATH=obj
SRC_PATH=src
BIN_PATH=bin
MOUNT_PATH=/tmp/fuse/mnt
CACHE_PATH=/tmp/fuse/cache
INCLUDE_PATH=include
FLAGS=-g -Wall -std=gnu99

OBJ=$(addprefix $(OBJ_PATH)/,dropbox.o dropboxJson.o dropboxOAuth.o dropboxUtils.o dropboxUtils.o htbl.o dropboxFuse.o utils.o)
BIN=$(BIN_PATH)/dropboxFuse


DROPBOX_H       = $(addprefix $(INCLUDE_PATH)/, dropbox.h dropboxOAuth.h dropboxJson.h dropboxUtils.h)
DROPBOX_JSON_H  = $(addprefix $(INCLUDE_PATH)/, dropboxJson.h)
DROPBOX_OAUTH_H = $(addprefix $(INCLUDE_PATH)/, dropboxOAuth.h dropboxUtils.h)
DROPBOX_UTILS_H = $(addprefix $(INCLUDE_PATH)/, dropboxUtils.h)

HTBL_H          = $(addprefix $(INCLUDE_PATH)/, htbl.h)
UTILS_H         = $(addprefix $(INCLUDE_PATH)/, utils.h)
DROPBOX_FUSE_H  = $(addprefix $(INCLUDE_PATH)/, dropbox.h htbl.h utils.h)

all: EXPORT_VAR $(BIN_PATH) $(OBJ_PATH) $(CACHE_PATH) $(MOUNT_PATH) $(BIN)

build:
	./bin/dropboxFuse $(MOUNT_PATH) -c $(CACHE_PATH) -t /tmp/fuse/token

EXPORT_VAR: 
	export LD_LIBRARY_PATH=$$LD_LIBRARY_PATH:$(LD_LIBRARY_PATH)

$(BIN): $(OBJ)
	$(CC) $(FLAGS) $^ -o $@ -I $(INCLUDE_PATH) -lcurl -loauth -ljansson -lfuse

$(OBJ_PATH)/dropbox.o : $(SRC_PATH)/dropbox.c $(DROPBOX_H)
	$(CC) $(FLAGS) -o $@ -c $< -I $(INCLUDE_PATH)

$(OBJ_PATH)/dropboxJson.o : $(SRC_PATH)/dropboxJson.c $(DROPBOX_JSON_H)
	$(CC) $(FLAGS) -o $@ -c $< -I $(INCLUDE_PATH)

$(OBJ_PATH)/dropboxOAuth.o : $(SRC_PATH)/dropboxOAuth.c $(DROPBOX_OAUTH_H)
	$(CC) $(FLAGS) -o $@ -c $< -I $(INCLUDE_PATH)

$(OBJ_PATH)/dropboxUtils.o : $(SRC_PATH)/dropboxUtils.c $(DROPBOX_UTILS_H)
	$(CC) $(FLAGS) -o $@ -c $< -I $(INCLUDE_PATH)

$(OBJ_PATH)/htbl.o : $(SRC_PATH)/htbl.c $(HTBL_H)
	$(CC) $(FLAGS) -o $@ -c $< -I $(INCLUDE_PATH)

$(OBJ_PATH)/utils.o : $(SRC_PATH)/utils.c $(UTILS_H)
	$(CC) $(FLAGS) -o $@ -c $< -I $(INCLUDE_PATH)

$(OBJ_PATH)/dropboxFuse.o : $(SRC_PATH)/dropboxFuse.c $(DROPBOX_FUSE_H)
	$(CC) $(FLAGS) -o $@ -c $< -I $(INCLUDE_PATH) -D_FILE_OFFSET_BITS=64

$(OBJ_PATH) $(BIN_PATH) $(CACHE_PATH) $(MOUNT_PATH) :
	mkdir -p $@

clean:
	rm -rf $(OBJ_PATH) $(BIN_PATH)

rebuild: clean $(BIN)

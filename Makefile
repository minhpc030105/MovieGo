# ===== Compiler =====
CC = gcc
CFLAGS = -Wall -pthread
MYSQL_INC = /usr/include/mysql
MYSQL_LIB = -lmysqlclient

# ===== Output =====
OUTPUT_CLIENT = client_out
OUTPUT_SERVER = server_out

# ===== Source files =====
SRC_CLIENT = client/client.c \
             client/headers/function.c \
             client/headers/menu.c \
             lib/messages/message.c \
             lib/socket/socket.c

SRC_SERVER = server/server.c \
             server/headers/function.c \
             server/headers/cache.c \
             server/headers/queryUser.c \
             server/headers/logger.c \
             lib/messages/message.c \
             lib/socket/socket.c \
             lib/users/functionUser.c

# ===== Targets =====
all: $(OUTPUT_CLIENT) $(OUTPUT_SERVER)

$(OUTPUT_SERVER):
	$(CC) $(CFLAGS) -o $@ $(SRC_SERVER) -I$(MYSQL_INC) $(MYSQL_LIB)

$(OUTPUT_CLIENT):
	$(CC) $(CFLAGS) -o $@ $(SRC_CLIENT) -lm

clean:
	rm -f $(OUTPUT_CLIENT) $(OUTPUT_SERVER)
 
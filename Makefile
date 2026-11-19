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
             server/headers/dispatch.c \
             server/headers/protocol.c \
             server/headers/data.c \
             server/headers/auth.c \
             server/headers/film.c \
             server/headers/booking.c \
             server/headers/payment.c \
             server/headers/admin.c \
             server/headers/cache.c \
             server/headers/logger.c \
             lib/socket/socket.c

# ===== Targets =====
all: $(OUTPUT_CLIENT) $(OUTPUT_SERVER)

$(OUTPUT_SERVER): $(SRC_SERVER) $(wildcard server/headers/*.h lib/socket/*.h lib/*.h db/*.h)
	$(CC) $(CFLAGS) -o $@ $(SRC_SERVER) -I$(MYSQL_INC) $(MYSQL_LIB)

$(OUTPUT_CLIENT): $(SRC_CLIENT) $(wildcard client/headers/*.h lib/messages/*.h lib/socket/*.h lib/*.h)
	$(CC) $(CFLAGS) -o $@ $(SRC_CLIENT) -lm

clean:
	rm -f $(OUTPUT_CLIENT) $(OUTPUT_SERVER)
 
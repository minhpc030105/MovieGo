# ===== Compiler =====
CC = gcc
CFLAGS = -Wall -Wextra -pthread
MYSQL_INC = /usr/include/mysql
MYSQL_LIB = -lmysqlclient
SSL_LIB = -lssl -lcrypto

# ===== Output =====
OUTPUT_CLIENT = client_out
OUTPUT_SERVER = server_out
OUTPUT_TEST_SEC = test_security_out

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
             server/headers/security_utils.c \
             server/headers/logger.c \
             lib/messages/message.c \
             lib/socket/socket.c \
             lib/users/functionUser.c

SRC_TEST_SEC = tests/test_security.c \
               server/headers/security_utils.c

# ===== Targets =====
all: $(OUTPUT_CLIENT) $(OUTPUT_SERVER)

$(OUTPUT_SERVER):
	$(CC) $(CFLAGS) -o $@ $(SRC_SERVER) -I$(MYSQL_INC) $(MYSQL_LIB) $(SSL_LIB)

$(OUTPUT_CLIENT):
	$(CC) $(CFLAGS) -o $@ $(SRC_CLIENT) -lm

test_security: $(OUTPUT_TEST_SEC)

$(OUTPUT_TEST_SEC):
	$(CC) $(CFLAGS) -o $@ $(SRC_TEST_SEC) $(SSL_LIB)

clean:
	rm -f $(OUTPUT_CLIENT) $(OUTPUT_SERVER) $(OUTPUT_TEST_SEC)

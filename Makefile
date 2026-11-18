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
OUTPUT_TEST_POOL = test_db_pool_out

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
             server/headers/db_pool.c \
             server/headers/logger.c \
             lib/messages/message.c \
             lib/socket/socket.c \
             lib/users/functionUser.c

SRC_TEST_SEC = tests/test_security.c \
               server/headers/security_utils.c

SRC_TEST_POOL = tests/test_db_pool.c \
                server/headers/db_pool.c

# ===== Targets =====
all: $(OUTPUT_CLIENT) $(OUTPUT_SERVER)

$(OUTPUT_SERVER):
	$(CC) $(CFLAGS) -o $@ $(SRC_SERVER) -I$(MYSQL_INC) $(MYSQL_LIB) $(SSL_LIB)

$(OUTPUT_CLIENT):
	$(CC) $(CFLAGS) -o $@ $(SRC_CLIENT) -lm

test_security: $(OUTPUT_TEST_SEC)

$(OUTPUT_TEST_SEC):
	$(CC) $(CFLAGS) -o $@ $(SRC_TEST_SEC) $(SSL_LIB)

test_db_pool: $(OUTPUT_TEST_POOL)

$(OUTPUT_TEST_POOL):
	$(CC) $(CFLAGS) -o $@ $(SRC_TEST_POOL) -I$(MYSQL_INC) $(MYSQL_LIB)

tests: test_security test_db_pool

clean:
	rm -f $(OUTPUT_CLIENT) $(OUTPUT_SERVER) $(OUTPUT_TEST_SEC) $(OUTPUT_TEST_POOL)

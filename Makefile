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
OUTPUT_TEST_C_MOD = test_c_modules_out
OUTPUT_TEST_LOCK = test_booking_lock_out

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
             server/headers/queryUser.c \
             server/headers/film.c \
             server/headers/booking.c \
             server/headers/booking_common.c \
             server/headers/booking_lock.c \
             server/headers/sweeper.c \
             server/headers/payment.c \
             server/headers/admin.c \
             server/headers/security_utils.c \
             server/headers/db_pool.c \
             server/headers/logger.c \
             lib/socket/socket.c

SRC_TEST_SEC = tests/test_security.c \
               server/headers/security_utils.c

SRC_TEST_POOL = tests/test_db_pool.c \
                server/headers/db_pool.c

SRC_TEST_C_MOD = tests/test_c_modules.c \
                 server/headers/queryUser.c \
                 server/headers/data.c \
                 server/headers/security_utils.c \
                 server/headers/protocol.c

SRC_TEST_LOCK = tests/test_booking_lock.c \
                server/headers/booking_lock.c

# ===== Targets =====
all: $(OUTPUT_CLIENT) $(OUTPUT_SERVER)

$(OUTPUT_SERVER): $(SRC_SERVER) $(wildcard server/headers/*.h lib/socket/*.h lib/*.h db/*.h)
	$(CC) $(CFLAGS) -o $@ $(SRC_SERVER) -I$(MYSQL_INC) $(MYSQL_LIB) $(SSL_LIB)

$(OUTPUT_CLIENT): $(SRC_CLIENT) $(wildcard client/headers/*.h lib/messages/*.h lib/socket/*.h lib/*.h)
	$(CC) $(CFLAGS) -o $@ $(SRC_CLIENT) -lm

test_security: $(OUTPUT_TEST_SEC)

$(OUTPUT_TEST_SEC):
	$(CC) $(CFLAGS) -o $@ $(SRC_TEST_SEC) $(SSL_LIB)

test_db_pool: $(OUTPUT_TEST_POOL)

$(OUTPUT_TEST_POOL):
	$(CC) $(CFLAGS) -o $@ $(SRC_TEST_POOL) -I$(MYSQL_INC) $(MYSQL_LIB)

test_c_modules: $(OUTPUT_TEST_C_MOD) $(OUTPUT_TEST_LOCK)

$(OUTPUT_TEST_C_MOD):
	$(CC) $(CFLAGS) -o $@ $(SRC_TEST_C_MOD) -I$(MYSQL_INC) $(MYSQL_LIB) $(SSL_LIB)

test_booking_lock: $(OUTPUT_TEST_LOCK)

$(OUTPUT_TEST_LOCK):
	$(CC) $(CFLAGS) -o $@ $(SRC_TEST_LOCK)

tests: test_security test_db_pool test_c_modules test_booking_lock

clean:
	rm -f $(OUTPUT_CLIENT) $(OUTPUT_SERVER) $(OUTPUT_TEST_SEC) $(OUTPUT_TEST_POOL) $(OUTPUT_TEST_C_MOD) $(OUTPUT_TEST_LOCK)

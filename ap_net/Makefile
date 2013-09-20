PATH1="."

CC=gcc
CFLAGS = $(OPTS) -Wall
# -mtune=pentium3 -m32

OBJDIR ?= ../compiled

deps=ap_net.h
conn_pool_obj=conn_pool/accept_connection.o conn_pool/bind.o conn_pool/check_conns.o conn_pool/check_state_sel.o conn_pool/close_connection.o conn_pool/connection_is_alive.o conn_pool/create.o conn_pool/move_conn.o conn_pool/poll.o conn_pool/poller_utils.o conn_pool/print_stat.o conn_pool/recv.o conn_pool/send.o conn_pool/set_max_connections.o conn_pool/utils.o
conn_pool_deps=$(deps) conn_pool/conn_pool_internals.h

poller_obj=poller/create.o poller/poll.o poller/single_fd.o
poller_deps=$(deps)

all: $(conn_pool_obj) $(poller_obj) $(deps)

clean:
	rm -f $(conn_pool_obj) $(poller_obj)

ap_net.tests: $(obj)
	$(CC) -c $(OPTS) ap_net_tests.c -o ap_net_tests conn_pool/*.o poller/*.o


conn_pool/accept_connection.o: conn_pool/accept_connection.c $(conn_pool_deps)
	$(CC) -c $(CFLAGS) conn_pool/accept_connection.c -o conn_pool/accept_connection.o

conn_pool/bind.o: conn_pool/bind.c $(conn_pool_deps)
	$(CC) -c $(CFLAGS) conn_pool/bind.c -o conn_pool/bind.o

conn_pool/check_conns.o: conn_pool/check_conns.c $(conn_pool_deps)
	$(CC) -c $(CFLAGS) conn_pool/check_conns.c -o conn_pool/check_conns.o

conn_pool/check_state_sel.o: conn_pool/check_state_sel.c $(conn_pool_deps)
	$(CC) -c $(CFLAGS) conn_pool/check_state_sel.c -o conn_pool/check_state_sel.o

conn_pool/close_connection.o: conn_pool/close_connection.c $(conn_pool_deps)
	$(CC) -c $(CFLAGS) conn_pool/close_connection.c -o conn_pool/close_connection.o

conn_pool/connection_is_alive.o: conn_pool/connection_is_alive.c $(conn_pool_deps)
	$(CC) -c $(CFLAGS) conn_pool/connection_is_alive.c -o conn_pool/connection_is_alive.o

conn_pool/create.o: conn_pool/create.c $(conn_pool_deps)
	$(CC) -c $(CFLAGS) conn_pool/create.c -o conn_pool/create.o

conn_pool/move_conn.o: conn_pool/move_conn.c $(conn_pool_deps)
	$(CC) -c $(CFLAGS) conn_pool/move_conn.c -o conn_pool/move_conn.o

conn_pool/poll.o: conn_pool/poll.c $(conn_pool_deps)
	$(CC) -c $(CFLAGS) conn_pool/poll.c -o conn_pool/poll.o

conn_pool/poller_utils.o: conn_pool/poller_utils.c $(conn_pool_deps)
	$(CC) -c $(CFLAGS) conn_pool/poller_utils.c -o conn_pool/poller_utils.o

conn_pool/print_stat.o: conn_pool/print_stat.c $(conn_pool_deps)
	$(CC) -c $(CFLAGS) conn_pool/print_stat.c -o conn_pool/print_stat.o

conn_pool/recv.o: conn_pool/recv.c $(conn_pool_deps)
	$(CC) -c $(CFLAGS) conn_pool/recv.c -o conn_pool/recv.o

conn_pool/send.o: conn_pool/send.c $(conn_pool_deps)
	$(CC) -c $(CFLAGS) conn_pool/send.c -o conn_pool/send.o

conn_pool/set_max_connections.o: conn_pool/set_max_connections.c $(conn_pool_deps)
	$(CC) -c $(CFLAGS) conn_pool/set_max_connections.c -o conn_pool/set_max_connections.o

conn_pool/utils.o: conn_pool/utils.c $(conn_pool_deps)
	$(CC) -c $(CFLAGS) conn_pool/utils.c -o conn_pool/utils.o

poller/create.o: poller/create.c $(poller_deps)
	$(CC) -c $(CFLAGS) poller/create.c -o poller/create.o

poller/poll.o: poller/poll.c $(poller_deps)
	$(CC) -c $(CFLAGS) poller/poll.c -o poller/poll.o

poller/single_fd.o: poller/single_fd.c $(poller_deps)
	$(CC) -c $(CFLAGS) poller/single_fd.c -o poller/single_fd.o


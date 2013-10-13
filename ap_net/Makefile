PATH1="."

CC=gcc
CFLAGS = $(OPTS) -Wall -ggdb
# -mtune=pentium3 -m32

OBJDIR ?= ../compiled

common_deps=ap_net.h
conn_pool_obj = conn_pool_accept_connection.o
conn_pool_obj += conn_pool_check_conns.o
conn_pool_obj += conn_pool_check_state_sel.o
conn_pool_obj += conn_pool_close_connection.o
conn_pool_obj += conn_pool_connect.o
conn_pool_obj += conn_pool_connection_is_alive.o
conn_pool_obj += conn_pool_connection_pre_connect.o
conn_pool_obj += conn_pool_create.o
conn_pool_obj += conn_pool_listener_create.o
conn_pool_obj += conn_pool_move_conn.o
conn_pool_obj += conn_pool_poll.o
conn_pool_obj += conn_pool_poller_utils.o
conn_pool_obj += conn_pool_print_stat.o
conn_pool_obj += conn_pool_recv.o
conn_pool_obj += conn_pool_send.o
conn_pool_obj += conn_pool_set_addr.o
conn_pool_obj += conn_pool_set_max_connections.o
conn_pool_obj += conn_pool_utils.o

conn_pool_deps=$(common_deps) conn_pool_internals.h

poller_obj = poller_create.o
poller_obj += poller_poll.o
poller_obj += poller_single_fd.o

poller_deps=$(common_deps)

all: lib compiletests

lib: $(conn_pool_obj) $(poller_obj) $(common_deps)

clean:
	rm -f $(conn_pool_obj) $(poller_obj)

compiletests: $(obj) ../$(libname)
	$(CC) $(CFLAGS) ap_net.tests.c -o ap_net.tests -l:../$(libname)

conn_pool_%.o:
deps=$(conn_pool_deps)

poller_%.o:
deps=$(poller_deps)

%.o: %.c $(deps)
	$(CC) -c $(CFLAGS) $< -o $@

SRCS:=$(wildcard *.c)
OBJS:=$(patsubst %.c,%.o,$(SRCS))
ELF:=process_server
CC:=gcc
CFLAGS:=-g -I /usr/include/mysql
$(ELF):$(OBJS)
	gcc $^ -o $@ -lpthread -I /usr/include/mysql/ -lmysqlclient
.PHONY:clean
clean:
	rm -rf $(ELF) $(OBJS)

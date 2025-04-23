SRCS:=$(wildcard *.c)
OBJS:=$(patsubst %.c,%.o,$(SRCS))
ELF:=process_client
CC:=gcc
CFLAGS:=-g 
$(ELF):$(OBJS)
	gcc $^ -o $@ -lpthread -lcrypt
.PHONY:clean
clean:
	rm -rf $(ELF) $(OBJS)

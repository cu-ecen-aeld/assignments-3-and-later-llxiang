CC = gcc
RM = rm -f
CFLAGS += -Wall -Werror

all: aesdsocket

aesdsocket:
	$(CROSS_COMPILE)$(CC) $(CFLAGS) -o aesdsocket aesdsocket.c

.PHONY : clean
clean:
	$(RM) *.o aesdsocket aesdsocket1

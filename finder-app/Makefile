CC = gcc
RM = rm -f

all: Write

Write:
	$(CROSS_COMPILE)$(CC) -o writer writer.c

.PHONY : clean
clean:
	$(RM) *.o writer

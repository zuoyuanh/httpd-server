CC = gcc
CFLAGS = -Wall -pedantic
MAIN = httpd
HEADERS = simple_net.h handler.h net_util.h basic_util.h limit_fork.h
OBJS = httpd.o handler.o simple_net.o net_util.o basic_util.o limit_fork.o
all : $(MAIN)

$(MAIN) : $(OBJS) $(HEADERS)
	$(CC) $(CFLAGS) -o $(MAIN) $(OBJS)

httpd.o : httpd.c $(HEADERS)
	$(CC) $(CFLAGS) -c httpd.c

simple_net.o : simple_net.c $(HEADERS)
	$(CC) $(CFLAGS) -c simple_net.c

handler.o : handler.c $(HEADERS)
	$(CC) $(CFLAGS) -c handler.c

net_util.o : net_util.c $(HEADERS)
	$(CC) $(CFLAGS) -c net_util.c

basic_util.o : basic_util.c $(HEADERS)
	$(CC) $(CFLAGS) -c basic_util.c

limit_fork.o : limit_fork.c $(HEADERS)
	$(CC) $(CFLAGS) -c limit_fork.c

clean :
	rm *.o $(MAIN)

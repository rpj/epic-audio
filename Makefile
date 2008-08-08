CC 	= gcc
EXEC 	= ea
CFLAGS 	= -g -w -O2 -DDEBUG -D__USE_GNU -D_GNU_SOURCE
CCFLAGS = -lid3tag -lmad -lz -lpthread $(CFLAGS)
OBJS 	= command.o list.o socket.o ea.o util.o search.o playlist.o

all:	$(EXEC)

$(EXEC):	$(OBJS)
	$(CC) $(CCFLAGS) -o $(EXEC) $(OBJS)

clean:	
	rm -fr $(OBJS) $(EXEC) *.~*.~ gmon.out

command.o:	command.c command.h util.h ea.h
ea.o:		ea.c ea.h util.h ea.h
list.o:		list.c list.h util.h ea.h
socket.o:	socket.c list.c socket.h list.h util.h ea.h
util.o:		util.c util.h ea.h
search.o: 	search.c util.c search.h util.h ea.h
playlist.o:	playlist.h ea.h
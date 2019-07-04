INCLUDE_DIRS = 
LIB_DIRS = 

CDEFS=
CFLAGS= -O -g $(INCLUDE_DIRS) $(CDEFS) -DLINUX
LIBS=-lpthread -lrt

HFILES=

CFILES1= Q3_deadlock.c 

SRCS1= ${HFILES} ${CFILES1}

OBJS1= ${CFILES1:.c=.o}

all: Q3_deadlock 

clean:
	-rm -f *.o *.d *.exe Q3_deadlock


Q3_deadlock: Q3_deadlock.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $(OBJS1) $(LIBS)



depend:

.c.o:
	$(CC) -MD $(CFLAGS) -c $<

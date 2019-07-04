INCLUDE_DIRS = 
LIB_DIRS = 

CDEFS=
CFLAGS= -O -g $(INCLUDE_DIRS) $(CDEFS) -DLINUX
LIBS=-lpthread -lrt

HFILES=

CFILES1= Q5.c 

SRCS1= ${HFILES} ${CFILES1}

OBJS1= ${CFILES1:.c=.o}

all: Q5 

clean:
	-rm -f *.o *.d *.exe Q5


Q5: Q5.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $(OBJS1) $(LIBS)



depend:

.c.o:
	$(CC) -MD $(CFLAGS) -c $<

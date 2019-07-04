INCLUDE_DIRS = 
LIB_DIRS = 

CDEFS=
CFLAGS= -O -g $(INCLUDE_DIRS) $(CDEFS) -DLINUX
LIBS=-lpthread -lrt

HFILES=

CFILES1= Q4_heap.c 

SRCS1= ${HFILES} ${CFILES1}

OBJS1= ${CFILES1:.c=.o}

all: Q4_heap 

clean:
	-rm -f *.o *.d *.exe Q4_heap


Q4_heap: Q4_heap.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $(OBJS1) $(LIBS)



depend:

.c.o:
	$(CC) -MD $(CFLAGS) -c $<

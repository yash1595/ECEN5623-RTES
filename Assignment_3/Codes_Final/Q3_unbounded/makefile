INCLUDE_DIRS = 
LIB_DIRS = 

CDEFS=
CFLAGS= -O -g $(INCLUDE_DIRS) $(CDEFS) -DLINUX
LIBS=-lpthread -lrt

HFILES=

CFILES1= Q3_unbounded.c 

SRCS1= ${HFILES} ${CFILES1}

OBJS1= ${CFILES1:.c=.o}

all: Q3_unbounded 

clean:
	-rm -f *.o *.d *.exe Q3_unbounded


Q3_unbounded: Q3_unbounded.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $(OBJS1) $(LIBS)



depend:

.c.o:
	$(CC) -MD $(CFLAGS) -c $<

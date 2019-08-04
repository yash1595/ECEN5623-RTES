IDIR =../inc
CC=gcc
CFLAGS= -Wall -O0 -lpthread -lrt -I$(IDIR)

ODIR=obj
LDIR =

LIBS=-lm

_DEPS = main.h client.h SYSLOG.h camera.h deadline.h nonblur.h pthread_dump_pgm.h pthread_gray.h pthread_read_frame.h pthread_scheduler.h thread_init.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = main.o client.o camera.o deadline.o nonblur.o pthread_dump_pgm.o pthread_gray.o pthread_read_frame.o pthread_scheduler.o SYSLOG.o thread_init.o 
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

capture: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ 

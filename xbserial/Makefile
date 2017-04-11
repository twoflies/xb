CC=g++
CFLAGS=-g -Wall -fPIC -L. -Wl,-rpath,.

ODIR=obj

LIBS=-lpthread

_OBJ = serial.o command.o frame.o file.o log.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: %.cc %.h
	$(CC) -c -o $@ $< $(CFLAGS)

libxbserial: $(OBJ)
	$(CC) -shared -o $@.so $^ $(CFLAGS) $(LIBS) -lm

test: $(ODIR)/test.o
	$(CC) -o test $^ $(CFLAGS) $(LIBS) -lxbserial

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core libxbserial.so test

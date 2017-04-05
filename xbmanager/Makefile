CC=g++
CFLAGS=-g -I ../xbserial -Wall -fPIC -L. -Wl,-rpath,.

ODIR=obj

LIBS=-lpthread -lxbserial

_OBJ = manager.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: %.cc %.h
	$(CC) -c -o $@ $< $(CFLAGS)

libxbmanager: $(OBJ)
	$(CC) -shared -o $@.so $^ $(CFLAGS) $(LIBS) -lm

xbm: $(ODIR)/xbm.o
	$(CC) -o xbm $^ $(CFLAGS) $(LIBS) -lxbmanager

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core libxbmanager.so xbm

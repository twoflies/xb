TOPTARGETS := all clean

SUBDIRS := xbserial xbmanager

all: xbserial xbmanager lib
xbserial:
	$(MAKE) -C xbserial $(MAKECMDGOALS)
	cp xbserial/libxbserial.so xbmanager/
xbmanager:
	$(MAKE) -C $@ $(MAKECMDGOALS)
lib: xbserial xbmanager
	cp xbserial/libxbserial.so lib/
	cp xbmanager/libxbmanager.so lib/
	cp xbserial/*.h lib/include/xbserial/
	cp xbmanager/*.h lib/include/xbmanager/

clean: xbserial_clean xbmanager_clean lib_clean
xbserial_clean:
	$(MAKE) -C xbserial clean
xbmanager_clean:
	$(MAKE) -C xbmanager clean
lib_clean:
	rm -f lib/*.so
	rm -f lib/include/xbserial/*.h
	rm -f lib/include/xbmanager/*.h

.PHONY: $(TOPTARGETS) $(SUBDIRS)

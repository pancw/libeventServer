INSTALLDIR=$(shell pwd)/../install/
LUALIB=-I$(INSTALLDIR)/lua/include -L$(INSTALLDIR)/lua/lib -llua
SOCKETLIB=-lws2_32

.PHONY: all win linux

all : 
	@echo Please do \'make PLATFORM\' where PLATFORM is one of these:
	@echo win linux

win: mongo.dll

linux: mongo.so

mongo.dll : lua-mongo.c lua-socket.c
	gcc --shared -Wall -g $^ -o$@ $(LUALIB) $(SOCKETLIB)

mongo.so : lua-mongo.c lua-socket.c
	gcc --shared -Wall -fPIC -g $^ -o$@ $(LUALIB) -lm

clean:
	rm -f mongo.dll mongo.so

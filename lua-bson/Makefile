INSTALLDIR=$(shell pwd)/install/
LUALIB=-I../install/lua/include -L../install/lua/lib -llua
SOCKETLIB=-lws2_32

.PHONY: all win linux

all : 
	@echo Please do \'make PLATFORM\' where PLATFORM is one of these:
	@echo win linux

win: bson.dll

linux: bson.so

bson.dll : bson.c
	gcc --shared -Wall -O2 $^ -o$@ $(LUALIB) $(SOCKETLIB)

bson.so : bson.c
	gcc --shared -Wall -fPIC -O2 $^ -o$@ $(LUALIB) -lm

clean:
	rm -f bson.dll bson.so

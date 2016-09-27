ROOT=.
SRC=server.cpp

EXTLIB=$(ROOT)/../
Flag= -g -std=c++0x -Wall -O0  -I $(EXTLIB)/install/libevent/include -I $(EXTLIB)/install/lua/include

LIBEVENT=$(EXTLIB)/install/libevent/lib/libevent.a  $(EXTLIB)/install/libevent/lib/libevent_core.a \
	$(EXTLIB)/install/libevent/lib/libevent_extra.a  $(EXTLIB)/install/libevent/lib/libevent_pthreads.a

LIBLUA=$(EXTLIB)/install/lua/lib/liblua.a

CPPFLAGS=$(Flag) -DENGINE_MAIN -I $(ROOT)/include

OBJ=$(subst .cpp,.o,$(SRC))
.dep:
	$(CXX) -MM $(CPPFLAGS) $(SRC) | sed s/"^\(.*\).o: \(.*\).cpp "/"\2.o: \2.cpp "/ > .dep

ENGINE_BIN=game_server

all: .dep $(ENGINE_BIN) luamongo

-include .dep

luamongo:
	cp -r $(EXTLIB)/install/lua-mongo $(ROOT)/src

$(ENGINE_BIN):$(OBJ) $(LIBEVENT) $(LIBLUA)
	$(CXX) $(CFLAGS) -o $@  $^ -lssl -lrt -Wl,-E -ldl

extlib:
	(cd $(EXTLIB)/ && make)

cleanall: clean clean3rd

clean:
	find . -name ".dep" -delete
	find . -name "*.o" -delete
	rm $(ENGINE_BIN)
#	rm -rf .dep *.o *.core $(ENGINE_BIN) 
	
cleanextlib:
	(cd $(EXTLIB)/ && make clean)

INSTALLDIR=$(shell pwd)/install/
all: libevent luadev luabson luamongo

libevent:
	(cd libevent-2.0.22-stable && ./configure --prefix=$(INSTALLDIR)/libevent && make install)
luadev:
	(cd lua-5.3.3 && make linux && make install INSTALL_TOP=$(INSTALLDIR)/lua)
luabson:
	cd $(INSTALLDIR) && mkdir -p lua-mongo
	cd lua-bson && make linux && cp bson.so $(INSTALLDIR)/lua-mongo
luamongo:
	cd lua-mongo && make linux && cp mongo.so mongo.lua $(INSTALLDIR)/lua-mongo

clean:
	(cd libevent-2.0.22-stable && make clean)
	(cd lua-5.3.3 && make clean)
	(cd lua-bson && make clean)
	(cd lua-mongo && make clean)

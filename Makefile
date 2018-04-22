prefix=/usr/local

build:
	g++ -O3 -Wextra testual.cpp -o testual

install:
	@install -m 0755 testual $(prefix)/bin
	@install -m 0644 testual.h $(prefix)/include

uninstall:
	@rm $(prefix)/bin/testual
	@rm $(prefix)/include/testual.h

clean:
	rm testual

all:	DisplayNixie DisplayNixie3x

install:
	cp DisplayNixie DisplayNixie3x /usr/local/sbin
	cp DisplayNixie.conf.example /etc/DisplayNixie.conf
	@echo "========================="
	@echo " Depending on whether you have a version 2 or version 3 card,"
	@echo " add \"/usr/local/sbin/DisplayNixie3x -c /etc/DisplayNixie.conf &\" or"
	@echo " add \"/usr/local/sbin/DisplayNixie -c /etc/DisplayNixie.conf &\""
	@echo " to /etc/rc.local or similar."
	@echo " "
	@echo " Adjust /etc/DisplayNixie.conf to your liking."
	@echo "========================="

clean:
	rm -f DisplayNixie3x.o DisplayNixie.o DisplayNixie3x DisplayNixie *~

DisplayNixie:	DisplayNixie.o
	g++ DisplayNixie.o -o DisplayNixie -lwiringPi

DisplayNixie.o:	DisplayNixie.cpp
	g++ -O0 -g3 -Wall -fmessage-length=0 -c -o DisplayNixie.o DisplayNixie.cpp

DisplayNixie3x:	DisplayNixie3x.o
	g++ DisplayNixie3x.o -o DisplayNixie3x -lwiringPi

DisplayNixie3x.o:	DisplayNixie3x.cpp
	g++ -O0 -g3 -Wall -fmessage-length=0 -c -o DisplayNixie3x.o DisplayNixie3x.cpp




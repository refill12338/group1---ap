all: main.c update.o JSON_action.o config.o
	mipsel-openwrt-linux-gcc -std=c99 -I/home/se-group1/ap/include main.c update.o JSON_action.o config.o -o ap_client
	rm *.o
update.o: update.c
	mipsel-openwrt-linux-gcc -std=c99 -I/home/se-group1/ap/include -c update.c -o update.o
JSON_action.o: JSON_action.o
	mipsel-openwrt-linux-gcc -std=c99 -I/home/se-group1/ap/include -c JSON_action.c -o JSON_action.o
config.o: config.o
	mipsel-openwrt-linux-gcc -std=c99 -I/home/se-group1/ap/include -c config.c -o config.o

SELF_DIR := ${CURDIR}

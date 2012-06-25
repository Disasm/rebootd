COMMON_OBJS = rc4.o key.o sha1.o rng.o
REBOOTD_OBJS = rebootd.o daemon.o $(COMMON_OBJS)
REBOOT_OBJS = reboot.o $(COMMON_OBJS)
CFLAGS = -O2 -Wall -Wno-unused-result

all: rebootd reboot

rebootd: $(REBOOTD_OBJS)
	g++ $(CFLAGS) -o $@ $(REBOOTD_OBJS)

reboot: $(REBOOT_OBJS)
	g++ $(CFLAGS) -o $@ $(REBOOT_OBJS)

.cpp.o:
	g++ $(CFLAGS) -o $@ -c $<

clean:
	rm -f *.o reboot rebootd

re: clean all


CFLAGS=-Wall -g
LDFLAGS=-g -lz

OBJS=azone.o s3d.o wld.o

#EQWldData.o EQWldDataFragment.o

#TODO: get un-lazy and compile to .o files.

all: azone

azone: $(OBJS)
	g++ $(LDFLAGS) $(OBJS) -o azone

clean:
	rm -f $(OBJS) azone


s3d.o: s3d.c s3d.h
	$(CC) -c $(CFLAGS) -O3 s3d.c -o s3d.o

wld.o: wld.c wld.h
	$(CC) -c $(CFLAGS) -O3 wld.c -o wld.o

%.o: %.cpp
	$(CC) -c $(CFLAGS) $< -o $@

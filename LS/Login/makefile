APP=login
SF=../common/debug.o ../common/crc32.o ../common/EQNetwork.o \
   ../common/timer.o ../common/database.o ../common/packet_dump.o \
   ../common/Mutex.o ../common/unix.o ../common/MiscFunctions.o \
   ../common/TCPConnection.o ../common/packet_functions.o \
   ../common/dbcore.o ../common/md5.o ../common/EQEMuError.o \
   EQCrypto.o logindatabase.o client.o net.o LWorld.o

#CC=/home/per/gcc-2.95.2/gcc/g++
#LINKER=/home/per/gcc-2.95.2/gcc/g++
CC=gcc
LINKER=gcc
COPTS=-g -march=i486 -O3 -pthread -pipe -DFX -D_GNU_SOURCE -DLOGINCRYPTO -DINVERSEXY -DNDEBUG -D_CONSOLE -I/usr/include/mysql -Wno-deprecated
#COPTS=-O3 -fomit-frame-pointer -m486 -Wall -pipe -DFX
LINKOPTS=-L. -lstdc++ -lm -L/usr/lib/mysql -lmysqlclient -lz -ldl

all: $(APP)

$(APP): $(SF)
	$(LINKER) $(COPTS) $(OBJS) $^ $(LINKOPTS) -o $@

clean:
	rm $(SF) $(APP)

%.o: %.cpp
	$(CC) -c $(COPTS) $< -o $@
	

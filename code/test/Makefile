# use normal make for this Makefile
#
# Makefile for building user programs to run on top of Nachos
#
# Several things to be aware of:
#
#    Nachos assumes that the location of the program startup routine (the
# 	location the kernel jumps to when the program initially starts up)
#       is at location 0.  This means: start.o must be the first .o passed 
# 	to ld, in order for the routine "Start" to be loaded at location 0
#

# If you are running on SunOS 5.x machines, uncomment the next line
GCCDIR = ../gnu/
LDFLAGS = -T newscript -N
ASFLAGS = -mips1
CPPFLAGS = $(INCDIR)

# if you aren't cross-compiling:
# GCCDIR =
# LDFLAGS = -N -T 0
# ASFLAGS =
# CPPFLAGS = -P $(INCDIR)


CC = $(GCCDIR)gcc
AS = $(GCCDIR)as
LD = $(GCCDIR)ld
CPP = $(GCCDIR)cpp

INCDIR =-I../userprog -I../threads
CFLAGS = -G 0 -c $(INCDIR)


all: halt matmult sort testfiles lockcondition fork exec passport multipassport locksharing netmvtest netclient addtomv signal multilarge

start.o: start.s ../userprog/syscall.h
	$(CPP) $(CPPFLAGS) start.s > strt.s
	$(AS) $(ASFLAGS) -o start.o strt.s
	rm strt.s

halt.o: halt.c
	$(CC) $(CFLAGS) -c halt.c
halt: halt.o start.o
	$(LD) $(LDFLAGS) start.o halt.o -o halt.coff
	../bin/coff2noff halt.coff halt

sort.o: sort.c
	$(CC) $(CFLAGS) -c sort.c
sort: sort.o start.o
	$(LD) $(LDFLAGS) start.o sort.o -o sort.coff
	../bin/coff2noff sort.coff sort

matmult.o: matmult.c
	$(CC) $(CFLAGS) -c matmult.c
matmult: matmult.o start.o
	$(LD) $(LDFLAGS) start.o matmult.o -o matmult.coff
	../bin/coff2noff matmult.coff matmult

testfiles.o: testfiles.c
	$(CC) $(CFLAGS) -c testfiles.c
testfiles: testfiles.o start.o
	$(LD) $(LDFLAGS) start.o testfiles.o -o testfiles.coff
	../bin/coff2noff testfiles.coff testfiles

lockcondition.o: lockcondition.c	
	$(CC) $(CFLAGS) -c lockcondition.c 
lockcondition: lockcondition.o start.o 	
	$(LD) $(LDFLAGS) start.o lockcondition.o -o lockcondition.coff
	../bin/coff2noff lockcondition.coff lockcondition  

fork.o: fork.c	
	$(CC) $(CFLAGS) -c fork.c 
fork: fork.o start.o 	
	$(LD) $(LDFLAGS) start.o fork.o -o fork.coff
	../bin/coff2noff fork.coff fork  

exec.o: exec.c	
	$(CC) $(CFLAGS) -c exec.c 
exec: exec.o start.o 	
	$(LD) $(LDFLAGS) start.o exec.o -o exec.coff
	../bin/coff2noff exec.coff exec 

passport.o: passport.c	
	$(CC) $(CFLAGS) -c passport.c 
passport: passport.o start.o 	
	$(LD) $(LDFLAGS) start.o passport.o -o passport.coff
	../bin/coff2noff passport.coff passport  

multipassport.o: multipassport.c	
	$(CC) $(CFLAGS) -c multipassport.c 
multipassport: multipassport.o start.o 	
	$(LD) $(LDFLAGS) start.o multipassport.o -o multipassport.coff
	../bin/coff2noff multipassport.coff multipassport  


locksharing.o: locksharing.c	
	$(CC) $(CFLAGS) -c locksharing.c 
locksharing: locksharing.o start.o 	
	$(LD) $(LDFLAGS) start.o locksharing.o -o locksharing.coff
	../bin/coff2noff locksharing.coff locksharing  

netmvtest.o: netmvtest.c	
	$(CC) $(CFLAGS) -c netmvtest.c 
netmvtest: netmvtest.o start.o 	
	$(LD) $(LDFLAGS) start.o netmvtest.o -o netmvtest.coff
	../bin/coff2noff netmvtest.coff netmvtest  


netclient.o: netclient.c	
	$(CC) $(CFLAGS) -c netclient.c 
netclient: netclient.o start.o 	
	$(LD) $(LDFLAGS) start.o netclient.o -o netclient.coff
	../bin/coff2noff netclient.coff netclient  


addtomv.o: addtomv.c	
	$(CC) $(CFLAGS) -c addtomv.c 
addtomv: addtomv.o start.o 	
	$(LD) $(LDFLAGS) start.o addtomv.o -o addtomv.coff
	../bin/coff2noff addtomv.coff addtomv  


signal.o: signal.c	
	$(CC) $(CFLAGS) -c signal.c 
signal: signal.o start.o 	
	$(LD) $(LDFLAGS) start.o signal.o -o signal.coff
	../bin/coff2noff signal.coff signal  

multilarge.o: multilarge.c	
	$(CC) $(CFLAGS) -c multilarge.c 
multilarge: multilarge.o start.o 	
	$(LD) $(LDFLAGS) start.o multilarge.o -o multilarge.coff
	../bin/coff2noff multilarge.coff multilarge  



clean:
	rm -f *.o *.coff

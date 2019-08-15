hw3 : testcase.o buf.o Disk.o fs.o fat.o myFuntion.o
	gcc -o hw3 testcase.o buf.o Disk.o fs.o fat.o myFuntion.o
testcase.o : testcase.c buf.h fs.h
	gcc -c testcase.c
buf.o : buf.c buf.h queue.h Disk.h
	gcc -c buf.c
Disk.o : Disk.c Disk.h
	gcc -c Disk.c
fs.o : buf.h myFuntion.h fat.h fs.h
	gcc -c fs.c
fat.o : fs.h Disk.h
	gcc -c fat.c
myFuntion.o : myFuntion.h
	gcc -c myFuntion.c
clear :
	rm *.o

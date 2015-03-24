sfs_test : sfs_test.o sfs.o disk_emu.o
	gcc -o sfs_test sfs_test.o sfs.o disk_emu.o

sfs : sfs.o disk_emu.o disk_emu.h
	gcc -g -o sfs sfs.o disk_emu.o

sfs.o : sfs.c disk_emu.h
	gcc -c sfs.c

disk_emu.o : disk_emu.c
	gcc -c disk_emu.c

clean : 
	rm -rf *.o
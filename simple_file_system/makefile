sfs_test : sfs_test.o sfs_api.o disk_emu.o
	gcc -o Wang_Mathieu_sfs sfs_test.o sfs_api.o disk_emu.o

sfs_api : sfs_api.o disk_emu.o disk_emu.h
	gcc -g -o sfs_api sfs_api.o disk_emu.o

sfs_api.o : sfs_api.c disk_emu.h
	gcc -c sfs_api.c

disk_emu.o : disk_emu.c
	gcc -c disk_emu.c

clean : 
	rm -rf *.o Wang_Mathieu_sfs

CC = $(LLVM)/bin/clang++

CCFLAGS = -I../include -DWWOS_HOST -g -O0

.PHONY: run clean

run: test_wwfs
	./test_wwfs

../libwwos/wwfs_host.o: ../libwwos/wwfs.cc
	$(CC) $(CCFLAGS) -DWWOS_HOST -c $< -o $@

test_wwfs.o: test_wwfs.cc
	$(CC) $(CCFLAGS) -DWWOS_HOST -c $< -o $@

test_wwfs: test_wwfs.o ../libwwos/wwfs_host.o
	$(CC) $(CCFLAGS) -o $@ $^

compile_flags.txt: Makefile
	echo $(CCFLAGS) "-xc++" | tr ' ' '\n' > $@

clean:
	rm -f test_wwfs.o ../libwwos/wwfs_host.o test_wwfs

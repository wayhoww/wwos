CC = $(LLVM)/bin/clang++

CCFLAGS = -I../include -DWWOS_HOST -g -O0

.PHONY: run clean

run: test_wwfs test_avl
	./test_wwfs
	./test_avl

../libwwos/wwfs_host.o: ../libwwos/wwfs.cc
	$(CC) $(CCFLAGS) -DWWOS_HOST -c $< -o $@

../libwwos/string_view_host.o: ../libwwos/string_view.cc
	$(CC) $(CCFLAGS) -DWWOS_HOST -c $< -o $@

test_wwfs.o: test_wwfs.cc
	$(CC) $(CCFLAGS) -DWWOS_HOST -c $< -o $@

test_wwfs: test_wwfs.o ../libwwos/wwfs_host.o ../libwwos/string_view_host.o
	$(CC) $(CCFLAGS) -o $@ $^

test_avl.o: test_avl.cc ../libwwos/string_view_host.o
	$(CC) $(CCFLAGS) -c $< -o $@

test_avl: test_avl.o

compile_flags.txt: Makefile
	echo $(CCFLAGS) "-xc++" | tr ' ' '\n' > $@

clean:
	rm -f test_wwfs.o ../libwwos/wwfs_host.o test_wwfs test_avl test_avl.o compile_flags.txt

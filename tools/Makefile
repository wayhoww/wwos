CC = $(LLVM)/bin/clang++

CCFLAGS = -I../include -DWWOS_HOST -O0 -g

.PHONY: all clean

all: wwfs compile_flags.txt

../libwwos/wwfs_host.o: ../libwwos/wwfs.cc
	$(CC) $(CCFLAGS) -DWWOS_HOST -c $< -o $@

../libwwos/string_view_host.o: ../libwwos/string_view.cc
	$(CC) $(CCFLAGS) -DWWOS_HOST -c $< -o $@

wwfs.o: wwfs.cc
	$(CC) $(CCFLAGS) -DWWOS_HOST -c $< -o $@

wwfs: wwfs.o ../libwwos/wwfs_host.o ../libwwos/string_view_host.o
	$(CC) $(CCFLAGS) -o $@ $^

compile_flags.txt: Makefile
	echo $(CCFLAGS) "-xc++" | tr ' ' '\n' > $@

clean:
	rm -f wwfs.o ../libwwos/wwfs_host.o wwfs

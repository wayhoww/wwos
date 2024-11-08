include ../../wwos.mk

CCFLAGS = -I/System/Volumes/Data/opt/homebrew/Cellar/qemu/9.0.2/include `pkg-config --cflags glib-2.0` -shared -fPIC -undefined dynamic_lookup

all: qemu_tracer.so compile_flags.txt

qemu_tracer.so: main.c
	$(CC) $(CCFLAGS) -xc $< -o $@

compile_flags.txt:
	echo $(CCFLAGS) | tr ' ' '\n' > $@

clean:
	rm -f qemu_tracer.so
	rm -f compile_flags.txt
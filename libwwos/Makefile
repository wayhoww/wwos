include ../wwos.mk

CCFLAGS += -I../include

.PHONY: all clean

clean:
	rm -f $(OBJS) libwwos.a libwwos_kernel.a

OBJS = alloc.o assert.o start.o wwfs.o runtime.o string_view.o syscall.o
OBJS_KERNEL = alloc_kernel.o assert_kernel.o wwfs_kernel.o string_view_kernel.o

%.o: %.cc
	$(CC) $(CCFLAGS) -DWWOS_APPLICATION -c $< -o $@

%_kernel.o: %.cc
	$(CC) $(CCFLAGS) -DWWOS_KERNEL -c $< -o $@

%_test.o: %.cc
	$(CC) $(CCFLAGS) -DWWOS_HOST -c $< -o $@

start.o: start.s
	$(AS) $(ASFLAGS) -c $< -o $@

libwwos.a: $(OBJS) 
	$(AR) rcs $@ $^

libwwos_kernel.a: $(OBJS_KERNEL)
	$(AR) rcs $@ $^

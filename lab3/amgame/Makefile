IMAGE = build/amgame
SRCS := $(shell find -L ./src/ -maxdepth 1 -regex ".*\\.\\(c\\|S\\)" -printf "%f\\n")
OBJS = $(addprefix build/, $(addsuffix .o, $(basename $(SRCS))))

### Compilation flags
CFLAGS   += -Wall -Werror -ggdb -Iinclude \
            -fno-asynchronous-unwind-tables -fno-builtin -fno-stack-protector \
            -Wno-main -m32 -fno-pic -fno-omit-frame-pointer -march=i386\
						-fcf-protection=none
ASFLAGS  += -Iinclude -m32 -fno-pic
LDFLAGS += -melf_i386 -N -Ttext-segment=0x00100000

$(shell mkdir -p build)

all: image
clean:
	rm -rf build/
.PHONY: all clean

image: $(OBJS) loader
	ld $(LDFLAGS) -o $(IMAGE).elf $(OBJS)
	(cat src/boot/bootblock.o; head -c 1024 /dev/zero; cat $(IMAGE).elf) > $(IMAGE)

loader:
	make -s -C src/boot

build/%.o: src/%.c
	gcc $(CFLAGS) -c -o $@ $<

build/%.o: src/%.S
	gcc $(ASFLAGS) -c -o $@ $<


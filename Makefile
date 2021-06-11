PATH += :/usr/local/pspdev/bin
CC=psp-gcc
CFLAGS=-G0 -O3 -c -Wall -Irenderer
CFLAGS += -fno-builtin-sin -fno-builtin-cos -fno-builtin-tan -fno-builtin-sqrt
OUT_FINAL=data.psp
OBJS=startup.o core.o renderer/matrix.o renderer/math.o renderer/render.o renderer/raster.o renderer/malloc.o

all: EBOOT.PBP

EBOOT.PBP: $(OUT_FINAL)
	pack-pbp EBOOT.PBP PARAM.SFO icon0.png NULL NULL NULL NULL $(OUT_FINAL) NULL

$(OUT_FINAL): $(OBJS)
	$(CC) $(OBJS) -nostartfiles -o $(OUT_FINAL)

startup.o: startup.S
	$(CC) -G0 -c -xassembler -O -o startup.o startup.S

renderer/%.o: renderer/%.c
	make -C renderer

%.o: %.c
	$(CC) $(CFLAGS) $<

clean:
	rm $(OBJS) $(OUT_FINAL) EBOOT.PBP


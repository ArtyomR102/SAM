OBJS = reciter.o sam.o render.o main.o debug.o

CC = gcc

# libsdl present
#CFLAGS =  -Wall -Os -DUSESDL `sdl-config --cflags` -I./SDL2/include/SDL2
#LFLAGS = `sdl-config --libs` -L./SDL2/lib

# libsdl present (Windows-corrected)
#CFLAGS =  -Wall -Os -DUSESDL -I./SDL2/include/SDL2
#LFLAGS = -L./SDL2/lib -lmingw32 -lSDL2main -lSDL2 -lm -lwinmm -limm32 -lversion -loleaut32 -lole32 -static-libgcc

# no libsdl present
CFLAGS = -Wall -Os
LFLAGS = 

sam: $(OBJS)
	$(CC) -o sam $(OBJS) $(LFLAGS)

%.o: src/%.c
	$(CC) $(CFLAGS) -c $<

package:
	tar -cvzf sam.tar.gz README.md Makefile sing src/

clean:
	rm -f *.o

archive:
	rm -f sam_windows.zip
	cd ..; zip SAM/sam_windows.zip	SAM/sam.exe SAM/SDL.dll SAM/README.md SAM/demos/*.bat

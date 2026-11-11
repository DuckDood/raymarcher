all: obj/ build/ obj/glad.c.o obj/main.c.o obj/vectors.c.o build/raymarcher 
OSMODE := l

obj/: 
ifeq (${OSMODE}, l)
	mkdir -p obj
else
	mkdir obj
endif

build/: 
ifeq (${OSMODE}, l)
	mkdir -p build
else
	mkdir build
endif

obj/glad.c.o: glad/src/glad.c 
ifeq (${OSMODE}, l)
	${CC} glad/src/glad.c -c -o obj/glad.c.o -Iglad/include
else
	${CC} glad/src/glad.c -c -o obj/glad.c.o -Iglad/include
endif


obj/main.c.o: src/main.c src/main.vert src/main.frag 
ifeq (${OSMODE}, l)
	${CC} src/main.c -c -o obj/main.c.o -Iglad/include -Iinclude -std=c23 -Wall -Wextra -Werror
else
	${CC} src/main.c -c -o obj/main.c.o -Iglad/include -Iinclude -std=c23 -Wall -Wextra -Werror
endif


obj/vectors.c.o: src/vectors.c src/main.vert src/main.frag 
ifeq (${OSMODE}, l)
	${CC} src/vectors.c -c -o obj/vectors.c.o -Iglad/include -Iinclude -std=c23 -Wall -Wextra -Werror
else
	${CC} src/vectors.c -c -o obj/vectors.c.o -Iglad/include -Iinclude -std=c23 -Wall -Wextra -Werror
endif


build/raymarcher: obj/glad.c.o obj/main.c.o obj/vectors.c.o src/main.vert src/main.frag 
ifeq (${OSMODE}, l)
	${CXX} obj/glad.c.o obj/main.c.o obj/vectors.c.o -o build/raymarcher -lSDL3 -lGL
else
	${CXX} obj/glad.c.o obj/main.c.o obj/vectors.c.o -o build/raymarcher -lSDL3 -lGL
endif


clean:
	rm -r obj
	rm -r build
.PHONY: clean


gencommands:
	mkdir emmgtemp
ifeq (${OSMODE}, l)
	clang glad/src/glad.c  -Iglad/include -MJ emmgtemp/0.json -fsyntax-only
else
	clang glad/src/glad.c  -Iglad/include -MJ emmgtemp/0.json -fsyntax-only
endif
ifeq (${OSMODE}, l)
	clang src/main.c  -Iglad/include -Iinclude -std=c23 -Wall -Wextra -Werror -MJ emmgtemp/1.json -fsyntax-only
else
	clang src/main.c  -Iglad/include -Iinclude -std=c23 -Wall -Wextra -Werror -MJ emmgtemp/1.json -fsyntax-only
endif
ifeq (${OSMODE}, l)
	clang src/vectors.c  -Iglad/include -Iinclude -std=c23 -Wall -Wextra -Werror -MJ emmgtemp/2.json -fsyntax-only
else
	clang src/vectors.c  -Iglad/include -Iinclude -std=c23 -Wall -Wextra -Werror -MJ emmgtemp/2.json -fsyntax-only
endif
# not cross platform here sad i think
	echo [ > emmgtemp/[
	echo ] > emmgtemp/]
	cat emmgtemp/[ emmgtemp/*.json emmgtemp/] > compile_commands.json
	rm -r emmgtemp

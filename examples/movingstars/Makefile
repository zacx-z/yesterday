CFLAGS+=-Wall -I../..
LIBS+=-lm -lc
ifeq ($(OS),Windows_NT)
	LIBS+= -lgdi32
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Darwin)
		LIBS+=-framework Cocoa
	else
		LIBS+=-lX11
	endif
endif

ifdef DEBUG
	CFLAGS+=-g
endif

ifndef NO_FPS_CAP
	ifndef FPS
		FPS=60
	endif
endif

ifdef FPS
	CFLAGS+=-DFPS=$(FPS)
endif

OUTPUT=movingstars
SRC=movingstars.c

.PHONY: clean

$(OUTPUT): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(OUTPUT) $(LIBS)

clean:
	@if [ -f $(OUTPUT) ]; then \
		rm $(OUTPUT); \
	fi

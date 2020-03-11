# Makefile -----------------------------
#       - Geoffrey Meier
#       - Project 4
# --------------------------------------

CFLAGS = -Wall -std=c99
DEBUG = -g -O0
CC = gcc

# Specify the executable and object files
EXE = smash
OBJS = commands.o history.o


all: $(EXE) Makefile

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(EXE): smash.o smashLib.a
	$(CC) $(CFLAGS) $^ -o $@ 

smashLib.a: $(OBJS)
	ar r $@ $?

clean: 
	rm -f $(EXE) *.a *.o *~ rules.d

debug: CFLAGS += $(DEBUG)
debug: clean $(EXE)


valgrind: debug
	valgrind --leak-check=full --show-leak-kinds=all --child-silent-after-fork=yes --trace-children=no $(EXE)
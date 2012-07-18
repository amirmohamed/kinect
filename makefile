.PHONY: clean, mrproper
.SUFFIXES:

CC = g++
EXEC = oneshot
DEBUG = no
FLAGS = -c -msse3 -O2 -DNDEBUG  `pkg-config --cflags --libs opencv`
INCLUDES = -I./lib -I/usr/include/ni -I/usr/include/nite
LINKS = -lOpenNI -lXnVNite_1_5_2


ifeq ($(DEBUG),yes)
	CFLAGS = -g -W -Wall
else
	CFLAGS = 
endif

all : $(EXEC)
ifeq ($(DEBUG),yes)
	@echo "Generation in debug mode"
else
	@echo "Generate in release mode"
endif

$(EXEC): main.o
	$(CC) -o ./$(EXEC) ./main.o $(CFLAGS) $(LINKS) 

main.o: main.cpp
	$(CC) $(FLAGS) $(INCLUDES) -o main.o main.cpp

exec : 
	@echo -n "[TEST] Executing ..."
	@sleep 0.4
	./$(EXEC)

clean:
	rm -rf *.o

mrproper: clean
	rm -rf oneshot

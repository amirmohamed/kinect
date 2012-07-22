.PHONY: clean, mrproper
.SUFFIXES:

CC = g++
EXEC = usbControl
DEBUG = no
FLAGS = -c -msse3 -O2 -DNDEBUG     # Not neccesary
INCLUDES = -I/usr/include/ni
LINKS = -lOpenNI


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

$(EXEC): example.o
	$(CC) -o ./$(EXEC) ./example.o $(CFLAGS) $(LINKS) 

example.o: example.cpp
	$(CC) $(FLAGS) $(INCLUDES) -o example.o example.cpp

exec : 
	@echo -n "[TEST] Executing ..."
	@sleep 0.4
	./$(EXEC)

clean:
	rm -rf *.o

mrproper: clean
	rm -rf handTracking

.PHONY: clean, mrproper
.SUFFIXES:

CC = javac
DEBUG = yes
FLAGS = -g
FILE = KinectServer
LINKS = -cp ./core.jar:./net.jar:.


ifeq ($(DEBUG),yes)
	CFLAGS = -g -W -Wall
else
	CFLAGS = 
endif

all : 
	$(CC) $(LINKS) $(FILE).java

exec : 
	@echo -n "[TEST] Executing: "
	java $(LINKS) $(FILE)

clean: 
	rm -rf $(FILE).class

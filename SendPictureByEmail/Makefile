CC  = gcc
CPP = g++
TARGET = home_monitor
OBJECTS = $(patsubst %.cpp,%.o, $(patsubst %.c,%.o, $(wildcard *.c)) $(wildcard *.cpp))
LIB = -lpthread

$(TARGET): $(OBJECTS)
	@echo "** Linking Executable"
	$(CPP) $(OBJECTS) $(LIB) -o $(TARGET)

clean:
	@rm -f *.o *~

%.o: %.c
	@echo "** Compiling C Source"
	$(CC) -c $<

%.o: %.cpp
	@echo "** Compiling CPP Source"
	$(CPP) -c $<



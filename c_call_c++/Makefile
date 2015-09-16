CC  = gcc
CPP = g++
TARGET = c_call_c++

OBJECTS = $(patsubst %.c,%.o, $(patsubst %.cpp,%.o, $(wildcard *.cpp)) $(wildcard *.c))

$(TARGET): $(OBJECTS)
	@echo "** Linking Executable"
	$(CPP) $(OBJECTS) -o $(TARGET)


%.o: %.c
	@echo "** Compiling C Source"
	$(CC) -c $<

%.o: %.cpp
	@echo "** Compiling CPP Source"
	$(CPP) -c $<
clean:
	@rm -f *.o *~

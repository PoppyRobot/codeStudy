CC  = gcc
CPP = g++
TARGET = event_loop_demo 
OBJECTS = $(patsubst %.cpp,%.o, $(wildcard *.cpp))

$(TARGET): $(OBJECTS)
	@echo "** Linking Executable"
	$(CPP) $(OBJECTS) -o $(TARGET) -lpthread

%.o: %.cpp
	@echo "** Compiling CPP Source"
	$(CPP) -c $< -DDEBUG 

clean:
	@rm -f *.o *~

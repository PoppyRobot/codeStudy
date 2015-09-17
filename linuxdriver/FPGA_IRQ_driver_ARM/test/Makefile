CC = arm-linux-gcc

TARGET = fpga_irq_test
OBJECTS = $(patsubst %.c,%.o, $(wildcard *.c))

$(TARGET): $(OBJECTS)
	@echo "** Linking Executable"
	$(CC) $(OBJECTS) -o $(TARGET)

%.o: %.c
	@echo "** Compiling C Source"
	$(CC) -c $<

clean:
	@rm -f *.o *~

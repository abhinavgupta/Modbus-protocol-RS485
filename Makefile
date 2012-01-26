# compiler and its flags
CC = gcc
FLAGS = -Wall

all: mbm

# main application
mbm: mbm.o modbus_rtu.o
	$(CC) $(FLAGS) -o mbm mbm.o modbus_rtu.o

mbm.o: mbm.c
	$(CC) $(FLAGS) -c mbm.c

modbus_rtu.o: modbus_rtu.c modbus_rtu.h
	$(CC) $(CFLAGS) -c modbus_rtu.c
	

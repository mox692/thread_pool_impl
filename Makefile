PRG    = thread_pool_impl
OBJ    = main.o errHandle.o

CFLAGS = -g -Wall
LFLAGS =

CC     =  gcc $(CFLAGS)

$(PRG) : $(OBJ) 
	$(CC) -o $@ $^ $(LFLAGS)

.c.o:
	$(CC) -c $<

clean:
	rm -rf $(PRG) $(OBJ)
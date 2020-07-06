CC =gcc
#LDFLAGS =-lpthread
LDFLAGS =-pthread
CFLAGS =-W -Wall -ansi -pedantic -Iinclude 

DIR=/tmp/3261765
DIR_1=.
BIN=$(DIR)/bin/
OBJ=$(DIR)/obj/
INCLUDE=$(DIR_1)/include/
LIB=$(DIR)/lib/
SRC=$(DIR_1)/src/


.SUFFIXES:
.PHONY:  test test_1 test_2 test_3

test: $(BIN)server
	/tmp/3261765/bin/server 2000 3 10000

test_1: $(BIN)server
	/tmp/3261765/bin/server 2001 3 10000

test_2: $(BIN)server
	/tmp/3261765/bin/server 2002 3 10000

test_3: $(BIN)server
	/tmp/3261765/bin/server 2003 3 10000

clean: 
	rm -rf $(OBJ)*.o $(BIN)*

$(BIN)%: $(OBJ)%.o 
	@if [ -d $(BIN) ]; then : ; else mkdir -p $(BIN); fi
	$(CC) $(LDFLAGS) -o $@ $^

$(OBJ)%.o: $(SRC)%.c $(INCLUDE)%.h
	@if [ -d $(OBJ) ]; then : ; else mkdir -p $(OBJ); fi
	$(CC) $(CFLAGS) -o $@ -c $<

$(INCLUDE)%.h:
	@if [ -d $(INCLUDE) ]; then : ; else mkdir -p $(INCLUDE); fi


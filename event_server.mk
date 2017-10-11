OPT = -g -O0
CXX = g++ $(OPT)

INC = -I. -I/usr/local/include
LIB = -pthread -ldl -lz -lrt -levent

##################################
BIN = event_server
OBJS = \
	event_server.o \
	common.o \

all: $(BIN)

$(BIN): $(OBJS)
	rm -f $@
	$(CXX) -o $@ $(INC) $^ $(LIB)

%.o: %.cpp
	$(CXX) $(INC) -c -o $@ $<

clean:
	rm -f *.o $(BIN) $(OBJS)

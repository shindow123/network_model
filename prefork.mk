OPT = -g -O0
CXX = g++ $(OPT)

INC = -I.
LIB = -pthread -ldl -lz -lrt

##################################
BIN = prefork
OBJS = \
	prefork.o \
	common.o \

all: $(BIN)

$(BIN): $(OBJS)
	rm -f $@
	$(CXX) -o $@ $(INC) $^ $(LIB)

%.o: %.cpp
	$(CXX) $(INC) -c -o $@ $<

clean:
	rm -f *.o $(BIN) $(OBJS)

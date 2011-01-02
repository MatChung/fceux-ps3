TARGET = stdio_serv
TEST_TARGET = test_stdio

OBJECTS = stdio_serv.o IOHandler.o ../../network/TCPSocket.o ../../network/Socket.o ../../network/Poller.o ../../threads/thread.o

TEST_OBJ = test.o ../../network/TCPSocket.o ../../network/Socket.o ../../network/Poller.o ../../threads/thread.o ../../network-stdio/net_stdio.o

LIBS = -lpthread

all: $(TARGET)

CXXFLAGS = -O3


%.o: %.cpp 
	$(CXX) $(CXXFLAGS) -o $@ -c $<


$(TARGET): $(OBJECTS)
	$(CXX) -o $@ $(OBJECTS) $(LIBS)


test: $(TEST_TARGET)

$(TEST_TARGET): $(TEST_OBJ)
	$(CXX) -o $@ $(TEST_OBJ) -Wl,-wrap,fopen -Wl,-wrap,fclose -Wl,-wrap,fwrite -Wl,-wrap,fread -Wl,-wrap,fseek -Wl,-wrap,ftell -Wl,-wrap,fflush -Wl,-wrap,rewind -Wl,-wrap,fgetpos -Wl,-wrap,fsetpos -Wl,-wrap,setbuf -Wl,-wrap,setvbuf -Wl,-wrap,ungetc -Wl,-wrap,feof


clean:
	rm -f $(OBJECTS) $(TARGET) $(TEST_TARGET) $(TEST_OBJ)

.PHONY: all clean test

CXX      = g++
CXXFLAGS = -std=c++17 -Wall -D_WIN32_WINNT=0x0A00
LDLIBS   = -lcrypto -lws2_32 -lcrypt32
SRC      = Source/web_main.cpp Source/main.cpp Source/Ballot.cpp Source/Blockchain.cpp Source/BPlusTree.cpp Source/Digest.cpp Source/Tests.cpp

web_server: $(SRC) $(wildcard include/*.h) include/HashTable.tpp
	$(CXX) $(CXXFLAGS) $(SRC) -o web_server $(LDLIBS)

run: web_server
	./web_server

clean:
	rm -f web_server web_server.exe

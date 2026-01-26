CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -pthread
LDFLAGS = -lrt -pthread

publisher: publisher.cpp message.h ring_buffer.h
	$(CXX) $(CXXFLAGS) -o publisher publisher.cpp $(LDFLAGS)

shm_consumer: shm_consumer.cpp message.h ring_buffer.h logger.h
	$(CXX) $(CXXFLAGS) -o shm_consumer shm_consumer.cpp $(LDFLAGS)

tcp_consumer: tcp_consumer.cpp message.h logger.h
	$(CXX) $(CXXFLAGS) -o tcp_consumer tcp_consumer.cpp $(LDFLAGS)

clean:
	rm -f publisher shm_consumer tcp_consumer

.PHONY: clean

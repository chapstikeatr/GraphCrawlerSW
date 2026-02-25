CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2

# RapidJSON header location (local copy)
RAPIDJSON_INC = ../rapidjson/include

# libcurl link (works on CentOS/Centaurus if curl-devel is installed)
LDFLAGS = -lcurl -pthread

TARGET = paraLC
SRC = parallel_level_client.cpp

#all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -I$(RAPIDJSON_INC) $(SRC) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)

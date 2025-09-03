CXX ?= g++
CXXFLAGS := -std=c++17 $(shell sdl2-config --cflags) $(shell pkg-config --cflags SDL2_ttf fftw3)
LDFLAGS := $(shell sdl2-config --libs) $(shell pkg-config --libs SDL2_ttf fftw3)

SRC := main.cpp
OBJ := $(SRC:.cpp=.o)
TARGET := iradio

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) -o $@ $(OBJ) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJ)

.PHONY: all clean

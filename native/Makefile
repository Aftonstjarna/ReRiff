CXX ?= c++
CXXFLAGS ?= -O2
JSON_INCLUDE ?= /usr/include

.PHONY: all
all: ../libreriff-catalog.so

../libreriff-catalog.so: song_catalog.cpp
	$(CXX) $(CXXFLAGS) -std=c++17 -Wall -Wextra -shared -fPIC -I$(JSON_INCLUDE) $< -o $@

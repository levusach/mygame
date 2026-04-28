CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra
LDFLAGS ?= -lncursesw
WIN_CXX ?= x86_64-w64-mingw32-g++
WIN_LDFLAGS ?= -lncursesw

world: main.cpp src/*.cpp src/*.hpp
	$(CXX) $(CXXFLAGS) main.cpp $(LDFLAGS) -o world

worldpodvoh: main.cpp src/*.cpp src/*.hpp
	$(CXX) $(CXXFLAGS) -DPODVOH_ADMIN main.cpp $(LDFLAGS) -o worldpodvoh

world.exe: main.cpp src/*.cpp src/*.hpp
	$(WIN_CXX) $(CXXFLAGS) main.cpp $(WIN_LDFLAGS) -o world.exe

worldpodvoh.exe: main.cpp src/*.cpp src/*.hpp
	$(WIN_CXX) $(CXXFLAGS) -DPODVOH_ADMIN main.cpp $(WIN_LDFLAGS) -o worldpodvoh.exe

win: world.exe
win-podvoh: worldpodvoh.exe
podvoh: worldpodvoh

clean:
	rm -f world world.exe worldpodvoh worldpodvoh.exe

.PHONY: clean win win-podvoh podvoh

CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra
LDFLAGS ?= -lncursesw
WIN_CXX ?= x86_64-w64-mingw32-g++
WIN_LDFLAGS ?= -lncursesw -lws2_32

world: main.cpp src/*.cpp src/*.hpp
	$(CXX) $(CXXFLAGS) main.cpp $(LDFLAGS) -o world

worldpodvoh: main.cpp src/*.cpp src/*.hpp
	$(CXX) $(CXXFLAGS) -DPODVOH_ADMIN main.cpp $(LDFLAGS) -o worldpodvoh

trainer: trainer.cpp src/*.cpp src/*.hpp
	$(CXX) $(CXXFLAGS) trainer.cpp $(LDFLAGS) -o trainer

agent_replay: agent_replay.cpp src/*.cpp src/*.hpp
	$(CXX) $(CXXFLAGS) agent_replay.cpp $(LDFLAGS) -o agent_replay

bot_panel: bot_panel.cpp
	$(CXX) $(CXXFLAGS) bot_panel.cpp $(LDFLAGS) -o bot_panel

home_builder: home_builder.cpp src/*.cpp src/*.hpp
	$(CXX) $(CXXFLAGS) home_builder.cpp $(LDFLAGS) -o home_builder

world.exe: main.cpp src/*.cpp src/*.hpp
	$(WIN_CXX) $(CXXFLAGS) main.cpp $(WIN_LDFLAGS) -o world.exe

worldpodvoh.exe: main.cpp src/*.cpp src/*.hpp
	$(WIN_CXX) $(CXXFLAGS) -DPODVOH_ADMIN main.cpp $(WIN_LDFLAGS) -o worldpodvoh.exe

win: world.exe
win-podvoh: worldpodvoh.exe
podvoh: worldpodvoh

clean:
	rm -f world world.exe worldpodvoh worldpodvoh.exe trainer agent_replay bot_panel home_builder

.PHONY: clean win win-podvoh podvoh

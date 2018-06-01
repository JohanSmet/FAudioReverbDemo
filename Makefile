INCDIRS = -I libs/FACT/src -I src/gl3w `sdl2-config --cflags`
LIBDIRS = `sdl2-config --libs` -L libs/FACT

CXXFLAGS += -g -Wall -fpic -fPIC -std=c++11 -std=gnu++11 $(INCDIRS)
CFLAGS += -g -Wall -fpic -fPIC $(INCDIRS)

LIBS = -lFAudio -lGL -ldl

CXXSRC =	src/audio.cpp \
			src/audio_faudio.cpp \
			src/main.cpp \
			src/main_gui.cpp \
			src/imgui/imgui.cpp \
			src/imgui/imgui_draw.cpp \
			src/imgui/imgui_impl_sdl_gl3.cpp 

CCSRC = 	src/gl3w/GL/gl3w.c

OBJ = $(CXXSRC:%.cpp=%.o) $(CCSRC:%.c=%.o)

TARGET = FAudioReverbDemo

$(TARGET): $(OBJ) FACT
	$(CXX) -o $@ -Wl,-rpath,./libs/FACT $(OBJ) $(LIBDIRS) $(LIBS)

FACT:
	$(MAKE) -C libs/FACT

%.o: %.cpp %.c
	$(CXX) -c -o $@ $< 

.PHONY: clean

clean:
	rm -f $(OBJ) $(TARGET)
	$(MAKE) -C libs/FACT clean

# Makefile for windows

CC := g++
CFLAGS := -Wall

# Objects
DEBUG := Debugger.o
RELEASE := main.o
SRCOBJS := Node.o Map.o Algorithms.o Controller.o StartingPoints.o
IMGUIOBJS := imgui.o imgui_draw.o imgui_tables.o imgui_widgets.o imgui-SFML.o
DEBUGOBJS := ${DEBUG} ${SRCOBJS} ${IMGUIOBJS}
RELEASEOBJS := ${RELEASE} ${SRCOBJS} ${IMGUIOBJS}

# Dependencies and Paths
INCLUDES := -Ivendor/SFML/include -Ivendor/imgui
LIBS := -Lvendor/SFML/lib -lsfml-window -lsfml-graphics -lsfml-system -lopengl32
SRC := src
IMGUISRC := vendor/imgui

# Output
TARGET := Simulator

debug: DebugInit clean
release: ReleaseInit clean

# Debug Target
DebugInit: ${DEBUGOBJS}
	${CC} ${CFLAGS} $^ ${LIBS}

# Release Target
ReleaseInit: ${RELEASEOBJS}
	${CC} ${CFLAGS} $^ ${LIBS}

# src
Debugger.o: ${SRC}/Debugger.cpp
	${CC} ${CFLAGS} ${INCLUDES} -c $<

main.o: ${SRC}/main.cpp
	${CC} ${CFLAGS} ${INCLUDES} -c $<

Node.o: ${SRC}/Node.cpp
	${CC} ${CFLAGS} ${INCLUDES} -c $<

Map.o: ${SRC}/Map.cpp
	${CC} ${CFLAGS} ${INCLUDES} -c $<

Algorithms.o: ${SRC}/Algorithms.cpp
	${CC} ${CFLAGS} ${INCLUDES} -c $<

Controller.o: ${SRC}/Controller.cpp
	${CC} ${CFLAGS} ${INCLUDES} -c $<

StartingPoints.o: ${SRC}/StartingPoints.cpp
	${CC} ${CFLAGS} ${INCLUDES} -c $<

# ImGui
imgui.o: ${IMGUISRC}/imgui.cpp
	${CC} ${CFLAGS} ${INCLUDES} -c $<

imgui_demo.o: ${IMGUISRC}/imgui_demo.cpp
	${CC} ${CFLAGS} ${INCLUDES} -c $<

imgui_draw.o: ${IMGUISRC}/imgui_draw.cpp
	${CC} ${CFLAGS} ${INCLUDES} -c $<

imgui_tables.o: ${IMGUISRC}/imgui_tables.cpp
	${CC} ${CFLAGS} ${INCLUDES} -c $<

imgui_widgets.o: ${IMGUISRC}/imgui_widgets.cpp
	${CC} ${CFLAGS} ${INCLUDES} -c $<

imgui-SFML.o: ${IMGUISRC}/imgui-SFML.cpp
	${CC} ${CFLAGS} ${INCLUDES} -c $<
	

# Clean objs
clean:
	del *.o
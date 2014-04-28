cmake_minimum_required(VERSION 2.8)

file(GLOB SRC
	"src/*.h"
	"src/*.c"
)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -lm -g")

add_executable(dt ${SRC})

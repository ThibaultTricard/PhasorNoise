cmake_minimum_required (VERSION 3.0)
project( "Phasor Noise" )


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

SET(RAYTRACING OFF CACHE BOOL "" FORCE)
add_subdirectory(LavaCake)

clearShader()
set(SRC_FILE
	src/main.cpp
	src/shaders/phasor.frag
	src/shaders/phasor.vert
)

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/shaders")

addShader(
  "${CMAKE_CURRENT_LIST_DIR}/src/shaders/phasor.vert"
  "${CMAKE_BINARY_DIR}/shaders/phasor.vert.spv"
)

addShader(
  "${CMAKE_CURRENT_LIST_DIR}/src/shaders/phasor.frag"
  "${CMAKE_BINARY_DIR}/shaders/phasor.frag.spv"
)



add_executable(PhasorNoise ${SRC_FILE})
target_link_libraries(PhasorNoise LavaCake)

AutoSPIRV(PhasorNoise)




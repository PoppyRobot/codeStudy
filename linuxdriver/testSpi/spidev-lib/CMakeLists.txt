cmake_minimum_required(VERSION 2.6)

SET(CMAKE_C_COMPILER "arm-linux-gcc")

EXEC_PROGRAM(svnversion  OUTPUT_VARIABLE LS_OUTPUT RETURN_VALUE LS_RVALUE)
message(${LS_OUTPUT} ${LS_RVALUE})
project(spidev-lib)
set(CMAKE_C_FLAGS "-g -Wall")

include_directories(src/)

add_definitions(-DTIME_TRACE)
add_library(spidev-lib src/spidev_lib.c)
add_executable(
     spi-testc sample/spidev-testc.c
)
target_link_libraries( spi-testc spidev-lib)

add_dependencies(spi-testc spidev-lib)

INSTALL(TARGETS  spidev-lib
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
INSTALL( FILES src/spidev_lib.h DESTINATION include)


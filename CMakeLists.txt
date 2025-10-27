cmake_minimum_required(VERSION 3.13)
project(satcon_raspi CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(satcon_raspi
    satcon_raspi.cpp
    GPIOInterrupt.cpp
    parser.cpp
    ICM20948_USER.cpp
    ICM20948/ICM20948.cpp
    ICM20948/ICM20948_raspi.cpp)

target_include_directories(satcon_raspi PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/ICM20948
    ${CMAKE_CURRENT_SOURCE_DIR}/Vector3D)

# Link libgpiod and pthread for event handling
find_library(GPIOD_LIB gpiod)
if(NOT GPIOD_LIB)
    message(FATAL_ERROR "libgpiod not found. Install with: sudo apt install -y libgpiod-dev")
endif()

target_link_libraries(satcon_raspi PRIVATE ${GPIOD_LIB} pthread)

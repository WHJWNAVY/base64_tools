cmake_minimum_required(VERSION 3.3.2)

project(base64 C)

set(B64_EXE_NAME base64)
set(B16_EXE_NAME base16)

set(CMAKE_BUILD_TYPE Release)

file(GLOB B64_SRCS src/main.c src/base64.c)
file(GLOB B16_SRCS src/base16.c)

add_executable(${B64_EXE_NAME} ${B64_SRCS})
add_executable(${B16_EXE_NAME} ${B16_SRCS})
target_include_directories(${PROJECT_NAME} PUBLIC
                                           ${PROJECT_SOURCE_DIR}
                                           ${PROJECT_SOURCE_DIR}/src)

install(TARGETS ${B64_EXE_NAME} RUNTIME DESTINATION bin)
install(TARGETS ${B16_EXE_NAME} RUNTIME DESTINATION bin)

cmake_minimum_required(VERSION 3.5...4.0)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "Minimum policy version for all subprojects")

project(NicoCraft LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

include(FetchContent)

############################
### Library dependencies ###
############################

FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.2
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(SFML)

FetchContent_Declare(
	glm
	GIT_REPOSITORY	https://github.com/g-truc/glm.git
        GIT_TAG 	1.0.3
)
FetchContent_MakeAvailable(glm)

add_library(common INTERFACE)
target_compile_features(common INTERFACE cxx_std_20)
target_compile_options(common INTERFACE
  $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>: -Wall -DGL_SILENCE_DEPRECATION>
  $<$<CXX_COMPILER_ID:MSVC>: /W4>
)

target_include_directories(common INTERFACE include)

# Find OpenGL
find_package(OpenGL REQUIRED)

#find coregraphics for raw mouse events in macos
if(APPLE)
    find_library(CORE_GRAPHICS_LIB CoreGraphics REQUIRED)
endif()



###############################
### Sorgenti e Tappe 1-10   ###
###############################

# nome eseguibile, nome file
add_executable(Tappa01 Tappa01/NicoCraft.cc) 
target_link_libraries(Tappa01 PRIVATE common SFML::Window SFML::Graphics OpenGL::GL glm::glm-header-only)

add_executable(Tappa02 Tappa02/NicoCraft.cc)
target_link_libraries(Tappa02 PRIVATE common SFML::Window SFML::Graphics OpenGL::GL glm::glm-header-only)

add_executable(Tappa03 Tappa03/NicoCraft.cc) 
target_link_libraries(Tappa03 PRIVATE common SFML::Window SFML::Graphics OpenGL::GL glm::glm-header-only)





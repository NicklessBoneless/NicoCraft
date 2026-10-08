cmake_minimum_required(VERSION 3.5...4.0)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "Minimum policy version for all subprojects")

project(NicoCraft LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Executables)

#Necessario per i generatori multi-configurazione (Visual Studio, Xcode): altrimenti
#aggiungono da soli una sottocartella per configurazione (Debug/ o Release/)
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_SOURCE_DIR}/Executables)
endforeach()

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

FetchContent_Declare(ImGui
    GIT_REPOSITORY https://github.com/ocornut/imgui
    GIT_TAG v1.91.9
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(ImGui)
set(IMGUI_DIR ${imgui_SOURCE_DIR})

set(IMGUI_SFML_FIND_SFML OFF)
FetchContent_Declare(ImGui-SFML
    GIT_REPOSITORY https://github.com/SFML/imgui-sfml
    GIT_TAG v3.0
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(ImGui-SFML)

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
    target_link_libraries(common INTERFACE ${CORE_GRAPHICS_LIB})
endif()

###############################
### Sorgenti e Tappe 1-15   ###
###############################

# nome eseguibile, nome file
add_executable(Tappa01 Tappa01/NicoCraft.cc) 
target_link_libraries(Tappa01 PRIVATE common SFML::Window SFML::Graphics OpenGL::GL glm::glm-header-only)

add_executable(Tappa02 Tappa02/NicoCraft.cc)
target_link_libraries(Tappa02 PRIVATE common SFML::Window SFML::Graphics OpenGL::GL glm::glm-header-only)

add_executable(Tappa03 Tappa03/NicoCraft.cc) 
target_link_libraries(Tappa03 PRIVATE common SFML::Window SFML::Graphics OpenGL::GL glm::glm-header-only)

add_executable(Tappa04 Tappa04/NicoCraft.cc) 
target_link_libraries(Tappa04 PRIVATE common SFML::Window SFML::Graphics OpenGL::GL glm::glm-header-only)

add_executable(Tappa05 Tappa05/NicoCraft.cc) 
target_link_libraries(Tappa05 PRIVATE common SFML::Window SFML::Graphics OpenGL::GL glm::glm-header-only)

add_executable(Tappa06 Tappa06/NicoCraft.cc) 
target_link_libraries(Tappa06 PRIVATE common SFML::Window SFML::Graphics OpenGL::GL glm::glm-header-only)

add_executable(Tappa07 Tappa07/NicoCraft.cc) 
target_link_libraries(Tappa07 PRIVATE common SFML::Window SFML::Graphics OpenGL::GL glm::glm-header-only)

add_executable(Tappa08 Tappa08/NicoCraft.cc) 
target_link_libraries(Tappa08 PRIVATE common SFML::Window SFML::Graphics OpenGL::GL glm::glm-header-only)

add_executable(Tappa09 Tappa09/NicoCraft.cc) 
target_link_libraries(Tappa09 PRIVATE common SFML::Window SFML::Graphics OpenGL::GL glm::glm-header-only)

add_executable(Tappa10 Tappa10/NicoCraft.cc) 
target_link_libraries(Tappa10 PRIVATE common SFML::Window SFML::Graphics OpenGL::GL glm::glm-header-only)

add_executable(Tappa11 Tappa11/NicoCraft.cc) 
target_link_libraries(Tappa11 PRIVATE common SFML::Window SFML::Graphics OpenGL::GL glm::glm-header-only)

add_executable(Tappa12 Tappa12/NicoCraft.cc) 
target_link_libraries(Tappa12 PRIVATE common SFML::Window SFML::Graphics OpenGL::GL glm::glm-header-only)

add_executable(Tappa13 Tappa13/NicoCraft.cc ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
target_link_libraries(Tappa13 PRIVATE common SFML::Window SFML::Graphics OpenGL::GL glm::glm-header-only ImGui-SFML::ImGui-SFML)
target_include_directories(Tappa13 PRIVATE ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_compile_definitions(Tappa13 PRIVATE IMGUI_IMPL_OPENGL_LOADER_GLAD2)

add_executable(Tappa14 Tappa14/NicoCraft.cc ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
target_link_libraries(Tappa14 PRIVATE common SFML::Window SFML::Graphics OpenGL::GL glm::glm-header-only ImGui-SFML::ImGui-SFML)
target_include_directories(Tappa14 PRIVATE ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_compile_definitions(Tappa14 PRIVATE IMGUI_IMPL_OPENGL_LOADER_GLAD2)

add_executable(Tappa15 Tappa15/NicoCraft.cc ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
target_link_libraries(Tappa15 PRIVATE common SFML::Window SFML::Graphics OpenGL::GL glm::glm-header-only ImGui-SFML::ImGui-SFML)
target_include_directories(Tappa15 PRIVATE ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_compile_definitions(Tappa15 PRIVATE IMGUI_IMPL_OPENGL_LOADER_GLAD2)

add_executable(Tappa16 Tappa16/NicoCraft.cc ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
target_link_libraries(Tappa16 PRIVATE common SFML::Window SFML::Graphics OpenGL::GL glm::glm-header-only ImGui-SFML::ImGui-SFML)
target_include_directories(Tappa16 PRIVATE ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_compile_definitions(Tappa16 PRIVATE IMGUI_IMPL_OPENGL_LOADER_GLAD2)



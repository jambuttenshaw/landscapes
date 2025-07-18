cmake_minimum_required(VERSION 3.18)

project(landscapes)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

if (MSVC)
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /D_ITERATOR_DEBUG_LEVEL=1")
endif()

option(DONUT_WITH_ASSIMP "" OFF)
option(DONUT_WITH_DX11 "" OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set(DONUT_SHADERS_OUTPUT_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders/framework")

add_subdirectory(donut)

include(donut/compileshaders.cmake)

macro(GroupSources curdir)
    file(GLOB children RELATIVE ${PROJECT_SOURCE_DIR}/${curdir} ${PROJECT_SOURCE_DIR}/${curdir}/*)
    foreach(child ${children})
        if(IS_DIRECTORY ${PROJECT_SOURCE_DIR}/${curdir}/${child})
            GroupSources(${curdir}/${child})
        else()
            string(REPLACE "/" "\\" groupname ${curdir})
            source_group(${groupname} FILES ${PROJECT_SOURCE_DIR}/${curdir}/${child})
        endif()
    endforeach()
endmacro()

file(GLOB_RECURSE shaders "shaders/*.hlsl" "shaders/*.hlsli" "shaders/*.h")
GroupSources("shaders")
file(GLOB_RECURSE sources "source/*.cpp" "source/*.h")
GroupSources("source")

set(project landscapes)
set(folder "Landscapes")

set(SHADER_INCLUDE_PATHS "")
list(APPEND SHADER_INCLUDE_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/Shared")
list(APPEND SHADER_INCLUDE_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/vendor/libcbt/hlsl")
list(APPEND SHADER_INCLUDE_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/vendor/libleb/hlsl")

set(shadermake_include_dirs "")
foreach(include_dir ${SHADER_INCLUDE_PATHS})
    set(shadermake_include_dirs "${shadermake_include_dirs} -I ${include_dir}")
endforeach()

donut_compile_shaders_all_platforms(
    TARGET ${project}_shaders
    PROJECT_NAME "Landscapes"
    CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/shaders/shaders.cfg
    SHADERMAKE_OPTIONS ${shadermake_include_dirs}
    SOURCES ${shaders}
    FOLDER ${folder}
    OUTPUT_BASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders/${project}
)

add_executable(${project} WIN32 ${sources})

target_include_directories(${project} PUBLIC "source")
target_include_directories(${project} PUBLIC "shaders/Shared")
target_include_directories(${project} PUBLIC "vendor")

target_link_libraries(${project} donut_app donut_engine donut_render)
add_dependencies(${project} ${project}_shaders)
set_target_properties(${project} PROPERTIES FOLDER ${folder})

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3 /MP")
endif()

## This configures the Visual Studio project with custom include directories to help Intellisense etc find the correct includes
## I got this solution from: https://stackoverflow.com/questions/47475731/cmake-include-directories-for-custom-target-type
if (CMAKE_GENERATOR MATCHES "Visual Studio")
    set(project landscapes_shaders)

    list(APPEND SHADER_INCLUDE_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/Donut/include/")

    file(
            WRITE "${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.vcxproj.user"
            "<?xml version=\"1.0\" encoding=\"utf-8\"?>
            <Project ToolsVersion=\"Current\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">
            <PropertyGroup>
                <IncludePath>$(IncludePath);@SHADER_INCLUDE_PATHS@</IncludePath>
            </PropertyGroup>
            </Project>"
        )

    configure_file(
        "${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.vcxproj.user"
        "${CMAKE_CURRENT_BINARY_DIR}/${project}.vcxproj.user"
        @ONLY
    )
endif()

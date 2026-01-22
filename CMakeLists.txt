cmake_minimum_required (VERSION 3.16)

project (JHU_CS_605.667)
add_definitions(-DSRC_ROOT=${CMAKE_SOURCE_DIR}/)

#################################
# Determine the target platform #
#################################

if(WIN32)
    set(BUILD_MS_WINDOWS 1)
    add_definitions(-DBUILD_WINDOWS)
    add_definitions(-DNOMINMAX)
    set(WINDOWS_ARCH "x64" CACHE STRING "Windows Architecture")
    set_property(CACHE WINDOWS_ARCH PROPERTY STRINGS "x64;x86;arm64")
    message(STATUS "Building for Windows:" ${WINDOWS_ARCH})
elseif(APPLE)
    set(BUILD_MACOS 1)
    add_definitions(-DBUILD_MACOS)
    message(STATUS "Building for MacOS")
elseif(UNIX AND NOT APPLE)
    set(BUILD_LINUX 1)
    add_definitions(-DBUILD_LINUX)
    message(STATUS "Building for Linux")
endif()

if(BUILD_MS_WINDOWS)
    set(SRC_DIR_WIN \"$(ProjectDir)..\")
    set(RESOURCE_DIR_WIN \"$(ProjectDir)..\\resources\" )
    add_definitions(-DSRC_DIR=${SRC_DIR_WIN})
    add_definitions(-DRESOURCE_DIR=${RESOURCE_DIR_WIN})
    message("SRC_DIR_WIN: " ${SRC_DIR_WIN})
    message("RESOURCE_DIR_WIN: " ${RESOURCE_DIR_WIN})
else()
    add_definitions(-DSRC_DIR=${CMAKE_SOURCE_DIR})
    add_definitions(-DRESOURCE_DIR=${CMAKE_SOURCE_DIR}/resources)
endif()

###################################################
# Set versions of C/C++                           #
# Note: CMAKE_CXX_STANDARD 14 should work as well #
###################################################
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

################################################
# Add OpenGL directive to included extenstions #
# and suppress depracation warnins             #
################################################
add_definitions(-DGL_GLEXT_PROTOTYPES)
add_definitions(-DGL_SILENCE_DEPRECATION)

set(MAIN_LIB_LIST "")
list(APPEND MAIN_LIB_LIST 
    ${CMAKE_DL_LIBS})
    

####################################
# Find libraries based on platform #
####################################
if(BUILD_MS_WINDOWS)
    # SDL
    unset(SDL_LIB CACHE)
    set(SDL_PATH ${CMAKE_SOURCE_DIR}/lib/win/SDL3/${WINDOWS_ARCH})
    find_library(SDL_LIB NAMES SDL3 PATHS ${SDL_PATH})
    list(APPEND MAIN_LIB_LIST ${SDL_LIB})
    
    # OpenGL
    add_definitions(-DGL_GLEXT_PROTOTYPES)
    add_definitions(-DGL_SILENCE_DEPRECATION)
    find_package(OpenGL REQUIRED)
    list(APPEND MAIN_LIB_LIST ${OPENGL_gl_LIBRARY})
    include_directories(${OpenGL_INCLUDE_DIR})
    include_directories(${OpenGL_INCLUDE_DIRS})
    link_directories(${OpenGL_LIBRARY_DIRS})
    add_definitions(${OpenGL_DEFINITIONS})
    if(NOT OPENGL_FOUND)
        message(ERROR " OPENGL not found!")
    endif(NOT OPENGL_FOUND)

    # GLEW
    set(GLEW_PATH ${CMAKE_SOURCE_DIR}/lib/win/glew/lib/${WINDOWS_ARCH})
    find_library(GLEW_LIB NAMES glew32 PATHS ${GLEW_PATH})
    list(APPEND MAIN_LIB_LIST ${GLEW_LIB})
    include_directories(${CMAKE_SOURCE_DIR}/lib/win/glew/include)

    # ASSIMP
    find_library(ASSIMP_LIB NAMES assimp-vc143-mt PATHS ${CMAKE_SOURCE_DIR}/lib/win/assimp)
    list(APPEND MAIN_LIB_LIST ${ASSIMP_LIB})

elseif(BUILD_MACOS)
    # SDL
    find_library(SDL_LIB NAMES SDL3 PATHS ${CMAKE_SOURCE_DIR}/lib/macos)
    list(APPEND MAIN_LIB_LIST ${SDL_LIB})
    
    # OpenGL
    add_definitions(-DGL_GLEXT_PROTOTYPES)
    add_definitions(-DGL_SILENCE_DEPRECATION)
    find_library(OPENGL_LIB NAMES OpenGL)
    include_directories(/usr/local/include)
    list(APPEND MAIN_LIB_LIST 
        "-framework OpenGL")

    # ASSIMP
    find_library(ASSIMP_LIB NAMES assimp PATHS ${CMAKE_SOURCE_DIR}/lib/macos/assimp)
    list(APPEND MAIN_LIB_LIST ${ASSIMP_LIB}) 
    find_library(ZLIB_LIB NAMES zlibstatic PATHS ${CMAKE_SOURCE_DIR}/lib/macos/zlib/)
    list(APPEND MAIN_LIB_LIST ${ZLIB_LIB}) 

elseif(BUILD_LINUX)
    # SDL
    find_library(SDL_LIB NAMES SDL3 PATHS ${CMAKE_SOURCE_DIR}/lib/linux/SDL3)
    list(APPEND MAIN_LIB_LIST ${SDL_LIB}) 
    
    # PThread
    find_package(Threads REQUIRED)
    find_library(PTHREAD_LIBRARY NAMES pthread)
    list(APPEND MAIN_LIB_LIST Threads::Threads)
    
    # OpenGL
    add_definitions(-DGL_GLEXT_PROTOTYPES)
    add_definitions(-DGL_SILENCE_DEPRECATION)
    find_package(OpenGL REQUIRED)
    include_directories(${OpenGL_INCLUDE_DIR})
    link_directories(${OpenGL_LIBRARY_DIRS})
    add_definitions(${OpenGL_DEFINITIONS})
    list(APPEND MAIN_LIB_LIST 
        ${OPENGL_opengl_LIBRARY})
    
    # ASSIMP
    find_library(ASSIMP_LIB NAMES assimp PATHS ${CMAKE_SOURCE_DIR}/lib/linux/assimp)
    list(APPEND MAIN_LIB_LIST ${ASSIMP_LIB}) 

endif()

############################################################
# Add each project to this list                            #
# Must be in the top-level directory [T] and the main file #
# must be named main.cpp.                                  #
#                                                          #
# For example, targets for Module2 and Module3 must be:    #
#	Module2/main.cpp                                       #
#	Module3/main.cpp                                       #
#                                                          #
#   Then target list will be                               #
#   set(TARGET_LIST "Module2")                             #
#   list(APPEND TARGET_LIST "Module3")                     #
############################################################
set(TARGET_LIST "SampleProject")
list(APPEND TARGET_LIST "PolygonMesh")
list(APPEND TARGET_LIST "RayTracer")
list(APPEND TARGET_LIST "GeometryTest")


#############################################
# Add paths to be searched for header files #
#############################################
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/include)

#######################
# Add sub directories #
#######################
set(SUB_LIB_LIST "")
add_subdirectory(scene)
add_subdirectory(geometry)
add_subdirectory(shader_support)
add_subdirectory(common)
add_subdirectory(filesystem_support)


######################################################
# Add files that need to be compiled for all targets #
######################################################
if(BUILD_MS_WINDOWS)
    file(GLOB GLEW_SOURCE_FILES 
        ${CMAKE_SOURCE_DIR}/lib/win/glew/src/glew.c
    )
endif()


###########################################################
# Iterate over each target and perform various operations #
###########################################################
foreach( target_i ${TARGET_LIST})
    ######################################################################
    # Add a directory for each target and set it as the binary directory #
    ######################################################################
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/${target_i}/)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${target_i})

    file(GLOB TARGET_SRC_FILES 
        ${CMAKE_SOURCE_DIR}/${target_i}/*.c
        ${CMAKE_SOURCE_DIR}/${target_i}/*.cpp
    )

    file(GLOB TARGET_HEADER_FILES 
        ${CMAKE_SOURCE_DIR}/${target_i}/*.h
        ${CMAKE_SOURCE_DIR}/${target_i}/*.hpp
    )

    file(GLOB SHADER_FILES
        ${CMAKE_SOURCE_DIR}/${target_i}/*.vert
        ${CMAKE_SOURCE_DIR}/${target_i}/*.frag
    )

    ########################
    # Add executable build #
    ########################
    add_executable(${target_i}
        ${CMAKE_SOURCE_DIR}/${target_i}/main.cpp 
        ${TARGET_SRC_FILES} 
        ${TARGET_HEADER_FILES} 
        ${SHADER_FILES}
        ${GLEW_SOURCE_FILES}
    )

    ##################################################################
    # Files that need to be copied to the build the /build directory #
    # Include all Shader files needed by the executable              #
    ##################################################################
    file(GLOB FILES_TO_COPY_TO_BUILD_DIR
        ${CMAKE_SOURCE_DIR}/${target_i}/*.vert
        ${CMAKE_SOURCE_DIR}/${target_i}/*.frag
    )
endforeach( target_i )

####################################
# Link libraries based on platform #
####################################
if(BUILD_MS_WINDOWS)
    foreach( target_i ${TARGET_LIST})
        source_group("filesystem_support" FILES ${FS_HEADER_FILES})
        source_group("geometry" FILES ${GEO_HEADER_FILES})
        source_group("scene" FILES ${SCENE_HEADER_FILES})
        source_group("shader_support" FILES ${SHAD_HEADER_FILES})
        source_group(${target_i} FILES ${TARGET_HEADER_FILES})

        file(GLOB SHADER_FILES
            ${CMAKE_SOURCE_DIR}/${target_i}/*.vert
            ${CMAKE_SOURCE_DIR}/${target_i}/*.frag
        )

        source_group("shaders" FILES ${SHADER_FILES})
        
        set_target_properties(${target_i} PROPERTIES COMPILE_FLAGS "-Wall")
        target_link_libraries(
            ${target_i} PRIVATE
            ${SUB_LIB_LIST}
            ${CMAKE_DL_LIBS}
            ${MAIN_LIB_LIST}
        )
        add_custom_command(TARGET  ${target_i} 
            POST_BUILD 
            COMMAND Copy /Y \"$(ProjectDir)..\\lib\\win\\SDL3\\${WINDOWS_ARCH}\\SDL3.dll\" \"$(TargetDir)\"
            COMMAND Copy /Y \"$(ProjectDir)..\\lib\\win\\glew\\bin\\${WINDOWS_ARCH}\\glew32.dll\" \"$(TargetDir)\"
            COMMAND Copy /Y \"$(ProjectDir)..\\lib\\win\\assimp\\assimp-vc143-mt.dll\" \"$(TargetDir)\"
            COMMENT "Moves dll files to output directory")
    endforeach( target_i )
elseif(BUILD_MACOS)
    foreach( target_i ${TARGET_LIST})
        set_target_properties(${target_i} PROPERTIES COMPILE_FLAGS "-Wno-deprecated -Wall")
        target_link_libraries(
            ${target_i} PRIVATE
            ${SUB_LIB_LIST}
            ${MAIN_LIB_LIST}
            ${PTHREAD_LIBRARY}
        )
    endforeach( target_i )
else()
    foreach( target_i ${TARGET_LIST})
        set_target_properties(${target_i} PROPERTIES COMPILE_FLAGS "-Wno-deprecated -Wall")
        target_link_libraries(
            ${target_i} PRIVATE
            ${SUB_LIB_LIST}
            ${MAIN_LIB_LIST}
            ${CMAKE_DL_LIBS} 
        )
    endforeach( target_i )
endif()

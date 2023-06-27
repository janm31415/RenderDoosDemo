if (WIN32)
  set(OPENGL_LIBRARIES opengl32.lib CACHE FILEPATH "opengl lib file")
  set(GLX_LIBRARIES "")
endif (WIN32)

if (UNIX)
  if (APPLE)
    set(OPENGL_LIBRARIES "" CACHE FILEPATH "opengl lib file")
    set(GLX_LIBRARIES "")
    
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework Cocoa -framework QuartzCore -framework Metal")
    set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -framework Cocoa -framework QuartzCore -framework Metal")
    
  else()
    set(OPENGL_LIBRARIES "/usr/lib/x86_64-linux-gnu/libOpenGL.so.0.0.0" CACHE FILEPATH "opengl lib file")
    set(GLX_LIBRARIES "/usr/lib/x86_64-linux-gnu/libGLX.so" CACHE FILEPATH "glx lib files")
  endif(APPLE)
endif (UNIX)

set(HDRS
trackball.h
    )
	
set(SRCS
main.cpp
trackball.c
)

if (APPLE)
set(GLEW
)
else (APPLE)
set(GLEW
glew.cpp
)
endif (APPLE)

set(SHADERS
)

if (APPLE)
list(APPEND HDRS ../SDL-metal/SDL_metal.h)
list(APPEND SRCS ../SDL-metal/SDL_metal.mm)
list(APPEND SHADERS ../RenderDoos/RenderDoos/shaders.metal)
endif (APPLE)

if (WIN32)
set(CMAKE_C_FLAGS_DEBUG "/W4 /MP /GF /RTCu /Od /MDd /Zi")
set(CMAKE_CXX_FLAGS_DEBUG "/W4 /MP /GF /RTCu /Od /MDd /Zi")
set(CMAKE_C_FLAGS_RELEASE "/W4 /MP /GF /O2 /Ob2 /Oi /Ot /MD /Zi")
set(CMAKE_CXX_FLAGS_RELEASE "/W4 /MP /GF /O2 /Ob2 /Oi /Ot /MD /Zi")
endif(WIN32)

if (UNIX)
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -msse4.1 -pthread")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -msse4.1 -pthread -std=c++11")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -msse4.1 -pthread")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -msse4.1 -pthread -std=c++11")
endif (UNIX)

# general build definitions
add_definitions(-DNOMINMAX)
add_definitions(-D_UNICODE)
add_definitions(-DUNICODE)
add_definitions(-D_SCL_SECURE_NO_WARNINGS)
add_definitions(-D_CRT_SECURE_NO_WARNINGS)
add_definitions(-DMEMORY_LEAK_TRACKING)

if (WIN32)
add_executable(RenderCubeSDL2 WIN32 ${HDRS} ${SRCS} ${GLEW} ${SHADERS})
else()
add_executable(RenderCubeSDL2 ${HDRS} ${SRCS} ${GLEW} ${SHADERS})
endif(WIN32)
source_group("Header Files" FILES ${hdrs})
source_group("Source Files" FILES ${srcs})
source_group("ThirdParty/Glew" FILES ${GLEW})
if (APPLE)
set_source_files_properties(${SHADERS} PROPERTIES LANGUAGE METAL)
endif (APPLE)

 target_include_directories(RenderCubeSDL2
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../RenderDoos
   	${CMAKE_CURRENT_SOURCE_DIR}/../RenderDoos/glew/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../RenderDoos/glew/include/glew
    ${CMAKE_CURRENT_SOURCE_DIR}/../SDL2/include/    
    )	
	
if (APPLE)
target_link_libraries(RenderCubeSDL2
    RenderDoos
    SDL2
    SDL2main
    )
else(APPLE)
target_link_libraries(RenderCubeSDL2    	   
    ${OPENGL_LIBRARIES}
    ${GLX_LIBRARIES}
    RenderDoos
    SDL2
    SDL2main
    )	
endif(APPLE)

cmake_minimum_required(VERSION 2.8)

project(DTSTest)

INCLUDE(CheckCCompilerFlag)

set(BUILD_GLES2 "false" CACHE FILEPATH "Build code for OpenGLES2")


ADD_DEFINITIONS(-DPCRE_STATIC=1)
ADD_DEFINITIONS(-DHAVE_CONFIG_H=1)
ADD_DEFINITIONS(-DDOM_INCLUDE_TINYXML=1)
ADD_DEFINITIONS(-DLINUX=1)
ADD_DEFINITIONS(-DUNICODE=1)
ADD_DEFINITIONS(-DGLEW_NO_GLU=1)

include_directories(../../libdts)
include_directories(../../example)
include_directories(../../libdts/src/)
include_directories(../../libdts/collada/include)
include_directories(../../libdts/tinyxml)
include_directories(../../libdts/pcre)
include_directories(../../libdts/collada/include/1.4)
include_directories(/usr/local/include)
link_directories(/opt/vc/lib)

set(DTSEXAMPLE_SOURCES
	../../example/main.cpp
	../../example/GLMaterial.cpp
	../../example/generic_support.cpp
	../../example/GLSimpleShader.cpp
	../../example/GLTSMeshRenderer.cpp
	../../example/soil/image_DXT.c
	../../example/soil/image_helper.c
	../../example/soil/SOIL.c
	../../example/soil/stb_image_aug.c
)


IF(BUILD_GLES2)
	message("Building for GLES2")
	ADD_DEFINITIONS(-DHAVE_OPENGLES2=1)
	set(EXTRA_LFLAGS "-lGLESv2 -lEGL -lm -lbcm_host")
ELSE()
	message("Building for OpenGL")
	SET(DTSEXAMPLE_SOURCES
		${DTSEXAMPLE_SOURCES}
		../../example/GL/glew.c
	)
ENDIF(BUILD_GLES2)

EXEC_PROGRAM(sdl2-config ARGS "--cflags" OUTPUT_VARIABLE SDL_CFLAGS)
EXEC_PROGRAM(sdl2-config ARGS "--libs" OUTPUT_VARIABLE SDL_LIBS)

#set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} ${SDL_LIBS} ${EXTRA_LFLAGS}")

add_executable(DTSTest ${DTSEXAMPLE_SOURCES})
set_target_properties(DTSTest PROPERTIES COMPILE_FLAGS ${SDL_CFLAGS})

target_link_libraries(DTSTest DTShape collada_dom tinyxml convexDecomp pcre ${SDL_LIBS} ${EXTRA_LFLAGS})

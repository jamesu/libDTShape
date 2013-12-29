cmake_minimum_required(VERSION 2.6)

project(zlib)

ADD_DEFINITIONS(-DHAVE_CONFIG_H=1)

set(ZLIB_SOURCES
	../../libdts/zlib/adler32.c
	../../libdts/zlib/crc32.c
	../../libdts/zlib/deflate.c
	../../libdts/zlib/gzio.c
	../../libdts/zlib/inffast.c
	../../libdts/zlib/zutil.c
	../../libdts/zlib/compress.c
	../../libdts/zlib/deflate.c
	../../libdts/zlib/infback.c
	../../libdts/zlib/inffast.h
	../../libdts/zlib/inflate.c
	../../libdts/zlib/inftrees.c
	../../libdts/zlib/trees.c
	../../libdts/zlib/uncompr.c
)

add_library(zlib STATIC ${ZLIB_SOURCES})

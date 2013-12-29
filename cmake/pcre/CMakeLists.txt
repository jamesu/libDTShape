cmake_minimum_required(VERSION 2.6)

project(pcre)

ADD_DEFINITIONS(-DPCRE_STATIC=1)
ADD_DEFINITIONS(-DHAVE_CONFIG_H=1)

include_directories(../../libdts/pcre)

set(PCRE_SOURCES
	../../libdts/pcre/pcre_chartables.c
	../../libdts/pcre/pcre_dfa_exec.c
	../../libdts/pcre/pcre_get.c
	../../libdts/pcre/pcre_maketables.c
	../../libdts/pcre/pcre_refcount.c
	../../libdts/pcre/pcre_try_flipped.c
	../../libdts/pcre/pcre_version.c
	../../libdts/pcre/pcre_compile.c
	../../libdts/../../libdts/pcre/pcre_exec.c
	../../libdts/pcre/pcre_globals.c
	../../libdts/pcre/pcre_newline.c
	../../libdts/pcre/pcre_study.c
	../../libdts/pcre/pcre_ucp_searchfuncs.c
	../../libdts/pcre/pcre_xclass.c
	../../libdts/pcre/pcre_config.c
	../../libdts/pcre/pcre_fullinfo.c
	../../libdts/pcre/pcre_info.c
	../../libdts/pcre/pcre_ord2utf8.c
	../../libdts/pcre/pcre_tables.c
	../../libdts/pcre/pcre_valid_utf8.c
	../../libdts/pcre/pcrecpp.cc
	../../libdts/pcre/pcre_scanner.cc
	../../libdts/pcre/pcre_stringpiece.cc
)

add_library(pcre STATIC ${PCRE_SOURCES})

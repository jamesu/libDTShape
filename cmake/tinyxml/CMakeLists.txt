cmake_minimum_required(VERSION 2.6)

project(tinyxml)

set(TINYXML_SOURCES
	../../libdts/tinyxml/tinystr.cpp
	../../libdts/tinyxml/tinyxml.cpp
	../../libdts/tinyxml/tinyxmlerror.cpp
	../../libdts/tinyxml/tinyxmlparser.cpp
)

add_library(tinyxml STATIC ${TINYXML_SOURCES})

cmake_minimum_required(VERSION 2.8)

project(convexDecomp)

ADD_DEFINITIONS(-DLINUX=1)

set(CONVEXDECOMP_SOURCES
	../../libdts/convexDecomp/NvConcavityVolume.cpp
	../../libdts/convexDecomp/NvRemoveTjunctions.cpp
	../../libdts/convexDecomp/NvSplitMesh.cpp
	../../libdts/convexDecomp/NvFloatMath.cpp
	../../libdts/convexDecomp/NvRayCast.cpp
	../../libdts/convexDecomp/NvThreadConfig.cpp
	../../libdts/convexDecomp/NvConvexDecomposition.cpp
	../../libdts/convexDecomp/NvMeshIslandGeneration.cpp
	../../libdts/convexDecomp/NvStanHull.cpp
	../../libdts/convexDecomp/wavefront.cpp
)

add_library(convexDecomp STATIC ${CONVEXDECOMP_SOURCES})

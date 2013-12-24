//
//  libDTShapeConfig.h
//  libdts
//
//  Created by James Urquhart on 11/12/2012.
//  Copyright (c) 2012 James Urquhart. All rights reserved.
//

#ifndef _LIBDTSHAPE_CONFIG_H
#define _LIBDTSHAPE_CONFIG_H

#define LIBDTSHAPE_MULTITHREAD
#define LIBDTSHAPE_FRAME_SIZE     16 << 20


#ifdef LIBDTSHAPE_DEBUG

#define LIBDTSHAPE_GATHER_METRICS 0
#define LIBDTSHAPE_ENABLE_PROFILE_PATH
#ifndef LIBDTSHAPE_DEBUG_GUARD
#define LIBDTSHAPE_DEBUG_GUARD
#endif

// Enables the C++ assert macros AssertFatal, AssertWarn, etc.
#define LIBDTSHAPE_ENABLE_ASSERTS

#endif

// Define if you implement custom IO, then implement TwistFork::Core::File
//#define LIBDTSHAPE_CUSTOM_IO

//

// Allow DTS files to be generated from collada files
#define LIBDTSHAPE_INCLUDE_COLLADA

#endif

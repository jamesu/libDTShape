//
//  twistforkConfig.h
//  libdts
//
//  Created by James Urquhart on 11/12/2012.
//  Copyright (c) 2012 James Urquhart. All rights reserved.
//

#ifndef _TWISTFORK_CONFIG_H
#define _TWISTFORK_CONFIG_H

#define TWISTFORK_MULTITHREAD
#define TWISTFORK_FRAME_SIZE     16 << 20


#ifdef TWISTFORK_DEBUG

#define TWISTFORK_ENABLE_PROFILE_PATH
#ifndef TWISTFORK_DEBUG_GUARD
#define TWISTFORK_DEBUG_GUARD
#endif


// Enables the C++ assert macros AssertFatal, AssertWarn, etc.
#define TWISTFORK_ENABLE_ASSERTS

#endif

#ifndef TWISTFORK_DISABLE_MEMORY_MANAGER
#define TWISTFORK_DISABLE_MEMORY_MANAGER
#endif

// Define if you implement custom IO, then implement TwistFork::Core::File
//#define TWISTFORK_CUSTOM_IO

//

// Allow DTS files to be generated from collada files
#define TWISTFORK_INCLUDE_COLLADA


// Define which string type we want to use
#include <string>
typedef std::string String;
extern String EmptyString;


#endif

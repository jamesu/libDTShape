//
//  libdtshape.h
//  libdts
//
//  Created by James Urquhart on 22/12/2013.
//  Copyright (c) 2013 James Urquhart. All rights reserved.
//

#ifndef __libdts__libdtshape__
#define __libdts__libdtshape__

#define DTS_CPU_FLAG(prop) (prop & 0xFFFF) << 16

#include "platform/platform.h"

BEGIN_NS(DTSLib)

void init(U32 opts);
void shutdown();

void initMeshIntrinsics();

END_NS



#endif /* defined(__libdts__libdtshape__) */

//
//  libdtshape.cpp
//  libdts
//
//  Created by James Urquhart on 22/12/2013.
//  Copyright (c) 2013 James Urquhart. All rights reserved.
//

#include "libdtshape.h"
#include "ts/tsRender.h"


BEGIN_NS(DTShapeInit)

using namespace DTShape;

void init(U32 opts)
{
   Processor::init();
   
   FrameAllocator::init(1024*1024); // 1mb
   
   Math::init(((opts >> 16) & 0xFFFF));
   
   initMeshIntrinsics();
   
   TSVertexColor::mDeviceSwizzle = &Swizzles::rgba;
}

void shutdown()
{
   
}

END_NS

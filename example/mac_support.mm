//
//  support.mm
//  DTSTest
//
//  Created by James Urquhart on 30/11/2013.
//  Copyright (c) 2013 James Urquhart. All rights reserved.
//

#include <Cocoa/Cocoa.h>

static char sPathBuffer[4096];
const char* GetAssetPath(const char *file)
{
   NSString *bundlePath = [[NSBundle mainBundle] bundlePath];
   snprintf(sPathBuffer, sizeof(sPathBuffer), "%s/Contents/Resources/%s", [bundlePath UTF8String], file);
   return sPathBuffer;
}

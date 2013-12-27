#include "platform/platform.h"
#include "core/strings/stringFunctions.h"

static char sPathBuffer[4096];
const char* GetAssetPath(const char *file)
{
   DTShape::dSprintf(sPathBuffer, sizeof(sPathBuffer), "%s/%s", DTShape::Platform::getRootDir(), file);
   return sPathBuffer;
}


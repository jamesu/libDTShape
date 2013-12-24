//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "ts/tsMaterialManager.h"

#include "core/util/safeDelete.h"
#include "core/log.h"

//-----------------------------------------------------------------------------

BEGIN_NS(DTShape)

//-----------------------------------------------------------------------------

void TSMaterialManager::mapMaterial(const String & textureName, const String & materialName)
{
   if (getMapEntry(textureName).isNotEmpty())
   {
      if (!textureName.equal("unmapped_mat", String::NoCase))
         Log::warnf(LogEntry::General, "Warning, overwriting material for: %s", textureName.c_str());
   }
   
   mMaterialMap[String::ToLower(textureName)] = materialName;
}

String TSMaterialManager::getMapEntry(const String & textureName) const
{
   MaterialMap::ConstIterator iter = mMaterialMap.find(String::ToLower(textureName));
   if ( iter == mMaterialMap.end() )
      return String();
   return iter->value;
}

//-----------------------------------------------------------------------------

END_NS

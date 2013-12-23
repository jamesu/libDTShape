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
#ifndef _TSMATERIALMANAGER_H_
#define _TSMATERIALMANAGER_H_

#ifndef _TSMATERIAL_H_
#include "ts/tsMaterial.h"
#endif

#ifndef _TSINGLETON_H_
#include "core/util/tSingleton.h"
#endif

#ifndef _TDICTIONARY_H_
#include "core/util/tDictionary.h"
#endif

class TSMaterialManager : public EngineObject
{
public:
   typedef Map<String, String> MaterialMap;
   TSMaterialManager::MaterialMap mMaterialMap;
   
   TSMaterialManager(){;}
   virtual ~TSMaterialManager() {;}

   // Allocate a new material (used by the collada import)
   virtual TSMaterial * allocateAndRegister(const String &objectName, const String &mapToName = String()) = 0;
   
   // Grab an existing material
   virtual TSMaterial * getMaterialDefinitionByName(const String &matName) = 0;

   // map textures to materials
   virtual void mapMaterial(const String & textureName, const String & materialName);
   virtual String getMapEntry(const String & textureName) const;

   // Return instance of named material caller is responsible for memory
   virtual TSMaterialInstance * createMatInstance( const String &matName, const GFXVertexFormat *vertexFormat = NULL ) = 0;
   virtual TSMaterialInstance * createFallbackMatInstance( const GFXVertexFormat *vertexFormat = NULL ) = 0;
   
   static TSMaterialManager *instance();
};

#define MATMGR TSMaterialManager::instance()

#endif // _MATERIAL_MGR_H_

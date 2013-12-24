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
#ifndef _TSMATERIAL_H_
#define _TSMATERIAL_H_

#ifndef _REFBASE_H_
#include "core/util/refBase.h"
#endif

//-----------------------------------------------------------------------------

BEGIN_NS(DTShape)

//-----------------------------------------------------------------------------

class GFXVertexFormat;
class ColladaAppMaterial;
class TSMaterialInstance;

/// Represents a material in a DTS shape
/// (The actual implementation logic is up to the client app, libDTS does no rendering
/// itself)
class TSMaterial : public StrongRefBase
{
public:
   TSMaterial() {;}
   virtual ~TSMaterial() {;}
   
   // Create an instance of this material
   virtual TSMaterialInstance *createMatInstance(const GFXVertexFormat *fmt=NULL) = 0;
   
   // Assign properties from collada material
   virtual bool initFromColladaMaterial(const ColladaAppMaterial *mat) = 0;
   
   // Get base material definition
   virtual TSMaterial *getMaterial() = 0;
   
   virtual bool isTranslucent() = 0;
   
   virtual const char* getName() = 0;
};

// Instance of TSMaterial. This is mainly for the app to track instances created from
// TSMaterial::createMatInstance.
class TSMaterialInstance : public StrongRefBase
{
public:
   
   TSMaterialInstance(){;}
   ~TSMaterialInstance(){;}
   
   virtual bool init(const GFXVertexFormat *fmt=NULL) = 0;
   virtual TSMaterial *getMaterial() = 0;
   virtual bool isTranslucent() = 0;
   virtual bool isValid() = 0;
   
   virtual int getStateHint() = 0;
   
   virtual const char* getName() = 0;
};

//-----------------------------------------------------------------------------

END_NS

#endif /// _BASEMATINSTANCE_H_







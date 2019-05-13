#ifndef _GLMATERIAL_H_
#define _GLMATERIAL_H_

/*
Copyright (C) 2013 James S Urquhart

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/

#include "ts/tsRender.h"
#include "ts/tsRenderState.h"
#include "ts/tsMaterial.h"
#include "ts/tsMaterialManager.h"
#include "core/util/tVector.h"

#include <unordered_map>
#include <Metal/Metal.h>

using namespace DTShape;

class MetalTSMaterial;
class MetalTSSceneRenderState;
class MetalSimpleShader;

struct MetalPipelineState
{
   id<MTLRenderPipelineState> pipeline;
   id<MTLDepthStencilState> depth;
   uint32_t variantIdx;
   
   ~MetalPipelineState()
   {
      pipeline = nil;
      depth = nil;
   }
};

// Basic OpenGL Material Instance
class MetalTSMaterialInstance : public TSMaterialInstance
{
public:
   MetalTSMaterial *mMaterial;
   MetalPipelineState* mPipeline;
   id<MTLTexture> mTexture;
   id<MTLSamplerState> mSampler;
   
   const GFXVertexFormat *mVertexFormat;
   
   MetalTSMaterialInstance(MetalTSMaterial *mat);
   ~MetalTSMaterialInstance();
   
   virtual bool init(TSMaterialManager* mgr, const GFXVertexFormat *fmt=NULL);
   
   virtual TSMaterial *getMaterial();
   
   virtual bool isTranslucent();
   
   virtual bool isValid();
   
   virtual int getStateHint();
   
   virtual const char* getName();
   
   void activate(const TSSceneRenderState *sceneState, TSRenderInst *renderInst);
};

// Basic OpenGL Material
class MetalTSMaterial : public TSMaterial
{
public:
   String mMapTo;
   String mName;
   
   MetalTSMaterial();
   ~MetalTSMaterial();
   
   // Create an instance of this material
   virtual TSMaterialInstance *createMatInstance(const GFXVertexFormat *fmt=NULL);
   
   // Assign properties from collada material
   virtual bool initFromColladaMaterial(const ColladaAppMaterial *mat);
   
   // Get base material definition
   virtual TSMaterial *getMaterial();
   
   virtual bool isTranslucent();
   
   virtual const char* getName();
};

// Basic OpenGL Material Manager
class TSPipelineCollection;
class MetalTSMaterialManager : public TSMaterialManager
{
public:
   
   MetalTSMaterial *mDummyMaterial;
   Vector<MetalTSMaterial*> mMaterials;
   
   id<MTLFunction> mBasicVertexFunction;
   id<MTLFunction> mSkinnedVertexFunction;
   id<MTLFunction> mBasicFragmentFunction;
   
   std::unordered_map<const char*, TSPipelineCollection*> mPipelines;
   
   id<MTLDevice> mDevice;
   
   
   
   MetalTSMaterialManager();
   ~MetalTSMaterialManager();
   
   void init(id<MTLDevice> &device, MTLPixelFormat backFormat);
   void cleanup();
   
   TSMaterial *allocateAndRegister(const String &objectName, const String &mapToName = String());

   // Grab an existing material
   virtual TSMaterial * getMaterialDefinitionByName(const String &matName);
   
   // Return instance of named material caller is responsible for memory
   virtual TSMaterialInstance * createMatInstance( const String &matName, const GFXVertexFormat *vertexFormat = NULL );
   virtual TSMaterialInstance * createFallbackMatInstance( const GFXVertexFormat *vertexFormat = NULL );
};


#endif // _GLMATERIAL_H_

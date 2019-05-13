#ifndef _GLTSMESHRENDERER_H_
#define _GLTSMESHRENDERER_H_

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

#include "ts/tsRenderState.h"
#include "ts/tsMesh.h"
#include <Metal/Metal.h>

#include "mtl_shared.h"

using namespace DTShape;

// Basic OpenGL Buffer Information
class MetalTSMeshInstanceRenderData : public TSMeshInstanceRenderData
{
public:
   MetalTSMeshInstanceRenderData() : mVB(), mNumVerts(0), mVBOffset(0) { memset(&mComputeParams, '\0', sizeof(mComputeParams)); }
   
   ~MetalTSMeshInstanceRenderData()
   {
   }
   
   id<MTLBuffer> mVB;
   U32 mVBOffset;
   U32 mNumVerts;
   SkinParams mComputeParams;
};

// Generic interface which stores vertex buffers and such for TSMeshes
class MetalTSMeshRenderer : public TSMeshRenderer
{
public:
   MetalTSMeshRenderer();
   virtual ~MetalTSMeshRenderer();
   
   static id<MTLDevice> smDevice;
   static id<MTLRenderCommandEncoder> smCurrentRenderEncoder;
   
   id<MTLBuffer> mPB;
   id<MTLBuffer> mVB;
   int mNumIndices;
   int mNumVerts;
   
   virtual void prepare(TSMesh *mesh, TSMeshInstanceRenderData *meshRenderData);
   
   virtual U8* mapVerts(TSMesh *mesh, TSMeshInstanceRenderData *meshRenderData);
   
   virtual void unmapVerts(TSMesh *mesh, TSMeshInstanceRenderData *meshRenderData);
   
   void bindArrays(const GFXVertexFormat *fmt);
   
   void unbindArrays(const GFXVertexFormat *fmt);
   
   virtual void onAddRenderInst(TSMesh *mesh, TSRenderInst *inst, TSRenderState *renderState);
   
   /// Renders whatever needs to be drawn
   virtual void doRenderInst(TSMesh *mesh, TSRenderInst *inst, TSRenderState *renderState);
   
   /// Renders whatever needs to be drawn
   virtual bool isDirty(TSMesh *mesh, TSMeshInstanceRenderData *meshRenderData);
   
   /// Cleans up Mesh renderer
   virtual void clear();
};

#endif

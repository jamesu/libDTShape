//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
// Portions Copyright (C) 2013 James S Urquhart
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

#include "MetalTSMeshRenderer.h"
#include "MetalMaterial.h"
#include "core/log.h"
#include "ts/tsShape.h"
#include "mtl_shared.h"
#include "libdtshape.h"

#include "main.h"
#include "SDL.h"
#include "ts/tsMaterialManager.h"
#include "MetalRenderer.h"

id<MTLDevice> MetalTSMeshRenderer::smDevice;
id<MTLRenderCommandEncoder> MetalTSMeshRenderer::smCurrentRenderEncoder;

static MTLPrimitiveType drawTypes[] = { MTLPrimitiveTypeTriangle, MTLPrimitiveTypeTriangleStrip, MTLPrimitiveTypeTriangle };
#define getDrawType(a) (drawTypes[a])

MetalTSMeshRenderer::MetalTSMeshRenderer() : mVB(nil), mPB(nil), mNumIndices(0), mNumVerts(0)
{
}

MetalTSMeshRenderer::~MetalTSMeshRenderer()
{
	clear();
}

void MetalTSMeshRenderer::prepare(TSMesh *mesh, TSMeshInstanceRenderData *meshRenderData)
{
  bool reloadPB = false;
  
  // Generate primitive buffer if neccesary
  if (mPB == 0 || mesh->indices.size() != mNumIndices)
  {
     reloadPB = true;
  }
  
  mNumIndices = mesh->indices.size();
  
  U32 size=0;
  MetalTSMeshInstanceRenderData *renderData = (MetalTSMeshInstanceRenderData*)meshRenderData;
  
   // Load buffer to mesh data if we're using HW skinning
   if (TSShape::smUseHardwareSkinning)
   {
      if (mVB == 0 || mNumVerts != mesh->mNumVerts)
      {
         mNumVerts = mesh->mNumVerts;
         size = mesh->mVertexFormat->getSizeInBytes() * mNumVerts;
         
         // Load vertices
         if (!mVB)
         {
            mVB = [smDevice newBufferWithLength:size options:MTLResourceCPUCacheModeWriteCombined];
            void *vertexPtr = mVB.contents;
            memcpy(vertexPtr, mesh->mVertexData.address(), size);
         }
         
         
         //Log::printf("Update buffer");
      }
   }
   else
   {
      mVB = nil;
      mNumVerts = 0;
   }
   
  // Update vertex buffer
  // (NOTE: if we're a Skin mesh and want to save a copy, we can also use mapVerts)
  if (renderData)
  {
     // First, ensure the buffer is valid
     if (TSShape::smUseHardwareSkinning && !TSShape::smUseComputeSkinning)
     {
        renderData->mVB = mVB;
        renderData->mVBOffset = 0;
        renderData->mNumVerts = mNumVerts;
     }
     else
     {
        // We need to refresh verts per-frame
        MetalRenderer* renderer = (MetalRenderer*)(AppState::getInstance()->mRenderer);
        renderData->mNumVerts = mesh->mNumVerts;
        size = mesh->mVertexFormat->getSizeInBytes() * renderData->mNumVerts;
        char *vertexPtr = renderer->allocFrameVBO(size, renderData->mVB, renderData->mVBOffset);
        
        if (vertexPtr)
        {
           if (!TSShape::smUseHardwareSkinning)
           {
              //dMemcpy( vertexPtr, mesh->mVertexData.address(), size );
           }
           else
           {
              //dMemset(vertexPtr, '\0', size);//dMemcpy( vertexPtr, mesh->mVertexData.address(), size );
           }
           dMemcpy( vertexPtr, mesh->mVertexData.address(), size );
           //[renderData->mVB didModifyRange:NSMakeRange(renderData->mVBOffset, size)];
        }
        
        if (renderData->mComputeParams.inStride == 0)
        {
           // Might as well determine compute skin params here
           SkinParams params;
           params.inStride = mesh->mVertexFormat->getSizeInBytes();
           params.inBoneOffset = 0;
           params.outStride = params.inStride;
           
           for (int i=0; i<mesh->mVertexFormat->getElementCount(); i++)
           {
              const GFXVertexElement &el = mesh->mVertexFormat->getElement(i);
              if (el.getSemantic() == GFXSemantic::BLENDINDICES) // found start of bones
                 break;
              
              params.inBoneOffset += el.getSizeInBytes();
           }
           
           renderData->mComputeParams = params;
        }
        
        
     }
  }
  
  // Load indices
  if (reloadPB)
  {
     size = sizeof(U16)*mNumIndices;
     mPB = [smDevice newBufferWithLength:size options:MTLResourceCPUCacheModeDefaultCache];
     
     // NOTE: Essentially all mesh indices in primitives are placed within 16bit boundaries
     // (see ColladaAppMesh::getPrimitives), so we can safely just convert them to U16s.
     U16 *indexPtr = (U16*)mPB.contents;
     dCopyArray( indexPtr, mesh->indices.address(), mesh->indices.size() );
  }
}

U8 *MetalTSMeshRenderer::mapVerts(TSMesh *mesh, TSMeshInstanceRenderData *meshRenderData)
{
  return NULL; // transformed vertices will be stored in mesh->mVertexData
}

void MetalTSMeshRenderer::unmapVerts(TSMesh *mesh, TSMeshInstanceRenderData *meshRenderData)
{
   //
}

void MetalTSMeshRenderer::bindArrays(const GFXVertexFormat *fmt)
{
   //
}

void MetalTSMeshRenderer::unbindArrays(const GFXVertexFormat *fmt)
{
  // reverse what we did in bindArrays
}

void MetalTSMeshRenderer::onAddRenderInst(TSMesh *mesh, TSRenderInst *inst, TSRenderState *renderState)
{
  // we don't need to do anything here since
}

/// Renders whatever needs to be drawn
void MetalTSMeshRenderer::doRenderInst(TSMesh *mesh, TSRenderInst *inst, TSRenderState *renderState)
{
   //Platform::outputDebugString("[TSM:%x:%x]Rendering with material %s", this, inst->renderData, inst->matInst ? inst->matInst->getName() : "NIL");
   ((MetalTSMaterialInstance*)inst->matInst)->activate(renderState->getSceneState(), inst);
   
  S32 matIdx = -1;
  
  // Bind VBOS
  MetalTSMeshInstanceRenderData *renderData = (MetalTSMeshInstanceRenderData*)inst->renderData;
  [smCurrentRenderEncoder setVertexBuffer:renderData->mVB offset:renderData->mVBOffset atIndex:TS_VERT_VBO];
   //[smCurrentRenderEncoder setVertexBuffer:mVB offset:renderData->mVBOffset atIndex:TS_VERT_VBO];
  
  // basically, go through primitives & draw
  for (int i=0; i<mesh->primitives.size(); i++)
  {
     TSDrawPrimitive prim = mesh->primitives[i];
     // Use the first index to determine which 16-bit address space we are operating in
     U32 startVtx = mesh->indices[prim.start] & 0xFFFF0000;
     U32 numVerts = getMin((U32)0x10000, renderData->mNumVerts - startVtx);
     
     matIdx = prim.matIndex & (TSDrawPrimitive::MaterialMask|TSDrawPrimitive::NoMaterial);
     
     if (matIdx & TSDrawPrimitive::NoMaterial)
        continue;
     
     [smCurrentRenderEncoder drawIndexedPrimitives:getDrawType(prim.matIndex >> 30) indexCount:prim.numElements indexType:MTLIndexTypeUInt16 indexBuffer:mPB indexBufferOffset:0];
  }
}

/// Renders whatever needs to be drawn
bool MetalTSMeshRenderer::isDirty(TSMesh *mesh, TSMeshInstanceRenderData *meshRenderData)
{
  // Basically: are the buffers the correct size?
  MetalTSMeshInstanceRenderData *renderData = (MetalTSMeshInstanceRenderData*)meshRenderData;
  
  const bool vertsChanged = renderData->mVB == 0 || renderData->mNumVerts != mesh->mNumVerts;
  const bool primsChanged = mPB == 0 || mNumIndices != mesh->indices.size();
  
  if (vertsChanged || primsChanged)
     return true;
  else
     return false;
}

/// Cleans up Mesh renderer
void MetalTSMeshRenderer::clear()
{
  //Platform::outputDebugString("[TSM:%x]Clearing", this);
  
   mPB = nil;
  mNumIndices = 0;
}

TSMeshRenderer *TSMeshRenderer::create()
{
   return new MetalTSMeshRenderer();
}

TSMeshInstanceRenderData *TSMeshInstanceRenderData::create()
{
   return new MetalTSMeshInstanceRenderData();
}


/*
 Copyright (C) 2019 James S Urquhart
 
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

#include "GLTSMeshRenderer.h"
#include "GLMaterial.h"
#include "core/log.h"
#include "ts/tsShape.h"
#include "mtl_shared.h"
#include "libdtshape.h"

#include "main.h"
#include "SDL.h"
#include "ts/tsMaterialManager.h"
#include "GLRenderer.h"

static GLenum drawTypes[] = { GL_TRIANGLES, GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN };
#define getDrawType(a) (drawTypes[a])

GLTSMeshRenderer::GLTSMeshRenderer() : mVB(0), mPB(0), mNumIndices(0), mNumVerts(0)
{
}

GLTSMeshRenderer::~GLTSMeshRenderer()
{
	clear();
}

void GLTSMeshRenderer::prepare(TSMesh *mesh, TSMeshInstanceRenderData *meshRenderData)
{
  bool reloadPB = false;
  
  // Generate primitive buffer if neccesary
  if (mPB == 0 || mesh->indices.size() != mNumIndices)
  {
     reloadPB = true;
  }
  
  mNumIndices = mesh->indices.size();
  
  U32 size=0;
  GLTSMeshInstanceRenderData *renderData = (GLTSMeshInstanceRenderData*)meshRenderData;
  
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
            glGenBuffers(1, &mVB);
            glBindBuffer(GL_ARRAY_BUFFER, mVB);
            glBufferData(GL_ARRAY_BUFFER, size, mesh->mVertexData.address(), GL_STATIC_DRAW);
         }
         
         //Log::printf("Update buffer");
      }
   }
   else
   {
      mVB = 0;
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
        GLRenderer* renderer = (GLRenderer*)(AppState::getInstance()->mRenderer);
        renderData->mNumVerts = mesh->mNumVerts;
        size = mesh->mVertexFormat->getSizeInBytes() * renderData->mNumVerts;
        char *vertexPtr = renderer->allocFrameVBO(size, renderData->mVB, renderData->mVBOffset);
        
        if (vertexPtr)
        {
           dMemcpy( vertexPtr, mesh->mVertexData.address(), size );
           glUnmapBuffer(GL_ARRAY_BUFFER);
        }
     }
  }
  
  // Load indices
  if (reloadPB)
  {
     size = sizeof(U16)*mNumIndices;
     if (mPB == 0)
     {
        glGenBuffers(1, &mPB);
     }
     
     glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mPB);
     glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->indices.size()*2, NULL, GL_STATIC_DRAW);
     U16* indexPtr = (U16*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
     
     // NOTE: Essentially all mesh indices in primitives are placed within 16bit boundaries
     // (see ColladaAppMesh::getPrimitives), so we can safely just convert them to U16s.
     dCopyArray( indexPtr, mesh->indices.address(), mesh->indices.size() );
     
     glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
  }
}

U8 *GLTSMeshRenderer::mapVerts(TSMesh *mesh, TSMeshInstanceRenderData *meshRenderData)
{
  return NULL; // transformed vertices will be stored in mesh->mVertexData
}

void GLTSMeshRenderer::unmapVerts(TSMesh *mesh, TSMeshInstanceRenderData *meshRenderData)
{
   //
}

void GLTSMeshRenderer::bindArrays(const GFXVertexFormat *fmt)
{
   //
}

void GLTSMeshRenderer::unbindArrays(const GFXVertexFormat *fmt)
{
  // reverse what we did in bindArrays
}

void GLTSMeshRenderer::onAddRenderInst(TSMesh *mesh, TSRenderInst *inst, TSRenderState *renderState)
{
  // we don't need to do anything here since
}

/// Renders whatever needs to be drawn
void GLTSMeshRenderer::doRenderInst(TSMesh *mesh, TSRenderInst *inst, TSRenderState *renderState)
{
   //Platform::outputDebugString("[TSM:%x:%x]Rendering with material %s", this, inst->renderData, inst->matInst ? inst->matInst->getName() : "NIL");
   ((GLTSMaterialInstance*)inst->matInst)->activate(renderState->getSceneState(), inst);
   
  S32 matIdx = -1;
  
  // Bind VBOS
  GLTSMeshInstanceRenderData *renderData = (GLTSMeshInstanceRenderData*)inst->renderData;
  glBindBuffer(GL_ARRAY_BUFFER, renderData->mVB);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mPB);
   
  ((GLTSMaterialInstance*)inst->matInst)->bindVertexBuffers();
  
  // basically, go through primitives & draw
  for (int i=0; i<mesh->primitives.size(); i++)
  {
     TSDrawPrimitive prim = mesh->primitives[i];
     // Use the first index to determine which 16-bit address space we are operating in
     U32 startVtx = mesh->indices[prim.start] & 0xFFFF0000;
     U32 numVerts = getMin((U32)0x10000, renderData->mNumVerts - startVtx);
     
     matIdx = prim.matIndex & (TSDrawPrimitive::MaterialMask | TSDrawPrimitive::NoMaterial);
     
     if (matIdx & TSDrawPrimitive::NoMaterial)
        continue;
     
     glDrawRangeElementsBaseVertex(getDrawType(prim.matIndex >> 30),
                                   0,
                                   numVerts,
                                   mesh->indices.size(),
                                   GL_UNSIGNED_SHORT,
                                   NULL,
                                   renderData->mVBOffset);
  }
}

/// Renders whatever needs to be drawn
bool GLTSMeshRenderer::isDirty(TSMesh *mesh, TSMeshInstanceRenderData *meshRenderData)
{
  // Basically: are the buffers the correct size?
  GLTSMeshInstanceRenderData *renderData = (GLTSMeshInstanceRenderData*)meshRenderData;
  
  const bool vertsChanged = renderData->mVB == 0 || renderData->mNumVerts != mesh->mNumVerts;
  const bool primsChanged = mPB == 0 || mNumIndices != mesh->indices.size();
  
  if (vertsChanged || primsChanged)
     return true;
  else
     return false;
}

/// Cleans up Mesh renderer
void GLTSMeshRenderer::clear()
{
  //Platform::outputDebugString("[TSM:%x]Clearing", this);
  
   if (mPB)
   {
      glDeleteBuffers(1, &mPB);
   }
   
  mNumIndices = 0;
}

#if !defined(USE_METAL) && !defined(USE_BOTH_RENDER)
void AppState::createRenderer()
{
   mRenderer = new GLRenderer();
   mMaterialManager = new GLTSMaterialManager();
}
TSMeshRenderer *TSMeshRenderer::create()
{
   return new GLTSMeshRenderer();
}

TSMeshInstanceRenderData *TSMeshInstanceRenderData::create()
{
   return new GLTSMeshInstanceRenderData();
}

#endif


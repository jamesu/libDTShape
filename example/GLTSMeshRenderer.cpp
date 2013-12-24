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

#include "GLTSMeshRenderer.h"
#include "GLMaterial.h"

static GLenum drawTypes[] = { GL_TRIANGLES, GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN };
#define getDrawType(a) (drawTypes[a])

extern GLStateTracker gGLStateTracker;

GLTSMeshRenderer::GLTSMeshRenderer() : mPB(0), mNumIndices(0)
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
     if (mPB == 0)
     {
        glGenBuffers(1, &mPB);
     }
     reloadPB = true;
  }
  
  mNumIndices = mesh->indices.size();
  
  U32 size;
  GLTSMeshInstanceRenderData *renderData = (GLTSMeshInstanceRenderData*)meshRenderData;
  
  // Update vertex buffer
  // (NOTE: if we're a Skin mesh and want to save a copy, we can also use mapVerts)
  if (renderData)
  {
     // First, ensure the buffer is valid
     if (renderData->mVB == 0 || renderData->mNumVerts != mesh->mNumVerts)
     {
        renderData->mNumVerts = mesh->mNumVerts;
        size = mesh->mVertexFormat->getSizeInBytes() * renderData->mNumVerts;
        
        // Load vertices
        if (renderData->mVB == 0)
           glGenBuffers(1, &renderData->mVB);
        glBindBuffer(GL_ARRAY_BUFFER, renderData->mVB);
        glBufferData(GL_ARRAY_BUFFER, size, NULL, mesh->mDynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
     }
     else
     {
        glBindBuffer(GL_ARRAY_BUFFER, renderData->mVB);
        size = mesh->mVertexFormat->getSizeInBytes() * renderData->mNumVerts;
     }
     
     // Now, load the transformed vertices straight from mVertexData
     char *vertexPtr = (char*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
     
     dMemcpy( vertexPtr, mesh->mVertexData.address(), size );
     
     glUnmapBuffer(GL_ARRAY_BUFFER);
     glBindBuffer(GL_ARRAY_BUFFER, 0);
  }
  
  // Load indices
  if (reloadPB)
  {
     size = sizeof(U16)*mNumIndices;
     glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mPB);
     glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, NULL, GL_STATIC_DRAW);
     
     // NOTE: Essentially all mesh indices in primitives are placed within 16bit boundaries
     // (see ColladaAppMesh::getPrimitives), so we can safely just convert them to U16s.
     U16 *indexPtr = (U16*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
     dCopyArray( indexPtr, mesh->indices.address(), mesh->indices.size() );
     
     glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
     glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  }
}

U8 *GLTSMeshRenderer::mapVerts(TSMesh *mesh, TSMeshInstanceRenderData *meshRenderData)
{
  return NULL; // transformed vertices will be stored in mesh->mVertexData
}

void GLTSMeshRenderer::unmapVerts(TSMesh *mesh, TSMeshInstanceRenderData *meshRenderData)
{
  
}

void GLTSMeshRenderer::bindArrays(const GFXVertexFormat *fmt)
{
  U32 stride = fmt->getSizeInBytes();
  
  bool hasPos = false;
  bool hasColor = false;
  bool hasTexcoord = false;
  bool hasNormal = false;
  U32 attribs = 0;
  
  // Loop thru the vertex format elements adding the array state...
  U32 texCoordIndex = 0;
  U8 *buffer = 0;
  GLenum __error = glGetError();
  
  for ( U32 i=0; i < fmt->getElementCount(); i++ )
  {
     const GFXVertexElement &element = fmt->getElement( i );
     
     if ( element.isSemantic( GFXSemantic::POSITION ) )
     {
        attribs |= kGLSimpleVertexAttribFlag_Position;
        if (!hasPos)
        {
           glVertexAttribPointer(kGLSimpleVertexAttrib_Position, element.getSizeInBytes() / 4, GL_FLOAT, GL_FALSE, stride, buffer );
           hasPos = true;
        }
        buffer += element.getSizeInBytes();
     }
     else if ( element.isSemantic( GFXSemantic::NORMAL ) )
     {
        attribs |= kGLSimpleVertexAttribFlag_Normal;
        if (!hasNormal)
        {
           glVertexAttribPointer(kGLSimpleVertexAttrib_Normal, element.getSizeInBytes() / 3, GL_FLOAT, GL_TRUE, stride, buffer );
           hasNormal = true;
        }
        buffer += element.getSizeInBytes();
     }
     else if ( element.isSemantic( GFXSemantic::COLOR ))
     {
        attribs |= kGLSimpleVertexAttribFlag_Color;
        if (!hasColor)
        {
           glVertexAttribPointer(kGLSimpleVertexAttrib_Color, element.getSizeInBytes(), GL_UNSIGNED_BYTE, GL_TRUE, stride, buffer );
           hasColor = true;
        }
        buffer += element.getSizeInBytes();
     }
     else // Everything else is a texture coordinate.
     {
        if (!hasTexcoord && (element.getSizeInBytes() / 4) == 2)
        {
           attribs |= kGLSimpleVertexAttribFlag_TexCoords;
           glVertexAttribPointer(kGLSimpleVertexAttrib_TexCoords, element.getSizeInBytes() / 4, GL_FLOAT, GL_FALSE, stride, buffer);
           hasTexcoord = true;
        }
        buffer += element.getSizeInBytes();
        ++texCoordIndex;
     }
  }
  __error = glGetError();
  
  gGLStateTracker.setVertexAttribs( attribs );
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
  
  ((GLTSMaterialInstance*)inst->matInst)->activate();
  
  S32 matIdx = -1;
  
  // Bind VBOS
  GLTSMeshInstanceRenderData *renderData = (GLTSMeshInstanceRenderData*)inst->renderData;
  glBindBuffer(GL_ARRAY_BUFFER, renderData->mVB);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mPB);
  
  bindArrays(mesh->mVertexFormat);
  GLenum __error = glGetError();
  
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
     
     U16 *ptr = NULL;
     glDrawElements(getDrawType(prim.matIndex >> 30), prim.numElements, GL_UNSIGNED_SHORT, ptr+prim.start);
  }
  
  unbindArrays(mesh->mVertexFormat);
  
  // unbind
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
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
     glDeleteBuffers(1, &mPB);
  
  mPB = 0;
  mNumIndices = 0;
}

TSMeshRenderer *TSMeshRenderer::create()
{
   return new GLTSMeshRenderer();
}

TSMeshInstanceRenderData *TSMeshInstanceRenderData::create()
{
   return new GLTSMeshInstanceRenderData();
}


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

#include "GLSimpleShader.h"
#include "ts/tsRenderState.h"
#include "ts/tsMesh.h"

// Generic interface which provides scene info to the rendering code
class GLTSSceneRenderState : public TSSceneRenderState
{
public:
   MatrixF mWorldMatrix;
   MatrixF mViewMatrix;
   MatrixF mProjectionMatrix;
   
   GLTSSceneRenderState()
   {
      mWorldMatrix = MatrixF(1);
      mViewMatrix = MatrixF(1);
      mProjectionMatrix = MatrixF(1);
   }
   
   ~GLTSSceneRenderState()
   {
   }
   
   virtual TSMaterialInstance *getOverrideMaterial( TSMaterialInstance *inst ) const
   {
      return inst;
   }
   
   virtual Point3F getCameraPosition() const
   {
      return mViewMatrix.getPosition();
   }
   
   virtual Point3F getDiffuseCameraPosition() const
   {
      return mViewMatrix.getPosition();
   }
   
   virtual RectF getViewport() const
   {
      return RectF(0,0,800,600);
   }
   
   virtual Point2F getWorldToScreenScale() const
   {
      return Point2F(1,1);
   }
   
   virtual const MatrixF *getWorldMatrix() const
   {
      return &mWorldMatrix;
   }
   
   virtual bool isShadowPass() const
   {
      return false;
   }
   
   // Shared matrix stuff
   virtual const MatrixF *getViewMatrix() const
   {
      return &mViewMatrix;
   }
   
   virtual const MatrixF *getProjectionMatrix() const
   {
      return &mProjectionMatrix;
   }
};

// Basic OpenGL Buffer Information
class GLTSMeshInstanceRenderData : public TSMeshInstanceRenderData
{
public:
   GLTSMeshInstanceRenderData() : mVB(0), mNumVerts(0) {;}
   
   ~GLTSMeshInstanceRenderData()
   {
      if (mVB != 0)
         glDeleteBuffers(1, &mVB);
      mVB = 0;
      mNumVerts = 0;
   }
   
   GLuint mVB;
   U32 mNumVerts;
};

// Generic interface which stores vertex buffers and such for TSMeshes
class GLTSMeshRenderer : public TSMeshRenderer
{
public:
   GLTSMeshRenderer();
   virtual ~GLTSMeshRenderer();
   
   GLuint mPB;
   int mNumIndices;
   
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

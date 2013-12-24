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

#ifndef _TSRENDERDATA_H_
#define _TSRENDERDATA_H_

#ifndef _MMATRIX_H_
#include "math/mMatrix.h"
#endif

#ifndef _MRECT_H_
#include "math/mRect.h"
#endif

#ifndef _DATACHUNKER_H_
#include "core/dataChunker.h"
#endif

#ifndef _TVECTOR_H_
#include "core/util/tVector.h"
#endif

//-----------------------------------------------------------------------------

BEGIN_NS(DTShape)

//-----------------------------------------------------------------------------

class TSSceneRenderState;
class Frustum;
class TSMaterialInstance;
class TSMeshRenderer;
class TSMesh;
class TSMeshInstanceRenderData;

typedef U32 TSRenderInstTypeHash;

class TSRenderState;

//**************************************************************************
// Render Instance
//**************************************************************************
typedef struct TSRenderInst
{
   /// The type of render instance this is.
   TSRenderInstTypeHash type;
   
   /// This should be true if the object needs to be sorted
   /// back to front with other translucent instances.
   /// @see sortDistSq
   bool translucentSort;
   
   /// The reference squared distance from the camera used for
   /// back to front sorting of the instances.
   /// @see translucentSort
   F32 sortDistSq;
   
   /// The default key used by render managers for
   /// internal sorting.
   U32 defaultKey;
   
   /// The secondary key used by render managers for
   /// internal sorting.
   U32 defaultKey2;
   
   /// Pointer to mesh
   TSMesh *mesh;
   
   /// Generic data attached to render instance (i.e. what we get from the object instance)
   TSMeshInstanceRenderData *renderData;
   
   /// If prim is NULL then this index is used to draw the
   /// indexed primitive from the primitive buffer.
   /// @see prim
   U32 primBuffIndex;
   
   /// The material to setup when drawing this instance.
   TSMaterialInstance *matInst;
   
   /// The object to world transform (world transform in most API's).
   const MatrixF *objectToWorld;
   
   /// The worldToCamera (view transform in most API's).
   const MatrixF* worldToCamera;
   
   /// The projection matrix.
   const MatrixF* projection;
   
   // misc render states
   U8    transFlags;
   bool  reflective;
   F32   visibility;
   
   void clear();
   void render(TSRenderState *state);
} TSRenderInst;

// Generic interface which provides scene info to the rendering code
class TSSceneRenderState
{
public:
   virtual ~TSSceneRenderState(){;}
   virtual TSMaterialInstance *getOverrideMaterial( TSMaterialInstance *inst ) const=0;
   
   virtual Point3F getCameraPosition() const=0;
   virtual Point3F getDiffuseCameraPosition() const=0;
   virtual RectF getViewport() const=0;
   virtual Point2F getWorldToScreenScale() const=0;
   
   virtual const MatrixF *getWorldMatrix() const=0;
   virtual bool isShadowPass() const=0;
   
   // Shared matrix stuff
   virtual const MatrixF *getViewMatrix() const = 0;
   virtual const MatrixF *getProjectionMatrix() const = 0;
};

/// A simple class for passing render state through the pre-render pipeline.
///
/// @section TSRenderState_intro Introduction
///
/// TSRenderState holds on to certain pieces of data that may be
/// set at the preparation stage of rendering (prepRengerImage etc.)
/// which are needed further along in the process of submitting
/// a render instance for later rendering by the RenderManager.
///
/// It was created to clean up and refactor the DTS rendering
/// from having a large number of static data that would be used
/// in varying places.  These statics were confusing and would often
/// cause problems when not properly cleaned up by various objects after
/// submitting their RenderInstances.
///
/// @section TSRenderState_functionality What Does TSRenderState Do?
///
/// TSRenderState is a simple class that performs the function of passing along
/// (from the prep function(s) to the actual submission) the data 
/// needed for the desired state of rendering.
///
/// @section TSRenderState_example Usage Example
///
/// TSRenderState is very easy to use.  Merely create a TSRenderState object (in prepRenderImage usually)
/// and set any of the desired data members (SceneRenderState, camera transform etc.), and pass the address of
/// your TSRenderState to your render function.
///
class TSRenderState
{
protected:
   
   // user-supplied interface which 
   const TSSceneRenderState *mState;
   
public:
   // Current world transform matrix
   MatrixF mWorldMatrix;
   
   /// Used to override the normal
   /// fade value of an object.
   /// This is multiplied by the current
   /// fade value of the instance
   /// to gain the resulting visibility fade (see TSMesh::render()).
   F32 mFadeOverride;

   /// These are used in some places
   /// TSShapeInstance::render, however,
   /// it appears they are never set to anything
   /// other than false.  We provide methods
   /// for setting them regardless.
   bool mNoRenderTranslucent;
   bool mNoRenderNonTranslucent;

   /// A generic hint value passed from the game
   /// code down to the material for use by shader 
   /// features.
   void *mMaterialHint;

   /// An optional object space frustum used to cull
   /// subobjects within the shape.
   const Frustum *mCuller;

   /// Use the origin point of the mesh for distance
   /// sorting for transparency instead of the nearest
   /// bounding box point.
   bool mUseOriginSort;
   
   /// Generic pointer to a render data object
   TSMeshInstanceRenderData *mRenderData;

protected:
   MultiTypedChunker mChunker;
   
public:
   Vector<TSRenderInst*> mRenderInsts;
   Vector<TSRenderInst*> mTranslucentRenderInsts;

public:

   TSRenderState();
   TSRenderState( TSRenderState &state );
   
   void reset();

   /// @name Get/Set methods.
   /// @{

   ///@see mState
   const TSSceneRenderState* getSceneState() const { return mState; }
   void setSceneState( const TSSceneRenderState *state ) { mState = state; }
   
   ///Sets a render
   TSMeshInstanceRenderData *getCurrentRenderData() const { return mRenderData; }
   void setCurrentRenderData(TSMeshInstanceRenderData *data) { mRenderData = data; }

   ///@see mFadeOverride
   F32 getFadeOverride() const { return mFadeOverride; }
   void setFadeOverride( F32 fade ) { mFadeOverride = fade; }

   ///@see mNoRenderTranslucent
   bool isNoRenderTranslucent() const { return mNoRenderTranslucent; }
   void setNoRenderTranslucent( bool noRenderTrans ) { mNoRenderTranslucent = noRenderTrans; }

   ///@see mNoRenderNonTranslucent
   bool isNoRenderNonTranslucent() const { return mNoRenderNonTranslucent; }
   void setNoRenderNonTranslucent( bool noRenderNonTrans ) { mNoRenderNonTranslucent = noRenderNonTrans; }

   ///@see mMaterialHint
   void* getMaterialHint() const { return mMaterialHint; }
   void setMaterialHint( void *materialHint ) { mMaterialHint = materialHint; }

   ///@see mCuller
   const Frustum* getCuller() const { return mCuller; }
   void setCuller( const  Frustum *culler ) { mCuller = culler; }

   ///@see mUseOriginSort
   void setOriginSort( bool enable ) { mUseOriginSort = enable; }
   bool useOriginSort() const { return mUseOriginSort; }
   
   /// Allocates a new TSRenderInst
   TSRenderInst *allocRenderInst();
   
   /// Allocates a new world matrix
   MatrixF *allocMatrix(const MatrixF &transform);
   
   /// Adds a new TSRenderInst to the rendering pool
   void addRenderInst(TSRenderInst *inst);
   
   /// Sorts TSRenderInsts
   void sortRenderInsts();

   /// @}
};


// Generic class to store render data
class TSMeshInstanceRenderData
{
public:
   TSMeshInstanceRenderData(){;}
   virtual ~TSMeshInstanceRenderData(){;}
   
   static TSMeshInstanceRenderData *create();
};

// Generic interface which stores vertex buffers and such for TSMeshes
class TSMeshRenderer
{
public:
   TSMeshRenderer() {;}
   virtual ~TSMeshRenderer() {;}
   
   /// Prepares any static buffers, and also any dynamic ones if
   /// renderState != NULL
   virtual void prepare(TSMesh *mesh, TSMeshInstanceRenderData *meshRenderData) = 0;
   
   /// Return a vertex buffer to update,
   /// or NULL if you want mesh->mVertexData
   /// to be directly updated
   virtual U8* mapVerts(TSMesh *mesh, TSMeshInstanceRenderData *meshRenderData) = 0;
   
   /// Counterpart to mapVerts, use this to unmap
   /// any mapped vertex buffers
   virtual void unmapVerts(TSMesh *mesh, TSMeshInstanceRenderData *meshRenderData) = 0;
   
   /// Called when a TSRenderInst is queued for rendering
   /// Use this to store the state of any dynamic vertex buffers
   virtual void onAddRenderInst(TSMesh *mesh, TSRenderInst *inst, TSRenderState *renderState) = 0;
   
   /// Renders whatever needs to be drawn, usually called AFTER the main
   virtual void doRenderInst(TSMesh *mesh, TSRenderInst *inst, TSRenderState *renderState) = 0;
   
   /// Returns true if buffers need updating
   virtual bool isDirty(TSMesh *mesh, TSMeshInstanceRenderData *renderData) = 0;
   
   /// Cleans up Mesh renderer
   virtual void clear() = 0;
   
   /// Factory function to create TSMeshRenderer
   static TSMeshRenderer *create();
};

//-----------------------------------------------------------------------------

END_NS

#endif // _TSRENDERDATA_H_

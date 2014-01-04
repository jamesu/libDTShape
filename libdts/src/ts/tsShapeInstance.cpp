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

#include "platform/platform.h"
#include "ts/tsShapeInstance.h"

#include "ts/tsLastDetail.h"
#include "ts/tsMaterialList.h"
#include "ts/tsDecal.h"
#include "platform/profiler.h"
#include "core/frameAllocator.h"
#include "ts/tsMaterialManager.h"
#include "ts/tsMaterial.h"
#include "math/util/frustum.h"

//-----------------------------------------------------------------------------

BEGIN_NS(DTShape)

//-------------------------------------------------------------------------------------
// constructors, destructors, initialization
//-------------------------------------------------------------------------------------

TSShapeInstance::TSShapeInstance( TSShape *shape, TSRenderState *renderState, bool loadMaterials )
{
   VECTOR_SET_ASSOCIATION(mMeshObjects);
   VECTOR_SET_ASSOCIATION(mNodeTransforms);
   VECTOR_SET_ASSOCIATION(mNodeReferenceRotations);
   VECTOR_SET_ASSOCIATION(mNodeReferenceTranslations);
   VECTOR_SET_ASSOCIATION(mNodeReferenceUniformScales);
   VECTOR_SET_ASSOCIATION(mNodeReferenceScaleFactors);
   VECTOR_SET_ASSOCIATION(mNodeReferenceArbitraryScaleRots);
   VECTOR_SET_ASSOCIATION(mThreadList);
   VECTOR_SET_ASSOCIATION(mTransitionThreads);

   mShape = shape;
   mCurrentRenderState = renderState;
   buildInstanceData( mShape, loadMaterials );
}

TSShapeInstance::~TSShapeInstance()
{
   // Clear render data for mesh objects
   for (S32 i=0; i<mMeshObjects.size(); i++)
   {
      MeshObjectInstance * objInst = &mMeshObjects[i];
      if (objInst->renderInstData)
         delete objInst->renderInstData;
   }
   
   mMeshObjects.clear();

   while (mThreadList.size())
      destroyThread(mThreadList.last());

   setMaterialList(NULL);

   delete [] mDirtyFlags;
}

void TSShapeInstance::buildInstanceData(TSShape * _shape, bool loadMaterials)
{
   mShape = _shape;

   debrisRefCount = 0;

   mCurrentDetailLevel = 0;
   mCurrentIntraDetailLevel = 1.0f;

   // all triggers off at start
   mTriggerStates = 0;

   //
   mAlphaAlways = false;
   mAlphaAlwaysValue = 1.0f;

   // material list...
   mMaterialList = NULL;
   mOwnMaterialList = false;

   //
   mData = 0;
   mScaleCurrentlyAnimated = false;

   if(loadMaterials)
      setMaterialList(mShape->materialList);

   // set up node data
   initNodeTransforms();

   // add objects to trees
   initMeshObjects();

   // set up subtree data
   S32 ss = mShape->subShapeFirstNode.size(); // we have this many subtrees
   mDirtyFlags = new U32[ss];

   mGroundThread = NULL;
   mCurrentDetailLevel = 0;

   animateSubtrees();

   // Construct billboards if not done already
   if ( loadMaterials && mShape )
      mShape->setupBillboardDetails( mShape->getPath().getFullPath() );
}

void TSShapeInstance::initNodeTransforms()
{
   // set up node data
   S32 numNodes = mShape->nodes.size();
   mNodeTransforms.setSize(numNodes);
}

void TSShapeInstance::initMeshObjects()
{
   // add objects to trees
   S32 numObjects = mShape->objects.size();
   mMeshObjects.setSize(numObjects);
   for (S32 i=0; i<numObjects; i++)
   {
      const TSObject * obj = &mShape->objects[i];
      MeshObjectInstance * objInst = &mMeshObjects[i];

      // hook up the object to it's node and transforms.
      objInst->mTransforms = &mNodeTransforms;
      objInst->nodeIndex = obj->nodeIndex;

      // set up list of meshes
      if (obj->numMeshes)
         objInst->meshList = &mShape->meshes[obj->startMeshIndex];
      else
         objInst->meshList = NULL;

      objInst->object = obj;
      objInst->forceHidden = false;
      
      objInst->renderInstData = TSMeshInstanceRenderData::create();
   }
}

void TSShapeInstance::setMaterialList( TSMaterialList *matList )
{
   // get rid of old list
   if ( mOwnMaterialList )
      delete mMaterialList;

   mMaterialList = matList;
   mOwnMaterialList = false;

   // If the material list is already be mapped then
   // don't bother doing the initializing a second time.
   // Note: only check the last material instance as this will catch both
   // uninitialised lists, as well as initialised lists that have had new
   // materials appended
   if ( mMaterialList && !mMaterialList->getMaterialInst( mMaterialList->size()-1 ) )
   {
      mMaterialList->setTextureLookupPath( mShape->getPath().getPath() );
      mMaterialList->mapMaterials();
      //Material::sAllowTextureTargetAssignment = true;
      initMaterialList();
      //Material::sAllowTextureTargetAssignment = false;
   }
}

void TSShapeInstance::cloneMaterialList()
{
   if ( mOwnMaterialList )
      return;

   mMaterialList = new TSMaterialList(mMaterialList);
   initMaterialList();

   mOwnMaterialList = true;
}

void TSShapeInstance::initMaterialList( )
{
   // Initialize the materials.
   mMaterialList->initMatInstances( mShape->getVertexFormat() );

   // TODO: It would be good to go thru all the meshes and
   // pre-create all the active material hooks for shadows,
   // reflections, and instancing.  This would keep these
   // hiccups from happening at runtime.
}

void TSShapeInstance::reSkin( String newBaseName, String oldBaseName )
{
   if( newBaseName.isEmpty() )
      newBaseName = "base";
   if( oldBaseName.isEmpty() )
      oldBaseName = "base";

   if ( newBaseName.equal( oldBaseName, String::NoCase ) )
      return;

   const U32 oldBaseNameLength = oldBaseName.length();

   // Make our own copy of the materials list from the resource if necessary
   if (ownMaterialList() == false)
      cloneMaterialList();

   TSMaterialList* pMatList = getMaterialList();
   pMatList->setTextureLookupPath( mShape->getPath().getPath() );

   // Cycle through the materials
   const Vector<String> &materialNames = pMatList->getMaterialNameList();
   for ( S32 i = 0; i < materialNames.size(); i++ )
   {
      // Try changing base
      const String &pName = materialNames[i];
      if ( pName.compare( oldBaseName, oldBaseNameLength, String::NoCase ) == 0 )
      {
         String newName( pName );
         newName.replace( 0, oldBaseNameLength, newBaseName );
         pMatList->renameMaterial( i, newName );
      }
   }

   // Initialize the material instances
   initMaterialList();
}

//-------------------------------------------------------------------------------------
// Render & detail selection
//-------------------------------------------------------------------------------------


void TSShapeInstance::beginUpdate(TSRenderState *newRenderState)
{
   mCurrentRenderState = newRenderState;
}

void TSShapeInstance::renderDebugNormals( F32 normalScalar, S32 dl )
{
#if 0
   if ( dl < 0 )
      return;

   AssertFatal( dl >= 0 && dl < mShape->details.size(),
      "TSShapeInstance::renderDebugNormals() - Bad detail level!" );

   static GFXStateBlockRef sb;
   if ( sb.isNull() )
   {
      GFXStateBlockDesc desc;
      desc.setCullMode( GFXCullNone );
      desc.setZReadWrite( true );
      desc.zWriteEnable = false;
      desc.vertexColorEnable = true;

      sb = GFX->createStateBlock( desc );
   }
   GFX->setStateBlock( sb );

   const TSDetail *detail = &mShape->details[dl];
   const S32 ss = detail->subShapeNum;
   if ( ss < 0 )
      return;

   const S32 start = mShape->subShapeFirstObject[ss];
   const S32 end   = start + mShape->subShapeNumObjects[ss];

   for ( S32 i = start; i < end; i++ )
   {
      MeshObjectInstance *meshObj = &mMeshObjects[i];
      if ( !meshObj )
         continue;

      const MatrixF &meshMat = meshObj->getTransform();

      // Then go through each TSMesh...
      U32 m = 0;
      for( TSMesh *mesh = meshObj->getMesh(m); mesh != NULL; mesh = meshObj->getMesh(m++) )
      {
         // and pull out the list of normals.
         const U32 numNrms = mesh->mNumVerts;
         PrimBuild::begin( GFXLineList, 2 * numNrms );
         for ( U32 n = 0; n < numNrms; n++ )
         {
            Point3F norm = mesh->mVertexData[n].normal();
            Point3F vert = mesh->mVertexData[n].vert();

            meshMat.mulP( vert );
            meshMat.mulV( norm );

            // Then render them.
            PrimBuild::color4f( mFabs( norm.x ), mFabs( norm.y ), mFabs( norm.z ), 1.0f );
            PrimBuild::vertex3fv( vert );
            PrimBuild::vertex3fv( vert + (norm * normalScalar) );
         }

         PrimBuild::end();
      }
   }
#endif
}

void TSShapeInstance::renderDebugNodes()
{
#if 0
   GFXDrawUtil *drawUtil = GFX->getDrawUtil();
   ColorI color( 255, 0, 0, 255 );

   GFXStateBlockDesc desc;
   desc.setBlend( false );
   desc.setZReadWrite( false, false );

   for ( U32 i = 0; i < mNodeTransforms.size(); i++ )
      drawUtil->drawTransform( desc, mNodeTransforms[i], NULL, NULL );
#endif
}

void TSShapeInstance::listMeshes( const String &state ) const
{
   if ( state.equal( "All", String::NoCase ) )
   {
      for ( U32 i = 0; i < mMeshObjects.size(); i++ )
      {
         const MeshObjectInstance &mesh = mMeshObjects[i];
         Log::warnf( "meshidx %3d, %8s, %s", i, ( mesh.forceHidden ) ? "Hidden" : "Visible", mShape->getMeshName(i).c_str() );         
      }
   }
   else if ( state.equal( "Hidden", String::NoCase ) )
   {
      for ( U32 i = 0; i < mMeshObjects.size(); i++ )
      {
         const MeshObjectInstance &mesh = mMeshObjects[i];
         if ( mesh.forceHidden )
            Log::warnf( "meshidx %3d, %8s, %s", i, "Visible", mShape->getMeshName(i).c_str() );         
      }
   }
   else if ( state.equal( "Visible", String::NoCase ) )
   {
      for ( U32 i = 0; i < mMeshObjects.size(); i++ )
      {
         const MeshObjectInstance &mesh = mMeshObjects[i];
         if ( !mesh.forceHidden )
            Log::warnf( "meshidx %3d, %8s, %s", i, "Hidden", mShape->getMeshName(i).c_str() );         
      }
   }
   else
   {
      Log::warnf( "TSShapeInstance::listMeshes( %s ) - only All/Hidden/Visible are valid parameters." );
   }
}

void TSShapeInstance::render( TSRenderState &rdata )
{
   if (mCurrentDetailLevel<0)
      return;

   PROFILE_SCOPE( TSShapeInstance_Render );

   // alphaIn:  we start to alpha-in next detail level when intraDL > 1-alphaIn-alphaOut
   //           (finishing when intraDL = 1-alphaOut)
   // alphaOut: start to alpha-out this detail level when intraDL > 1-alphaOut
   // NOTE:
   //   intraDL is at 1 when if shape were any closer to us we'd be at dl-1,
   //   intraDL is at 0 when if shape were any farther away we'd be at dl+1
   F32 alphaOut = mShape->alphaOut[mCurrentDetailLevel];
   F32 alphaIn  = mShape->alphaIn[mCurrentDetailLevel];
   F32 saveAA = mAlphaAlways ? mAlphaAlwaysValue : 1.0f;

   /// This first case is the single detail level render.
   if ( mCurrentIntraDetailLevel > alphaIn + alphaOut )
      render( rdata, mCurrentDetailLevel, mCurrentIntraDetailLevel );
   else if ( mCurrentIntraDetailLevel > alphaOut )
   {
      // draw this detail level w/ alpha=1 and next detail level w/
      // alpha=1-(intraDl-alphaOut)/alphaIn

      // first draw next detail level
      if ( mCurrentDetailLevel + 1 < mShape->details.size() && mShape->details[ mCurrentDetailLevel + 1 ].size > 0.0f )
      {
         setAlphaAlways( saveAA * ( alphaIn + alphaOut - mCurrentIntraDetailLevel ) / alphaIn );
         render( rdata, mCurrentDetailLevel + 1, 0.0f );
      }

      setAlphaAlways( saveAA );
      render( rdata, mCurrentDetailLevel, mCurrentIntraDetailLevel );
   }
   else
   {
      // draw next detail level w/ alpha=1 and this detail level w/
      // alpha = 1-intraDL/alphaOut

      // first draw next detail level
      if ( mCurrentDetailLevel + 1 < mShape->details.size() && mShape->details[ mCurrentDetailLevel + 1 ].size > 0.0f )
         render( rdata, mCurrentDetailLevel+1, 0.0f );

      setAlphaAlways( saveAA * mCurrentIntraDetailLevel / alphaOut );
      render( rdata, mCurrentDetailLevel, mCurrentIntraDetailLevel );
      setAlphaAlways( saveAA );
   }
}

void TSShapeInstance::setMeshForceHidden( const char *meshName, bool hidden )
{
   Vector<MeshObjectInstance>::iterator iter = mMeshObjects.begin();
   for ( ; iter != mMeshObjects.end(); iter++ )
   {
      S32 nameIndex = iter->object->nameIndex;
      const char *name = mShape->names[ nameIndex ];

      if ( dStrcmp( meshName, name ) == 0 )
      {
         iter->forceHidden = hidden;
         return;
      }
   }
}

void TSShapeInstance::setMeshForceHidden( S32 meshIndex, bool hidden )
{
   AssertFatal( meshIndex > -1 && meshIndex < mMeshObjects.size(),
      "TSShapeInstance::setMeshForceHidden - Invalid index!" );
                  
   mMeshObjects[meshIndex].forceHidden = hidden;
}

void TSShapeInstance::render( TSRenderState &rdata, S32 dl, F32 intraDL )
{
   AssertFatal( dl >= 0 && dl < mShape->details.size(),"TSShapeInstance::render" );

   S32 i;

   const TSDetail * detail = &mShape->details[dl];
   S32 ss = detail->subShapeNum;
   S32 od = detail->objectDetailNum;

   // if we're a billboard detail, draw it and exit
   if ( ss < 0 )
   {
      PROFILE_SCOPE( TSShapeInstance_RenderBillboards );
      
      if ( !rdata.isNoRenderTranslucent() && ( TSLastDetail::smCanShadow || !rdata.getSceneState()->isShadowPass() ) )
         mShape->billboardDetails[ dl ]->render( rdata, mAlphaAlways ? mAlphaAlwaysValue : 1.0f );

      return;
   }

   // run through the meshes   
   S32 start = rdata.isNoRenderNonTranslucent() ? mShape->subShapeFirstTranslucentObject[ss] : mShape->subShapeFirstObject[ss];
   S32 end   = rdata.isNoRenderTranslucent() ? mShape->subShapeFirstTranslucentObject[ss] : mShape->subShapeFirstObject[ss] + mShape->subShapeNumObjects[ss];
   for (i=start; i<end; i++)
   {
      // following line is handy for debugging, to see what part of the shape that it is rendering
      // const char *name = mShape->names[ mMeshObjects[i].object->nameIndex ];
      mMeshObjects[i].render( od, mMaterialList, rdata, mAlphaAlways ? mAlphaAlwaysValue : 1.0f );
   }
}

void TSShapeInstance::setCurrentDetail( S32 dl, F32 intraDL )
{
   PROFILE_SCOPE( TSShapeInstance_setCurrentDetail );

   mCurrentDetailLevel = mClamp( dl, -1, mShape->mSmallestVisibleDL );
   mCurrentIntraDetailLevel = intraDL > 1.0f ? 1.0f : (intraDL < 0.0f ? 0.0f : intraDL);

   // Restrict the chosen detail level by cutoff value.
   if ( (S32)mCurrentRenderState->smNumSkipRenderDetails > 0 && mCurrentDetailLevel >= 0 )
   {
      S32 cutoff = getMin( (S32)mCurrentRenderState->smNumSkipRenderDetails, mShape->mSmallestVisibleDL );
      if ( mCurrentDetailLevel < cutoff )
      {
         mCurrentDetailLevel = cutoff;
         mCurrentIntraDetailLevel = 1.0f;
      }
   }
}

S32 TSShapeInstance::setDetailFromPosAndScale(  const TSSceneRenderState *state,
                                                const Point3F &pos, 
                                                const Point3F &scale )
{
   VectorF camVector = pos - state->getDiffuseCameraPosition();
   F32 dist = getMax( camVector.len(), 0.01f );
   F32 invScale = ( 1.0f / getMax( getMax( scale.x, scale.y ), scale.z ) );

   return setDetailFromDistance( state, dist * invScale );
}

S32 TSShapeInstance::setDetailFromDistance( const TSSceneRenderState *state, F32 scaledDistance )
{
   PROFILE_SCOPE( TSShapeInstance_setDetailFromDistance );

   // For debugging/metrics.
   mCurrentRenderState->smLastScaledDistance = scaledDistance;

   // Shortcut if the distance is really close or negative.
   if ( scaledDistance <= 0.0f )
   {
      mShape->mDetailLevelLookup[0].get( mCurrentDetailLevel, mCurrentIntraDetailLevel );
      return mCurrentDetailLevel;
   }

   // The pixel scale is used the linearly scale the lod
   // selection based on the viewport size.
   //
   // The original calculation from TGEA was...
   //
   // pixelScale = viewport.extent.x * 1.6f / 640.0f;
   //
   // Since we now work on the viewport height, assuming
   // 4:3 aspect ratio, we've changed the reference value
   // to 300 to be more compatible with legacy shapes.
   //
   const F32 pixelScale = state->getViewport().extent.y / 300.0f;

   // This is legacy DTS support for older "multires" based
   // meshes.  The original crossbow weapon uses this.
   //
   // If we have more than one detail level and the maxError
   // is non-negative then we do some sort of screen error 
   // metric for detail selection.
   //
   if ( mShape->mUseDetailFromScreenError )
   {
      // The pixel size of 1 meter at the input distance.
      F32 pixelRadius = 1.0f;//TODOstate->projectRadius( scaledDistance, 1.0f ) * pixelScale;
      static const F32 smScreenError = 5.0f;
      return setDetailFromScreenError( smScreenError / pixelRadius );
   }

   // We're inlining TSSceneRenderState::projectRadius here to 
   // skip the unnessasary divide by zero protection.
   F32 pixelRadius = ( mShape->radius / scaledDistance ) * state->getWorldToScreenScale().y * pixelScale;
   F32 pixelSize = pixelRadius * mCurrentRenderState->smDetailAdjust;

   if (  pixelSize > mCurrentRenderState->smSmallestVisiblePixelSize &&
         pixelSize <= mShape->mSmallestVisibleSize )
      pixelSize = mShape->mSmallestVisibleSize + 0.01f;

   // For debugging/metrics.
   mCurrentRenderState->smLastPixelSize = pixelSize;

   // Clamp it to an acceptable range for the lookup table.
   U32 index = (U32)mClampF( pixelSize, 0, mShape->mDetailLevelLookup.size() - 1 );

   // Check the lookup table for the detail and intra detail levels.
   mShape->mDetailLevelLookup[ index ].get( mCurrentDetailLevel, mCurrentIntraDetailLevel );

   // Restrict the chosen detail level by cutoff value.
   if ( mCurrentRenderState->smNumSkipRenderDetails > 0 && mCurrentDetailLevel >= 0 )
   {
      S32 cutoff = getMin( (S32)mCurrentRenderState->smNumSkipRenderDetails, mShape->mSmallestVisibleDL );
      if ( mCurrentDetailLevel < cutoff )
      {
         mCurrentDetailLevel = cutoff;
         mCurrentIntraDetailLevel = 1.0f;
      }
   }

   return mCurrentDetailLevel;
}

S32 TSShapeInstance::setDetailFromScreenError( F32 errorTolerance )
{
   PROFILE_SCOPE( TSShapeInstance_setDetailFromScreenError );

   // For debugging/metrics.
   mCurrentRenderState->smLastScreenErrorTolerance = errorTolerance;

   // note:  we use 10 time the average error as the metric...this is
   // more robust than the maxError...the factor of 10 is to put average error
   // on about the same scale as maxError.  The errorTOL is how much
   // error we are able to tolerate before going to a more detailed version of the
   // shape.  We look for a pair of details with errors bounding our errorTOL,
   // and then we select an interpolation parameter to tween betwen them.  Ok, so
   // this isn't exactly an error tolerance.  A tween value of 0 is the lower poly
   // model (higher detail number) and a value of 1 is the higher poly model (lower
   // detail number).

   // deal with degenerate case first...
   // if smallest detail corresponds to less than half tolerable error, then don't even draw
   F32 prevErr;
   if ( mShape->mSmallestVisibleDL < 0 )
      prevErr = 0.0f;
   else
      prevErr = 10.0f * mShape->details[mShape->mSmallestVisibleDL].averageError * 20.0f;
   if ( mShape->mSmallestVisibleDL < 0 || prevErr < errorTolerance )
   {
      // draw last detail
      mCurrentDetailLevel = mShape->mSmallestVisibleDL;
      mCurrentIntraDetailLevel = 0.0f;
      return mCurrentDetailLevel;
   }

   // this function is a little odd
   // the reason is that the detail numbers correspond to
   // when we stop using a given detail level...
   // we search the details from most error to least error
   // until we fit under the tolerance (errorTOL) and then
   // we use the next highest detail (higher error)
   for (S32 i = mShape->mSmallestVisibleDL; i >= 0; i-- )
   {
      F32 err0 = 10.0f * mShape->details[i].averageError;
      if ( err0 < errorTolerance )
      {
         // ok, stop here

         // intraDL = 1 corresponds to fully this detail
         // intraDL = 0 corresponds to the next lower (higher number) detail
         mCurrentDetailLevel = i;
         mCurrentIntraDetailLevel = 1.0f - (errorTolerance - err0) / (prevErr - err0);
         return mCurrentDetailLevel;
      }
      prevErr = err0;
   }

   // get here if we are drawing at DL==0
   mCurrentDetailLevel = 0;
   mCurrentIntraDetailLevel = 1.0f;
   return mCurrentDetailLevel;
}

//-------------------------------------------------------------------------------------
// Object (MeshObjectInstance & PluginObjectInstance) render methods
//-------------------------------------------------------------------------------------

void TSShapeInstance::ObjectInstance::render( S32, TSMaterialList *, TSRenderState &rdata, F32 alpha )
{
   AssertFatal(0,"TSShapeInstance::ObjectInstance::render:  no default render method.");
}

void TSShapeInstance::MeshObjectInstance::render(  S32 objectDetail, 
                                                   TSMaterialList *materials, 
                                                   TSRenderState &rdata, 
                                                   F32 alpha )
{
   PROFILE_SCOPE( TSShapeInstance_MeshObjectInstance_render );

   if ( forceHidden || ( ( visible * alpha ) <= 0.01f ) )
      return;

   TSMesh *mesh = getMesh(objectDetail);
   if ( !mesh )
      return;

   const MatrixF &transform = getTransform();

   if ( rdata.getCuller() )
   {
      Box3F box( mesh->getBounds() );
      transform.mul( box );
      if ( rdata.getCuller()->isCulled( box ) )
         return;
   }

   //const MatrixF *worldMatrix = rdata.getSceneState()->getWorldMatrix();
   //rdata.mWorldMatrix = *worldMatrix;
   rdata.mWorldMatrix = transform;//.mul(transform);

   mesh->setFade( visible * alpha );
   
   rdata.setCurrentRenderData(renderInstData);

   // Pass a hint to the mesh that time has advanced and that the
   // skin is dirty and needs to be updated.  This should result
   // in the skin only updating once per frame in most cases.
   const U32 currTime = 0;// TODOSim::getCurrentTime();
   bool isSkinDirty = true;//currTime != mLastTime;

   mesh->render(  materials, 
                  rdata, 
                  isSkinDirty,
                  *mTransforms,
                  *mesh->mRenderer );

   // Update the last render time.
   mLastTime = currTime;
}

TSShapeInstance::MeshObjectInstance::MeshObjectInstance() 
   : meshList(0), object(0), frame(0), matFrame(0),
     visible(1.0f), forceHidden(false), mLastTime( 0 )
{
}

void TSShapeInstance::prepCollision()
{
   PROFILE_SCOPE( TSShapeInstance_PrepCollision );
}

//-----------------------------------------------------------------------------

END_NS

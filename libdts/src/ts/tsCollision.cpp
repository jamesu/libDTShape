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

#include "platform/platform.h"

#include "ts/tsShapeInstance.h"
#include "ts/tsMaterialList.h"
//#include "scene/sceneObject.h"
#include "collision/convex.h"
#include "collision/collision.h"
//#include "T3D/tsStatic.h" // TODO: We shouldn't have this dependancy!
//#include "T3D/physics/physicsPlugin.h"
#include "ts/physicsCollision.h"
#include "collision/concretePolyList.h"
#include "collision/vertexPolyList.h"
#include "platform/profiler.h"

//-------------------------------------------------------------------------------------
// Collision methods
//-------------------------------------------------------------------------------------

bool TSShapeInstance::buildPolyList(AbstractPolyList * polyList, S32 dl)
{
   // if dl==-1, nothing to do
   if (dl==-1)
      return false;

   AssertFatal(dl>=0 && dl<mShape->details.size(),"TSShapeInstance::buildPolyList");

   // get subshape and object detail
   const TSDetail * detail = &mShape->details[dl];
   S32 ss = detail->subShapeNum;
   S32 od = detail->objectDetailNum;

   // This detail does not have any geometry.
   if ( ss < 0 )
      return false;

   // nothing emitted yet...
   bool emitted = false;
   U32 surfaceKey = 0;

   S32 start = mShape->subShapeFirstObject[ss];
   S32 end   = mShape->subShapeNumObjects[ss] + start;
   if (start<end)
   {
      MatrixF initialMat;
      Point3F initialScale;
      polyList->getTransform(&initialMat,&initialScale);

      // set up for first object's node
      MatrixF mat;
      MatrixF scaleMat(true);
      F32* p = scaleMat;
      p[0]  = initialScale.x;
      p[5]  = initialScale.y;
      p[10] = initialScale.z;
      const MatrixF *previousMat = &mMeshObjects[start].getTransform();
      mat.mul(initialMat,scaleMat);
      mat.mul(*previousMat);
      polyList->setTransform(&mat,Point3F(1, 1, 1));

      // run through objects and collide
      for (S32 i=start; i<end; i++)
      {
         MeshObjectInstance * mesh = &mMeshObjects[i];

         if (od >= mesh->object->numMeshes)
            continue;

         if (&mesh->getTransform() != previousMat)
         {
            // different node from before, set up for this node
            previousMat = &mesh->getTransform();

            if (previousMat != NULL)
            {
               mat.mul(initialMat,scaleMat);
               mat.mul(*previousMat);
               polyList->setTransform(&mat,Point3F(1, 1, 1));
            }
         }
         // collide...
         emitted |= mesh->buildPolyList(od,polyList,surfaceKey,mMaterialList);
      }

      // restore original transform...
      polyList->setTransform(&initialMat,initialScale);
   }

   return emitted;
}

bool TSShapeInstance::getFeatures(const MatrixF& mat, const Point3F& n, ConvexFeature* cf, S32 dl)
{
   // if dl==-1, nothing to do
   if (dl==-1)
      return false;

   AssertFatal(dl>=0 && dl<mShape->details.size(),"TSShapeInstance::buildPolyList");

   // get subshape and object detail
   const TSDetail * detail = &mShape->details[dl];
   S32 ss = detail->subShapeNum;
   S32 od = detail->objectDetailNum;

   // nothing emitted yet...
   bool emitted = false;
   U32 surfaceKey = 0;

   S32 start = mShape->subShapeFirstObject[ss];
   S32 end   = mShape->subShapeNumObjects[ss] + start;
   if (start<end)
   {
      MatrixF final;
      const MatrixF* previousMat = &mMeshObjects[start].getTransform();
      final.mul(mat, *previousMat);

      // run through objects and collide
      for (S32 i=start; i<end; i++)
      {
         MeshObjectInstance * mesh = &mMeshObjects[i];

         if (od >= mesh->object->numMeshes)
            continue;

         if (&mesh->getTransform() != previousMat)
         {
            previousMat = &mesh->getTransform();
            final.mul(mat, *previousMat);
         }
         emitted |= mesh->getFeatures(od, final, n, cf, surfaceKey);
      }
   }
   return emitted;
}

bool TSShapeInstance::castRay(const Point3F & a, const Point3F & b, RayInfo * rayInfo, S32 dl)
{
   // if dl==-1, nothing to do
   if (dl==-1)
      return false;

   AssertFatal(dl>=0 && dl<mShape->details.size(),"TSShapeInstance::castRay");

   // get subshape and object detail
   const TSDetail * detail = &mShape->details[dl];
   S32 ss = detail->subShapeNum;
   S32 od = detail->objectDetailNum;

   // This detail has no geometry to hit.
   if ( ss < 0 )
      return false;

   S32 start = mShape->subShapeFirstObject[ss];
   S32 end   = mShape->subShapeNumObjects[ss] + start;
   RayInfo saveRay;
   saveRay.t = 1.0f;
   const MatrixF * saveMat = NULL;
   bool found = false;
   if (start<end)
   {
      Point3F ta, tb;

      // set up for first object's node
      MatrixF mat;
      const MatrixF * previousMat = &mMeshObjects[start].getTransform();
      mat = *previousMat;
      mat.inverse();
      mat.mulP(a,&ta);
      mat.mulP(b,&tb);

      // run through objects and collide
      for (S32 i=start; i<end; i++)
      {
         MeshObjectInstance * mesh = &mMeshObjects[i];

         if (od >= mesh->object->numMeshes)
            continue;

         if (&mesh->getTransform() != previousMat)
         {
            // different node from before, set up for this node
            previousMat = &mesh->getTransform();

            if (previousMat != NULL)
            {
               mat = *previousMat;
               mat.inverse();
               mat.mulP(a,&ta);
               mat.mulP(b,&tb);
            }
         }
         // collide...
         if (mesh->castRay(od,ta,tb,rayInfo, mMaterialList))
         {
            if (!rayInfo)
               return true;

            if (rayInfo->t <= saveRay.t)
            {
               saveRay = *rayInfo;
               saveMat = previousMat;
            }
            found = true;
         }
      }
   }

   // collide with any skins for this detail level...
   // TODO: if ever...

   // finalize the deal...
   if (found)
   {
      *rayInfo = saveRay;
      saveMat->mulV(rayInfo->normal);
      rayInfo->point  = b-a;
      rayInfo->point *= rayInfo->t;
      rayInfo->point += a;
   }
   return found;
}

bool TSShapeInstance::castRayRendered(const Point3F & a, const Point3F & b, RayInfo * rayInfo, S32 dl)
{
   // if dl==-1, nothing to do
   if (dl==-1)
      return false;

   AssertFatal(dl>=0 && dl<mShape->details.size(),"TSShapeInstance::castRayRendered");

   // get subshape and object detail
   const TSDetail * detail = &mShape->details[dl];
   S32 ss = detail->subShapeNum;
   S32 od = detail->objectDetailNum;

   if ( ss == -1 )
      return false;

   S32 start = mShape->subShapeFirstObject[ss];
   S32 end   = mShape->subShapeNumObjects[ss] + start;
   RayInfo saveRay;
   saveRay.t = 1.0f;
   const MatrixF * saveMat = NULL;
   bool found = false;
   if (start<end)
   {
      Point3F ta, tb;

      // set up for first object's node
      MatrixF mat;
      const MatrixF * previousMat = &mMeshObjects[start].getTransform();
      mat = *previousMat;
      mat.inverse();
      mat.mulP(a,&ta);
      mat.mulP(b,&tb);

      // run through objects and collide
      for (S32 i=start; i<end; i++)
      {
         MeshObjectInstance * mesh = &mMeshObjects[i];

         if (od >= mesh->object->numMeshes)
            continue;

         if (&mesh->getTransform() != previousMat)
         {
            // different node from before, set up for this node
            previousMat = &mesh->getTransform();

            if (previousMat != NULL)
            {
               mat = *previousMat;
               mat.inverse();
               mat.mulP(a,&ta);
               mat.mulP(b,&tb);
            }
         }
         // collide...
         if (mesh->castRayRendered(od,ta,tb,rayInfo, mMaterialList))
         {
            if (!rayInfo)
               return true;

            if (rayInfo->t <= saveRay.t)
            {
               saveRay = *rayInfo;
               saveMat = previousMat;
            }
            found = true;
         }
      }
   }

   // collide with any skins for this detail level...
   // TODO: if ever...

   // finalize the deal...
   if (found)
   {
      *rayInfo = saveRay;
      saveMat->mulV(rayInfo->normal);
      rayInfo->point  = b-a;
      rayInfo->point *= rayInfo->t;
      rayInfo->point += a;
   }
   return found;
}

Point3F TSShapeInstance::support(const Point3F & v, S32 dl)
{
   // if dl==-1, nothing to do
   AssertFatal(dl != -1, "Error, should never try to collide with a non-existant detail level!");
   AssertFatal(dl>=0 && dl<mShape->details.size(),"TSShapeInstance::support");

   // get subshape and object detail
   const TSDetail * detail = &mShape->details[dl];
   S32 ss = detail->subShapeNum;
   S32 od = detail->objectDetailNum;

   S32 start = mShape->subShapeFirstObject[ss];
   S32 end   = mShape->subShapeNumObjects[ss] + start;

   F32     currMaxDP   = -1e9f;
   Point3F currSupport = Point3F(0, 0, 0);
   const MatrixF * previousMat = NULL;
   MatrixF mat;
   if (start<end)
   {
      Point3F va;

      // set up for first object's node
      previousMat = &mMeshObjects[start].getTransform();
      mat = *previousMat;
      mat.inverse();

      // run through objects and collide
      for (S32 i=start; i<end; i++)
      {
         MeshObjectInstance * mesh = &mMeshObjects[i];

         if (od >= mesh->object->numMeshes)
            continue;

         TSMesh* physMesh = mesh->getMesh(od);
         if (physMesh && !mesh->forceHidden && mesh->visible > 0.01f)
         {
            // collide...
            if (&mesh->getTransform() != previousMat)
            {
               // different node from before, set up for this node
               previousMat = &mesh->getTransform();
               mat = *previousMat;
               mat.inverse();
            }
            mat.mulV(v, &va);
            physMesh->support(mesh->frame, va, &currMaxDP, &currSupport);
         }
      }
   }

   if (currMaxDP != -1e9f)
   {
      previousMat->mulP(currSupport);
      return currSupport;
   }
   else
   {
      return Point3F(0, 0, 0);
   }
}

void TSShapeInstance::computeBounds(S32 dl, Box3F & bounds)
{
   // if dl==-1, nothing to do
   if (dl==-1)
      return;

   AssertFatal(dl>=0 && dl<mShape->details.size(),"TSShapeInstance::computeBounds");

   // get subshape and object detail
   const TSDetail * detail = &mShape->details[dl];
   S32 ss = detail->subShapeNum;
   S32 od = detail->objectDetailNum;

   // use shape bounds for imposter details
   if (ss < 0)
   {
      bounds = mShape->bounds;
      return;
   }

   S32 start = mShape->subShapeFirstObject[ss];
   S32 end   = mShape->subShapeNumObjects[ss] + start;

   // run through objects and updating bounds as we go
   bounds.minExtents.set( 10E30f, 10E30f, 10E30f);
   bounds.maxExtents.set(-10E30f,-10E30f,-10E30f);
   Box3F box;
   for (S32 i=start; i<end; i++)
   {
      MeshObjectInstance * mesh = &mMeshObjects[i];

      if (od >= mesh->object->numMeshes)
         continue;

      if (mesh->getMesh(od))
      {
         mesh->getMesh(od)->computeBounds(mesh->getTransform(),box, 0); // use frame 0 so TSSkinMesh uses skinned verts to compute bounds
         bounds.minExtents.setMin(box.minExtents);
         bounds.maxExtents.setMax(box.maxExtents);
      }
   }
}

//-------------------------------------------------------------------------------------
// Object (MeshObjectInstance & PluginObjectInstance) collision methods
//-------------------------------------------------------------------------------------

bool TSShapeInstance::ObjectInstance::buildPolyList(S32 objectDetail, AbstractPolyList *polyList, U32 &surfaceKey, TSMaterialList *materials )
{
   TWISTFORK_UNUSED( objectDetail );
   TWISTFORK_UNUSED( polyList );
   TWISTFORK_UNUSED( surfaceKey );
   TWISTFORK_UNUSED( materials );

   AssertFatal(0,"TSShapeInstance::ObjectInstance::buildPolyList:  no default method.");
   return false;
}

bool TSShapeInstance::ObjectInstance::getFeatures(S32 objectDetail, const MatrixF& mat, const Point3F& n, ConvexFeature* cf, U32& surfaceKey)
{
   TWISTFORK_UNUSED( objectDetail );
   TWISTFORK_UNUSED( mat );
   TWISTFORK_UNUSED( n );
   TWISTFORK_UNUSED( cf );
   TWISTFORK_UNUSED( surfaceKey );

   AssertFatal(0,"TSShapeInstance::ObjectInstance::buildPolyList:  no default method.");
   return false;
}

void TSShapeInstance::ObjectInstance::support(S32, const Point3F&, F32*, Point3F*)
{
   AssertFatal(0,"TSShapeInstance::ObjectInstance::supprt:  no default method.");
}

bool TSShapeInstance::ObjectInstance::castRay( S32 objectDetail, const Point3F &start, const Point3F &end, RayInfo *rayInfo, TSMaterialList *materials )
{
   TWISTFORK_UNUSED( objectDetail );
   TWISTFORK_UNUSED( start );
   TWISTFORK_UNUSED( end );
   TWISTFORK_UNUSED( rayInfo );

   AssertFatal(0,"TSShapeInstance::ObjectInstance::castRay:  no default method.");
   return false;
}

bool TSShapeInstance::MeshObjectInstance::buildPolyList( S32 objectDetail, AbstractPolyList *polyList, U32 &surfaceKey, TSMaterialList *materials )
{
   TSMesh * mesh = getMesh(objectDetail);
   if (mesh && !forceHidden && visible>0.01f)
      return mesh->buildPolyList(frame,polyList,surfaceKey,materials);
   return false;
}

bool TSShapeInstance::MeshObjectInstance::getFeatures(S32 objectDetail, const MatrixF& mat, const Point3F& n, ConvexFeature* cf, U32& surfaceKey)
{
   TSMesh* mesh = getMesh(objectDetail);
   if (mesh && !forceHidden && visible > 0.01f)
      return mesh->getFeatures(frame, mat, n, cf, surfaceKey);
   return false;
}

void TSShapeInstance::MeshObjectInstance::support(S32 objectDetail, const Point3F& v, F32* currMaxDP, Point3F* currSupport)
{
   TSMesh* mesh = getMesh(objectDetail);
   if (mesh && !forceHidden && visible > 0.01f)
      mesh->support(frame, v, currMaxDP, currSupport);
}


bool TSShapeInstance::MeshObjectInstance::castRay( S32 objectDetail, const Point3F & start, const Point3F & end, RayInfo * rayInfo, TSMaterialList* materials )
{
   TSMesh* mesh = getMesh( objectDetail );
   if( mesh && !forceHidden && visible > 0.01f )
      return mesh->castRay( frame, start, end, rayInfo, materials );
   return false;
}

bool TSShapeInstance::MeshObjectInstance::castRayRendered( S32 objectDetail, const Point3F & start, const Point3F & end, RayInfo * rayInfo, TSMaterialList* materials )
{
   TSMesh* mesh = getMesh( objectDetail );
   if( mesh && !forceHidden && visible > 0.01f )
      return mesh->castRayRendered( frame, start, end, rayInfo, materials );
   return false;
}

void TSShape::findColDetails( bool useVisibleMesh, Vector<S32> *outDetails, Vector<S32> *outLOSDetails ) const
{
   PROFILE_SCOPE( TSShape_findColDetails );

   if ( useVisibleMesh )
   {
      // If we're using the visible mesh for collision then
      // find the highest detail and use that.

      U32 highestDetail = -1;
      F32 highestSize = -F32_MAX;

      for ( U32 i = 0; i < details.size(); i++ )
      {
         // Make sure we skip any details that shouldn't be rendered
         if ( details[i].size < 0 )
            continue;

         /*
         // Also make sure we skip any collision details with a size.
         const String &name = names[details[i].nameIndex];
         if (  dStrStartsWith( name, "Collision" ) ||
               dStrStartsWith( name, "LOS" ) )
            continue;
         */

         // Otherwise test against the current highest size
         if ( details[i].size > highestSize )
         {
            highestDetail = i;
            highestSize = details[i].size;
         }
      }

      // We use the same detail for both raycast and collisions.
      if ( highestDetail != -1 )
      {
         outDetails->push_back( highestDetail );
         if ( outLOSDetails )
            outLOSDetails->push_back( highestDetail );
      }

      return;
   }

   // Detail meshes starting with COL or LOS is considered
   // to be a collision mesh.
   //
   // The LOS (light of sight) details are used for raycasts.

   for ( U32 i = 0; i < details.size(); i++ )
   {
      const String &name = names[ details[i].nameIndex ];
      if ( !dStrStartsWith( name, "Collision" ) )
         continue;

      outDetails->push_back(i);

      // If we're not returning LOS details then skip out.
      if ( !outLOSDetails )
         continue;

      // The way LOS works is that it will check to see if there is a LOS detail that matches
      // the the collision detail + 1 + MaxCollisionShapes (this variable name should change in
      // the future). If it can't find a matching LOS it will simply use the collision instead.
      // We check for any "unmatched" LOS's further down.

      // Extract the detail number from the name.
      S32 number = 0;
      String::GetTrailingNumber( name, number );
      
      // Look for a matching LOS collision detail.
      //
      // TODO: Fix the old 9 detail offset which is there
      // because you cannot have two detail markers with
      // the same detail number.
      //
      const S32 LOSOverrideOffset = 9;
      String buff = String::ToString( "LOS-%d", mAbs( number ) + LOSOverrideOffset );
      S32 los = findDetail( buff );
      
      // If we didn't find the lod detail then use the
      // normal collision detail for LOS tests.
      if ( los == -1 )
         los = i;

      outLOSDetails->push_back( los );
   }

   // If we're not returning LOS details then skip out.
   if ( !outLOSDetails )
      return;

   // Snag any "unmatched" LOS details and put 
   // them at the end of the list.
   for ( U32 i = 0; i < details.size(); i++ )
   {
      const String &name = names[ details[i].nameIndex ];
      if ( !dStrStartsWith( name, "LOS" ) )
         continue;

      // See if we already have this LOS
      bool found = false;
      for (U32 j = 0; j < outLOSDetails->size(); j++)
      {
         if ( (*outLOSDetails)[j] == i )
         {
            found = true;
            break;
         }
      }

      if ( !found )
         outLOSDetails->push_back(i);
   }
}

PhysicsCollision* TSShape::buildColShape( bool useVisibleMesh, const Point3F &scale )
{
   return _buildColShapes( useVisibleMesh, scale, NULL, false );
}

void TSShape::buildColShapes( bool useVisibleMesh, const Point3F &scale, Vector< CollisionShapeInfo > *list )
{
   _buildColShapes( useVisibleMesh, scale, list, true );
}

PhysicsCollision* TSShape::_buildColShapes( bool useVisibleMesh, const Point3F &scale, Vector< CollisionShapeInfo > *list, bool perMesh )
{
   PROFILE_SCOPE( TSShape_buildColShapes );

   PhysicsCollision *colShape = NULL;
   U32 surfaceKey = 0;

   if ( useVisibleMesh )
   {
      // Here we build triangle collision meshes from the
      // visible detail levels.

      // A negative subshape on the detail means we don't have geometry.
      const TSShape::Detail &detail = details[0];     
      if ( detail.subShapeNum < 0 )
         return NULL;

      // We don't try to optimize the triangles we're given
      // and assume the art was created properly for collision.
      ConcretePolyList polyList;
      polyList.setTransform( &MatrixF::Identity, scale );

      // Create the collision meshes.
      S32 start = subShapeFirstObject[ detail.subShapeNum ];
      S32 end = start + subShapeNumObjects[ detail.subShapeNum ];
      for ( S32 o=start; o < end; o++ )
      {
         const TSShape::Object &object = objects[o];
         if ( detail.objectDetailNum >= object.numMeshes )
            continue;

         // No mesh or no verts.... nothing to do.
         TSMesh *mesh = meshes[ object.startMeshIndex + detail.objectDetailNum ];
         if ( !mesh || mesh->mNumVerts == 0 )
            continue;

         // Gather the mesh triangles.
         polyList.clear();
         mesh->buildPolyList( 0, &polyList, surfaceKey, NULL );

         // Create the collision shape if we haven't already.
         if ( !colShape )
            colShape = PhysicsCollision::create();

         // Get the object space mesh transform.
         MatrixF localXfm;
         getNodeWorldTransform( object.nodeIndex, &localXfm );

         colShape->addTriangleMesh( polyList.mVertexList.address(),
            polyList.mVertexList.size(),
            polyList.mIndexList.address(),
            polyList.mIndexList.size() / 3,
            localXfm );

         if ( perMesh )
         {
            list->increment();
            list->last().colNode = -1;
            list->last().colShape = colShape;
            colShape = NULL;
         }
      }

      // Return what we built... if anything.
      return colShape;
   }


   // Scan out the collision hulls...
   //
   // TODO: We need to support LOS collision for physics.
   //
   for ( U32 i = 0; i < details.size(); i++ )
   {
      const TSShape::Detail &detail = details[i];
      const String &name = names[detail.nameIndex];

      // Is this a valid collision detail.
      if ( !dStrStartsWith( name, "Collision" ) || detail.subShapeNum < 0 )
         continue;

      // Now go thru the meshes for this detail.
      S32 start = subShapeFirstObject[ detail.subShapeNum ];
      S32 end = start + subShapeNumObjects[ detail.subShapeNum ];
      if ( start >= end )
         continue;         

      for ( S32 o=start; o < end; o++ )
      {
         const TSShape::Object &object = objects[o];
         const String &meshName = names[ object.nameIndex ];

         if ( object.numMeshes <= detail.objectDetailNum )
            continue;

         // No mesh, a flat bounds, or no verts.... nothing to do.
         TSMesh *mesh = meshes[ object.startMeshIndex + detail.objectDetailNum ];
         if ( !mesh || mesh->getBounds().isEmpty() || mesh->mNumVerts == 0 )
            continue;

         // We need the default mesh transform.
         MatrixF localXfm;
         getNodeWorldTransform( object.nodeIndex, &localXfm );

         // We have some sort of collision shape... so allocate it.
         if ( !colShape )
            colShape = PhysicsCollision::create();

         // We have geometry... what is it?
         if ( dStrStartsWith( meshName, "Colbox" ) )
         {
            // The bounds define the box extents directly.
            Point3F halfWidth = mesh->getBounds().getExtents() * 0.5f;

            // Add the offset to the center of the bounds 
            // into the local space transform.
            MatrixF centerXfm( true );
            centerXfm.setPosition( mesh->getBounds().getCenter() );
            localXfm.mul( centerXfm );

            colShape->addBox( halfWidth, localXfm );
         }
         else if ( dStrStartsWith( meshName, "Colsphere" ) )
         {
            // Get a sphere inscribed to the bounds.
            F32 radius = mesh->getBounds().len_min() * 0.5f;

            // Add the offset to the center of the bounds 
            // into the local space transform.
            MatrixF primXfm( true );
            primXfm.setPosition( mesh->getBounds().getCenter() );
            localXfm.mul( primXfm );

            colShape->addSphere( radius, localXfm );
         }
         else if ( dStrStartsWith( meshName, "Colcapsule" ) )
         {
            // Use the smallest extent as the radius for the capsule.
            Point3F extents = mesh->getBounds().getExtents();
            F32 radius = extents.least() * 0.5f;

            // We need to center the capsule and align it to the Y axis.
            MatrixF primXfm( true );
            primXfm.setPosition( mesh->getBounds().getCenter() );

            // Use the longest axis as the capsule height.
            F32 height = -radius * 2.0f;
            if ( extents.x > extents.y && extents.x > extents.z )
            {
               primXfm.setColumn( 0, Point3F( 0, 0, 1 ) );
               primXfm.setColumn( 1, Point3F( 1, 0, 0 ) );
               primXfm.setColumn( 2, Point3F( 0, 1, 0 ) );
               height += extents.x;
            }
            else if ( extents.z > extents.x && extents.z > extents.y )
            {
               primXfm.setColumn( 0, Point3F( 0, 1, 0 ) );
               primXfm.setColumn( 1, Point3F( 0, 0, 1 ) );
               primXfm.setColumn( 2, Point3F( 1, 0, 0 ) );
               height += extents.z;
            }
            else
               height += extents.y;

            // Add the primitive transform into the local transform.
            localXfm.mul( primXfm );

            // If we didn't find a positive height then fallback to
            // creating a sphere which is better than nothing.
            if ( height > 0.0f )
               colShape->addCapsule( radius, height, localXfm );
            else
               colShape->addSphere( radius, localXfm );
         }
         else if ( dStrStartsWith( meshName, "Colmesh" ) )
         {
            // For a triangle mesh we gather the triangles raw from the
            // mesh and don't do any optimizations or cleanup.
            ConcretePolyList polyList;
            polyList.setTransform( &MatrixF::Identity, scale );
            mesh->buildPolyList( 0, &polyList, surfaceKey, NULL );
            colShape->addTriangleMesh( polyList.mVertexList.address(), 
                                       polyList.mVertexList.size(),
                                       polyList.mIndexList.address(),
                                       polyList.mIndexList.size() / 3,
                                       localXfm );
         }
         else
         {
            // Any other mesh name we assume as a generic convex hull.
            //
            // Collect the verts using the vertex polylist which will 
            // filter out duplicates.  This is importaint as the convex
            // generators can sometimes fail with duplicate verts.
            //
            VertexPolyList polyList;
            MatrixF meshMat( localXfm );

            Point3F t = meshMat.getPosition();
            t.convolve( scale );
            meshMat.setPosition( t );            

            polyList.setTransform( &MatrixF::Identity, scale );
            mesh->buildPolyList( 0, &polyList, surfaceKey, NULL );
            colShape->addConvex( polyList.getVertexList().address(), 
                                 polyList.getVertexList().size(),
                                 meshMat );
         }

         if ( perMesh )
         {
            list->increment();
            
            S32 detailNum;
            String::GetTrailingNumber( name, detailNum );            
            
            String str = String::ToString( "%s%i", meshName.c_str(), detailNum );
            S32 found = findNode( str );

            if ( found == -1 )
            {
               str = str.replace('-','_');
               found = findNode( str );
            }

            list->last().colNode = found;            
            list->last().colShape = colShape;

            colShape = NULL;
         }

      } // objects

   } // details

   return colShape;
}

static Point3F	texGenAxis[18] =
{
   Point3F(0,0,1), Point3F(1,0,0), Point3F(0,-1,0),
   Point3F(0,0,-1), Point3F(1,0,0), Point3F(0,1,0),
   Point3F(1,0,0), Point3F(0,1,0), Point3F(0,0,1),
   Point3F(-1,0,0), Point3F(0,1,0), Point3F(0,0,-1),
   Point3F(0,1,0), Point3F(1,0,0), Point3F(0,0,1),
   Point3F(0,-1,0), Point3F(-1,0,0), Point3F(0,0,-1)
};

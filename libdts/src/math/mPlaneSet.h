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

#ifndef _MPLANESET_H_
#define _MPLANESET_H_

#ifndef _MPLANE_H_
#include "math/mPlane.h"
#endif

#ifndef _MBOX_H_
#include "math/mBox.h"
#endif

#ifndef _MSPHERE_H_
#include "math/mSphere.h"
#endif

#ifndef _MORIENTEDBOX_H_
#include "math/mOrientedBox.h"
#endif

#ifndef _TEMPALLOC_H_
#include "core/tempAlloc.h"
#endif

//-----------------------------------------------------------------------------

BEGIN_NS(DTShape)

//-----------------------------------------------------------------------------

/// Set of planes which can be tested against bounding volumes.
///
/// This class is meant as a helper to collect functionality for working with sets
/// of planes.  As a helper, it does not define means to manage the data it uses.
///
/// @warn This class does not manage the memory needed for the set.
template< typename T >
class PlaneSet
{
   protected:

      /// Number of planes in #mPlanes.
      U32 mNumPlanes;

      /// Set of planes.  The memory for this array is managed outside
      /// this class.
      const T* mPlanes;

      template< typename P > OverlapTestResult _testOverlap( const P& bounds ) const;
      template< typename P > bool _isContained( const P& bounds ) const;

   public:

      /// @name Constructors
      /// @{

      /// Create an uninitialized set.
      /// @warn None of the members will be initialized.
      PlaneSet() {}
      
      /// Use the given set of planes to initialize the set.
      ///
      /// @param planes The planes.  Memory must be valid for the entire
      ///   lifetime of the plane set.
      /// @param numPlanes Number of planes.
      PlaneSet( const T* planes, U32 numPlanes )
         : mNumPlanes( numPlanes ), mPlanes( planes ) {}

      /// @}

      /// @name Accessors
      /// @{

      /// Return the number of planes that this set consists of.
      U32 getNumPlanes() const { return mNumPlanes; }

      /// Return the array of planes for this set.
      const T* getPlanes() const { return mPlanes; }

      /// @}

      /// @name Intersection
      /// All of these intersection methods are approximate in that they can produce
      /// false positives on GeometryIntersecting.  For precise results, use Intersector.
      /// @{

      /// Test intersection of the given AABB with the volume defined by the plane set.
      ///
      /// @param aabb Axis-aligned bounding box.
      /// @return The type of overlap that the given AABB has with the plane set.
      ///
      /// @note The method will not test for those edge cases where none of the planes
      ///      rejects the given AABB yet the AABB is actually outside the volume defined by the planes.
      ///      This speeds up the test at the cost of losing precision which is acceptable for
      ///      cases where doing the additional tests for all intersecting objects will generally
      ///      waste more time than accepting a few additional non-intersecting objects as
      ///      intersecting.
      OverlapTestResult testPotentialIntersection( const Box3F& aabb ) const
      {
         return _testOverlap( aabb );
      }

      /// Test intersection of the given sphere with the volume defined by the plane set.
      ///
      /// @param sphere Sphere.
      /// @return The type of overlap that the given sphere has with the plane set.
      ///
      /// @note The method will not test for those edge cases where none of the planes
      ///      rejects the given sphere yet the sphere is actually outside the volume defined by the planes.
      ///      This speeds up the test at the cost of losing precision which is acceptable for
      ///      cases where doing the additional tests for all intersecting objects will generally
      ///      waste more time than accepting a few additional non-intersecting objects as
      ///      intersecting.
      OverlapTestResult testPotentialIntersection( const SphereF& sphere ) const
      {
         return _testOverlap( sphere );
      }

      /// Test intersection of the given OBB with the volume defined by the plane set.
      ///
      /// @param obb Oriented bounding box.
      /// @return The type of overlap that the given OBB has with the plane set.
      ///
      /// @note The method will not test for those edge cases where none of the planes
      ///      rejects the given OBB yet the OBB is actually outside the volume defined by the planes.
      ///      This speeds up the test at the cost of losing precision which is acceptable for
      ///      cases where doing the additional tests for all intersecting objects will generally
      ///      waste more time than accepting a few additional non-intersecting objects as
      ///      intersecting.
      OverlapTestResult testPotentialIntersection( const OrientedBox3F& obb ) const
      {
         return _testOverlap( obb );
      }

      /// Returns a bitmask of which planes are hit by the given box.
      U32 testPlanes( const Box3F& bounds, U32 planeMask = 0xFFFFFFFF, F32 expand = 0.0f ) const;

      /// @}

      /// @name Containment
      /// Testing for containment of geometric shapes within the volume enclosed by the planes.
      /// @{

      /// Return true if the given point lies on the positive side of all the planes
      /// in the set.
      bool isContained( const Point3F& point, F32 epsilon = __EQUAL_CONST_F ) const;

      /// Return true if all of the given points lie on the positive side of all the planes
      /// in the set.
      bool isContained( const Point3F* points, U32 numPoints ) const;

      /// Return true if the given AABB lies on the positive side of all the planes
      /// in the set.
      bool isContained( const Box3F& aabb ) const { return _isContained( aabb ); }

      /// Return true if the given sphere lies on the positive side of all the planes
      /// in the set.
      bool isContained( const SphereF& sphere ) const { return _isContained( sphere ); }

      /// Return true if the given OBB lies on the positive side of all the planes
      /// in the set.
      bool isContained( const OrientedBox3F& obb ) const { return _isContained( obb ); }

      /// @}

      /// @name Clipping
      /// @{

      /// Clip the given line segment against the plane set.  If the segment
      /// intersects with any of the planes, the points will be clipped at the
      /// intersection.
      ///
      /// @return True if any part of the segment is inside the volume defined by the plane set.
      bool clipSegment( Point3F& pnt0, Point3F& pnt1 ) const;

      /// Clip a convex polygon by all planes in the set.
      ///
      /// @param inVertices Array holding the vertices of the input polygon.
      /// @param inNumVertices Number of vertices in @a inVertices.
      /// @param outVertices Array to hold the vertices of the clipped polygon.  Must have spaces for
      ///   @a inNumVertices + numberOfPlanesInSet.  Must not be the same as @a inVertices.
      /// @param maxOutVertices Maximum number of vertices that can be stored in @a outVertices.  If insufficient to
      ///   store the clipped polygon, the return value will be 0.
      ///
      /// @return Number of vertices in the clipped polygon, i.e. number of vertices stored in @a outVertices.
      ///
      /// @note Be aware that if the polygon fully lies on the negative side of all planes,
      ///   the resulting @a outNumVertices will be zero, i.e. no polygon will result from the clip.
      U32 clipPolygon( const Point3F* inVertices, U32 inNumVertices, Point3F* outVertices, U32 maxOutVertices ) const;

      /// @}
};

typedef PlaneSet< PlaneF > PlaneSetF;
typedef PlaneSet< PlaneD > PlaneSetD;


//-----------------------------------------------------------------------------

template< typename T >
template< typename P >
inline OverlapTestResult PlaneSet< T >::_testOverlap( const P& bounds ) const
{
   bool allInside = true;

   // First, find out whether there is any plane for which the bounds completely
   // lie on the negative side.  If so, the bounds are clearly outside of the volume.
   
   const U32 numPlanes = getNumPlanes();
   for( U32 nplane = 0; nplane < numPlanes; ++ nplane )
   {
      const PlaneF& plane = getPlanes()[ nplane ];

      const PlaneF::Side side = plane.whichSide( bounds );
      if( side == PlaneF::Back )
         return GeometryOutside;

      if( side != PlaneF::Front )
         allInside = false;
   }

   // Test for containment.

   if( allInside )
      return GeometryInside;

   // Otherwise classify as intersecting.

   return GeometryIntersecting;
}

//-----------------------------------------------------------------------------

template< typename T >
inline bool PlaneSet< T >::isContained( const Point3F& point, F32 epsilon ) const
{
   epsilon = - epsilon;

   for( U32 i = 0; i < mNumPlanes; ++ i )
   {
      const F32 dist = mPlanes[ i ].distToPlane( point );
      if( dist < epsilon )
         return false;
   }

   return true;
}

//-----------------------------------------------------------------------------

template< typename T >
inline bool PlaneSet< T >::isContained( const Point3F* points, U32 numPoints ) const
{
   for( U32 i = 0; i < numPoints; ++ i )
      if( !isContained( points[ i ] ) )
         return false;

   return true;
}

//-----------------------------------------------------------------------------

template< typename T >
template< typename P >
inline bool PlaneSet< T >::_isContained( const P& bounds ) const
{
   for( U32 i = 0; i < mNumPlanes; ++ i )
      if( mPlanes[ i ].whichSide( bounds ) != PlaneF::Front )
         return false;

   return true;
}

//-----------------------------------------------------------------------------

template< typename T >
U32 PlaneSet< T >::testPlanes( const Box3F& bounds, U32 planeMask, F32 expand ) const
{
   AssertFatal( mNumPlanes <= 32, "PlaneSet::testPlanes - Too many planes in set!" );

   // This is based on the paper "A Faster Overlap Test for a Plane and a Bounding Box" 
   // by Kenny Hoff.  See http://www.cs.unc.edu/~hoff/research/vfculler/boxplane.html

   U32 retMask = 0;

   const U32 numPlanes = getMin( mNumPlanes, U32( 32 ) );
   for ( S32 i = 0; i < numPlanes; i++ )
   {
      U32 mask = ( 1 << i );

      if ( !( planeMask & mask ) )
         continue;

      const PlaneF& plane = mPlanes[ i ];

      Point3F minPoint, maxPoint;

      if( plane.x > 0 )
      {
         maxPoint.x = bounds.maxExtents.x;
         minPoint.x = bounds.minExtents.x;
      }
      else
      {
         maxPoint.x = bounds.minExtents.x;
         minPoint.x = bounds.maxExtents.x;
      }

      if( plane.y > 0 )
      {
         maxPoint.y = bounds.maxExtents.y;
         minPoint.y = bounds.minExtents.y;
      }
      else
      {
         maxPoint.y = bounds.minExtents.y;
         minPoint.y = bounds.maxExtents.y;
      }

      if( plane.z > 0 )
      {
         maxPoint.z = bounds.maxExtents.z;
         minPoint.z = bounds.minExtents.z;
      }
      else
      {
         maxPoint.z = bounds.minExtents.z;
         minPoint.z = bounds.maxExtents.z;
      }

      F32 maxDot = mDot( maxPoint, plane );

      if( maxDot <= - ( plane.d + expand ) )
         return -1;

      F32 minDot = mDot( minPoint, plane );

      if( ( minDot + plane.d ) < 0.0f )
         retMask |= mask;
   }

   return retMask;
}

//-----------------------------------------------------------------------------

END_NS

#endif // !_MPLANESET_H_

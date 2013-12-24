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
#include "math/mPlane.h"

#include "math/mathUtils.h"
#include "math/mBox.h"
#include "math/mOrientedBox.h"
#include "math/mSphere.h"
#include "math/mPlaneSet.h"

//-----------------------------------------------------------------------------

BEGIN_NS(DTShape)

//-----------------------------------------------------------------------------

bool PlaneF::intersect( const PlaneF& plane, Point3F& outLinePt, VectorF& outLineDir ) const
{
   // Compute direction of intersection line.
   outLineDir = mCross( *this, plane );

   // If d is zero, the planes are parallel (and separated)
   // or coincident, so they're not considered intersecting
   F32 denom = mDot( outLineDir, outLineDir );
   if ( denom < 0.00001f ) 
      return false;

   // Compute point on intersection line
   outLinePt = - mCross( d * plane - plane.d * *this,
                         outLineDir ) / denom;

   return true;
}

//-----------------------------------------------------------------------------

bool PlaneF::isParallelTo( const PlaneF& plane, F32 epsilon ) const
{
   F32 val = 1.0f - mFabs( mDot( *this, plane ) );
   return ( val > - epsilon ) && ( val < epsilon );
}

//-----------------------------------------------------------------------------

bool PlaneF::isPerpendicularTo( const PlaneF& plane, F32 epsilon ) const
{
   F32 val = mDot( *this, plane );
   return ( val > - epsilon) && ( val < epsilon );   
}

//-----------------------------------------------------------------------------

bool PlaneF::clipSegment( const Point3F& start, const Point3F& end, Point3F& outNewEnd ) const
{
   // Intersect ray with plane.

   F32 dist = intersect( start, end );
   if( dist == PARALLEL_PLANE || dist < 0.f || dist > 1.f )
      return false;

   // Compute distance to point on segment.

   Point3F dir = end - start;
   dir *= dist;

   // Compute new end point.

   outNewEnd = start + dir;

   return true;
}

//-----------------------------------------------------------------------------

U32 PlaneF::clipPolygon( const Point3F* inVertices, U32 inNumVertices, Point3F* outVertices ) const
{
   // Find the first vertex that lies on the front of the plane.

   S32 start = -1;
   for( U32 i = 0; i < inNumVertices; i ++ )
   {
      Side side = whichSide( inVertices[ i ] );
      if( side == PlaneF::Front )
      {
         start = i;
         break;
      }
   }

   // If nothing was in front of the plane, we're done.

   if( start == -1 )
      return 0;

   Point3F finalPoints[ 128 ];
   U32  numFinalPoints = 0;

   U32 baseStart = start;
   U32 end       = ( start + 1 ) % inNumVertices;

   dMemcpy( outVertices, inVertices, inNumVertices * sizeof( Point3F ) );

   while( end != baseStart )
   {
      const Point3F& rStartPoint = outVertices[ start ];
      const Point3F& rEndPoint   = outVertices[ end ];

      PlaneF::Side fSide = whichSide( rStartPoint );
      PlaneF::Side eSide = whichSide( rEndPoint );

      S32 code = fSide * 3 + eSide;
      switch( code )
      {
         case 4:   // f f
         case 3:   // f o
         case 1:   // o f
         case 0:   // o o
            // No Clipping required
            finalPoints[ numFinalPoints ++ ] = outVertices[ start ];
            start = end;
            end   = ( end + 1 ) % inNumVertices;
            break;


         case 2:   // f b
         {
            // In this case, we emit the front point, Insert the intersection,
            //  and advancing to point to first point that is in front or on...
            //
            finalPoints[ numFinalPoints ++ ] = outVertices[ start ];

            Point3F vector = rEndPoint - rStartPoint;
            F32 t        = - ( distToPlane( rStartPoint ) / mDot( *this, vector ) );

            Point3F intersection = rStartPoint + ( vector * t );
            finalPoints[ numFinalPoints ++ ] = intersection;

            U32 endSeek = ( end + 1 ) % inNumVertices;
            while( whichSide( outVertices[ endSeek ] ) == PlaneF::Back )
               endSeek = ( endSeek + 1 ) % inNumVertices;

            end   = endSeek;
            start = ( end + ( inNumVertices - 1 ) ) % inNumVertices;

            const Point3F& rNewStartPoint = outVertices[ start ];
            const Point3F& rNewEndPoint   = outVertices[ end ];

            vector = rNewEndPoint - rNewStartPoint;
            t = - ( distToPlane( rNewStartPoint ) / mDot( *this, vector ) );

            intersection = rNewStartPoint + ( vector * t );
            outVertices[ start ] = intersection;
         }
         break;

         case -1:  // o b
         {
            // In this case, we emit the front point, and advance to point to first
            //  point that is in front or on...
            //
            finalPoints[ numFinalPoints ++ ] = outVertices[ start ];

            U32 endSeek = ( end + 1 ) % inNumVertices;
            while( whichSide( outVertices[ endSeek ] ) == PlaneF::Back)
               endSeek = ( endSeek + 1 ) % inNumVertices;

            end   = endSeek;
            start = (end + ( inNumVertices - 1 ) ) % inNumVertices;

            const Point3F& rNewStartPoint = outVertices[ start ];
            const Point3F& rNewEndPoint   = outVertices[ end ];

            Point3F vector = rNewEndPoint - rNewStartPoint;
            F32 t        = - ( distToPlane( rNewStartPoint ) / mDot( *this, vector ) );

            Point3F intersection = rNewStartPoint + ( vector * t );
            outVertices[ start ] = intersection;
         }
         break;

        case -2:  // b f
        case -3:  // b o
        case -4:  // b b
           // In the algorithm used here, this should never happen...
           AssertISV(false, "SGUtil::clipToPlane: error in polygon clipper");
           break;

        default:
           AssertFatal(false, "SGUtil::clipToPlane: bad outcode");
           break;
      }

   }

   // Emit the last point.
   finalPoints[ numFinalPoints ++ ] = outVertices[ start ];
   AssertFatal( numFinalPoints >= 3, avar("Error, this shouldn't happen!  Invalid winding in clipToPlane: %d", numFinalPoints ) );

   // Copy the new rWinding, and we're set!
   //
   dMemcpy( outVertices, finalPoints, numFinalPoints * sizeof( Point3F ) );
   AssertISV( numFinalPoints <= 128, "MaxWindingPoints exceeded in scenegraph.  Fatal error.");

   return numFinalPoints;
}



//-----------------------------------------------------------------------------

template< typename T >
bool PlaneSet< T >::clipSegment( Point3F &pnt0, Point3F &pnt1 ) const
{
   F32 tmin = F32_MAX;
   F32 tmax = -F32_MAX;
   U32 hitCount = 0;
   Point3F tpnt;
   
   const U32 numPlanes = mNumPlanes;
   for( U32 i = 0; i < numPlanes; ++ i )
   {
      const PlaneF &plane = mPlanes[ i ];
      
      F32 t = plane.intersect( pnt0, pnt1 );
      
      if( t >= 0.0f && t <= 1.0f )
      {
         tpnt.interpolate( pnt0, pnt1, t );
         
         if ( isContained( tpnt, 1.0e-004f ) )
         {
            tmin = getMin( tmin, t );
            tmax = getMax( tmax, t );
            hitCount ++;
         }
      }
   }
   
   // If we had no intersections then either both points are inside or both are outside.
   
   if( hitCount == 0 )
      return isContained( pnt0 );
   
   // If we had one intersection then we have one point inside.
   // tmin and tmax are the same here.
   if( hitCount == 1 )
   {
      if( isContained( pnt0 ) )
         pnt1.interpolate( pnt0, pnt1, tmax );
      else
         pnt0.interpolate( pnt0, pnt1, tmin );
   }
   else
   {
      Point3F prevPnt0( pnt0 );
      Point3F prevPnt1( pnt1 );
      
      if( tmin < F32_MAX )
         pnt0.interpolate( prevPnt0, prevPnt1, tmin );
      if( tmax > -F32_MAX )
         pnt1.interpolate( prevPnt0, prevPnt1, tmax );
   }
   
   return true;
}

//-----------------------------------------------------------------------------

template< typename T >
U32 PlaneSet< T >::clipPolygon( const Point3F* inVertices, U32 inNumVertices, Point3F* outVertices, U32 maxOutVertices ) const
{
   TempAlloc< Point3F > tempBuffer( inNumVertices + mNumPlanes );
   
   // We use two buffers as interchanging roles as source and target.
   // For the first iteration, inVertices is the source.
   
   Point3F* tempPolygon = tempBuffer;
   Point3F* clippedPolygon = const_cast< Point3F* >( inVertices );
   
   U32 numClippedPolygonVertices = inNumVertices;
   U32 numTempPolygonVertices = 0;
   
   for( U32 nplane = 0; nplane < mNumPlanes; ++ nplane )
   {
      // Make the output of the last iteration the
      // input of this iteration.
      
      Point3F *prevPolygon = clippedPolygon;
      clippedPolygon = tempPolygon;
      tempPolygon = prevPolygon;
      numTempPolygonVertices = numClippedPolygonVertices;
      
      if( maxOutVertices < numTempPolygonVertices + 1 )
         return 0;
      
      // Clip our current remainder of the original polygon
      // against the current plane.
      
      const PlaneF& plane = mPlanes[ nplane ];
      numClippedPolygonVertices = plane.clipPolygon( tempPolygon, numTempPolygonVertices, clippedPolygon );
      
      // If the polygon was completely on the backside of the plane,
      // then polygon is outside the frustum.  In this case, return false
      // to indicate we haven't clipped anything.
      
      if( !numClippedPolygonVertices )
         return false;
      
      // On first iteration, replace the inVertices with the
      // outVertices buffer.
      
      if( tempPolygon == inVertices )
         tempPolygon = outVertices;
   }
   
   // If outVertices isn't the target buffer of the last
   // iteration, copy the vertices over from the temporary
   // buffer.
   
   if( clippedPolygon != outVertices )
      dMemcpy( outVertices, clippedPolygon, numClippedPolygonVertices * sizeof( Point3F ) );
   
   return numClippedPolygonVertices;
}

//-----------------------------------------------------------------------------

END_NS


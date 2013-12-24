//
//  tsRender.cpp
//  libdts
//
//  Created by James Urquhart on 08/12/2012.
//  Copyright (c) 2012 James Urquhart. All rights reserved.
//

#include "platform/platform.h"
#include "tsRender.h"

#include "platform/profiler.h"
#include "core/util/hashFunction.h"

//-----------------------------------------------------------------------------

BEGIN_NS(DTShape)

//-----------------------------------------------------------------------------


Swizzle<U8, 4> *TSVertexColor::mDeviceSwizzle = NULL;

U32 GFXVertexElement::getSizeInBytes() const
{
   switch ( mType )
   {
      case GFXDeclType_Float:
         return 4;
         
      case GFXDeclType_Float2:
         return 8;
         
      case GFXDeclType_Float3:
         return 12;
         
      case GFXDeclType_Float4:
         return 16;
         
      case GFXDeclType_Color:
         return 4;
         
      default:
         return 0;
   };
}


GFXVertexFormat::GFXVertexFormat()
:  mDirty( true ),
mHasColor( false ),
mHasNormal( false ),
mHasTangent( false ),
mTexCoordCount( 0 ),
mSizeInBytes( 0 ),
mDecl( NULL )
{
   VECTOR_SET_ASSOCIATION( mElements );
}

void GFXVertexFormat::copy( const GFXVertexFormat &format )
{
   mDirty = format.mDirty;
   mHasNormal = format.mHasNormal;
   mHasTangent = format.mHasTangent;
   mHasColor = format.mHasColor;
   mTexCoordCount = format.mTexCoordCount;
   mSizeInBytes = format.mSizeInBytes;
   mElements = format.mElements;
   mDecl = format.mDecl;
}

void GFXVertexFormat::append( const GFXVertexFormat &format, U32 streamIndex )
{
   for ( U32 i=0; i < format.getElementCount(); i++ )
   {
      mElements.increment();
      mElements.last() = format.getElement( i );
      if ( streamIndex != -1 )
         mElements.last().mStreamIndex = streamIndex;
   }
   
   mDirty = true;
}

void GFXVertexFormat::clear()
{
   mDirty = true;
   mElements.clear();
   mDecl = NULL;
}

void GFXVertexFormat::addElement( const GFXSemantic::GFXSemantic semantic, GFXDeclType type, U32 index, U32 stream )
{
   mDirty = true;
   mElements.increment();
   mElements.last().mStreamIndex = stream;
   mElements.last().mSemantic = semantic;
   mElements.last().mSemanticIndex = index;
   mElements.last().mType = type;
}

GFXVertexDecl* GFXVertexFormat::getDecl() const
{
   if ( !mDecl || mDirty )
      const_cast<GFXVertexFormat*>(this)->_updateDecl();
   
   return mDecl;
}

bool GFXVertexFormat::hasNormal() const
{
   if ( mDirty )
      const_cast<GFXVertexFormat*>(this)->_updateDirty();
   
   return mHasNormal;
}

bool GFXVertexFormat::hasTangent() const
{
   if ( mDirty )
      const_cast<GFXVertexFormat*>(this)->_updateDirty();
   
   return mHasTangent;
}

bool GFXVertexFormat::hasColor() const
{
   if ( mDirty )
      const_cast<GFXVertexFormat*>(this)->_updateDirty();
   
   return mHasColor;
}

U32 GFXVertexFormat::getTexCoordCount() const
{
   if ( mDirty )
      const_cast<GFXVertexFormat*>(this)->_updateDirty();
   
   return mTexCoordCount;
}

U32 GFXVertexFormat::getSizeInBytes() const
{
   if ( mDirty )
      const_cast<GFXVertexFormat*>(this)->_updateDirty();
   
   return mSizeInBytes;
}

void GFXVertexFormat::_updateDirty()
{
   PROFILE_SCOPE( GFXVertexFormat_updateDirty );
   
   mTexCoordCount = 0;
   
   mHasColor = false;
   mHasNormal = false;
   mHasTangent = false;
   mSizeInBytes = 0;
   
   for ( U32 i=0; i < mElements.size(); i++ )
   {
      const GFXVertexElement &element = mElements[i];
      
      if ( element.isSemantic( GFXSemantic::NORMAL ) )
         mHasNormal = true;
      else if ( element.isSemantic( GFXSemantic::TANGENT ) )
         mHasTangent = true;
      else if ( element.isSemantic( GFXSemantic::COLOR ) )
         mHasColor = true;
      else if ( element.isSemantic( GFXSemantic::TEXCOORD ) )
         ++mTexCoordCount;
      
      mSizeInBytes += element.getSizeInBytes();
   }
   
   mDirty = false;
}

void GFXVertexFormat::_updateDecl()
{
   PROFILE_SCOPE( GFXVertexFormat_updateDecl );
   
   if ( mDirty )
      _updateDirty();
   
   static GFXVertexDecl decl;
   mDecl = &decl;
}

//-----------------------------------------------------------------------------

END_NS

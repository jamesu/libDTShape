//
//  tsRender.h
//  libdts
//
//  Created by James Urquhart on 08/12/2012.
//  Copyright (c) 2012 James Urquhart. All rights reserved.
//

#ifndef _TSRENDER_H_
#define _TSRENDER_H_

#include "core/util/swizzle.h"
#include "core/util/refBase.h"
#include "core/color.h"
#include "core/util/tVector.h"

//-----------------------------------------------------------------------------

BEGIN_NS(DTShape)

//-----------------------------------------------------------------------------

class GFXDevice;

class TSVertexColor
{
private:
   U32 packedColorData;
   
public:
   static Swizzle<U8, 4> *mDeviceSwizzle;
   
public:
   static void setSwizzle( Swizzle<U8, 4> *val ) { mDeviceSwizzle = val; }
   
   TSVertexColor() : packedColorData( 0xFFFFFFFF ) {} // White with full alpha
   TSVertexColor( const ColorI &color ) { set( color ); }
   
   void set( U8 red, U8 green, U8 blue, U8 alpha = 255 )
   {
      packedColorData = red << 0 | green << 8 | blue << 16 | alpha << 24;
      mDeviceSwizzle->InPlace( &packedColorData, sizeof( packedColorData ) );
   }
   
   void set( const ColorI &color )
   {
      mDeviceSwizzle->ToBuffer( &packedColorData, (U8 *)&color, sizeof( packedColorData ) );
   }
   
   TSVertexColor &operator=( const ColorI &color ) { set( color ); return *this; }
   operator const U32 *() const { return &packedColorData; }
   const U32& getPackedColorData() const { return packedColorData; }
   
   void getColor( ColorI *color ) const
   {
      mDeviceSwizzle->ToBuffer( color, &packedColorData, sizeof( packedColorData ) );
   }      
};

/// Defines a vertex declaration type.
/// @see GFXVertexElement
/// @see GFXVertexFormat
enum GFXDeclType
{
   GFXDeclType_FIRST = 0,
   
   /// A single component F32.
   GFXDeclType_Float = 0,
   
   /// A two-component F32.
   /// @see Point2F
   GFXDeclType_Float2,
   
   /// A three-component F32.
   /// @see Point3F
   GFXDeclType_Float3,
   
   /// A four-component F32.
   /// @see Point4F
   GFXDeclType_Float4,
   
   /// A four-component, packed, unsigned bytes mapped to 0 to 1 range.
   /// @see GFXVertexColor
   GFXDeclType_Color,
   
   /// The count of total GFXDeclTypes.
   GFXDeclType_COUNT,
};

//-----------------------------------------------------------------------------

enum GFXPrimitiveType
{
   GFXPT_FIRST = 0,
   GFXPointList = 0,
   GFXLineList,
   GFXLineStrip,
   GFXTriangleList,
   GFXTriangleStrip,
   GFXTriangleFan,
   GFXPT_COUNT
};


/// The known Torque vertex element semantics.  You can use
/// other semantic strings, but they will be interpreted as
/// a TEXCOORD.
/// @see GFXVertexElement
/// @see GFXVertexFormat
namespace GFXSemantic
{
   enum GFXSemantic
   {
      POSITION = 0,
      NORMAL = 1,
      BINORMAL = 2,
      TANGENT = 3,
      TANGENTW = 4,
      COLOR = 5,
      TEXCOORD = 6
   };
}


/// This is a simple wrapper for the platform specific
/// vertex declaration data which is held by the vertex
/// format.
///
/// If your using it... you probably shouldn't be.
///
/// @see GFXVertexFormat
class GFXVertexDecl
{
public:
   virtual ~GFXVertexDecl() {}
};


/// The element structure helps define the data layout
/// for GFXVertexFormat.
///
/// @see GFXVertexFormat
///
class GFXVertexElement
{
   friend class GFXVertexFormat;
   
protected:
   
   /// The stream index when rendering from multiple
   /// vertex streams.  In most cases this is 0.
   U32 mStreamIndex;
   
   /// A valid Torque shader symantic.
   /// @see GFXSemantic
   GFXSemantic::GFXSemantic mSemantic;
   
   /// The semantic index is used where there are
   /// multiple semantics of the same type.  For
   /// instance with texcoords.
   U32 mSemanticIndex;
   
   /// The element type.
   GFXDeclType mType;
   
public:
   
   /// Default constructor.
   GFXVertexElement()
   :  mStreamIndex( 0 ),
   mSemanticIndex( 0 ),
   mType( GFXDeclType_Float4 )
   {
   }
   
   /// Copy constructor.
   GFXVertexElement( const GFXVertexElement &elem )
   :  mStreamIndex( elem.mStreamIndex ),
   mSemantic( elem.mSemantic ),
   mSemanticIndex( elem.mSemanticIndex ),
   mType( elem.mType )
   {
   }
   
   /// Returns the stream index.
   U32 getStreamIndex() const { return mStreamIndex; }
   
   /// Returns the semantic name which is usually a
   /// valid Torque semantic.
   /// @see GFXSemantic
   const GFXSemantic::GFXSemantic& getSemantic() const { return mSemantic; }
   
   /// Returns the semantic index which is used where there
   /// are multiple semantics of the same type.  For instance
   /// with texcoords.
   U32 getSemanticIndex() const { return mSemanticIndex; }
   
   /// Returns the type for the semantic.
   GFXDeclType getType() const { return mType; }
   
   /// Returns true of the semantic matches.
   bool isSemantic( const GFXSemantic::GFXSemantic sm ) const { return ( mSemantic == sm ); }
   
   /// Returns the size in bytes of the semantic type.
   U32 getSizeInBytes() const;
};


/// The vertex format structure usually created via the declare and
/// implement macros.
///
/// You can use this class directly to create a vertex format, but
/// note that it is expected to live as long as the VB that uses it
/// exists.
///
/// @see GFXDeclareVertexFormat
/// @see GFXImplementVertexFormat
/// @see GFXVertexElement
///
class GFXVertexFormat
{
public:
   
   /// Default constructor for an empty format.
   GFXVertexFormat();
   
   /// The copy constructor.
   GFXVertexFormat( const GFXVertexFormat &format ) { copy( format ); }
   
   /// Copy the other vertex format.
   void copy( const GFXVertexFormat &format );
   
   /// Used to append a vertex format to the end of this one.
   void append( const GFXVertexFormat &format, U32 streamIndex = -1 );
   
   /// Clears all the vertex elements.
   void clear();
   
   /// Adds a vertex element to the format.
   ///
   /// @param semantic A valid Torque semantic string.
   /// @param type The element type.
   /// @param index The semantic index which is typically only used for texcoords.
   ///
   void addElement( const GFXSemantic::GFXSemantic semantic, GFXDeclType type, U32 index = 0, U32 stream = 0 );
   
   /// Returns true if there is a NORMAL semantic in this vertex format.
   bool hasNormal() const;
   
   /// Returns true if there is a TANGENT semantic in this vertex format.
   bool hasTangent() const;
   
   /// Returns true if there is a COLOR semantic in this vertex format.
   bool hasColor() const;
   
   /// Returns the texture coordinate count by
   /// counting the number of TEXCOORD semantics.
   U32 getTexCoordCount() const;
   
   /// Returns true if these two formats are equal.
   inline bool isEqual( const GFXVertexFormat &format ) const;
   
   /// Returns the total elements in this format.
   U32 getElementCount() const { return mElements.size(); }
   
   /// Returns the vertex element by index.
   const GFXVertexElement& getElement( U32 index ) const { return mElements[index]; }
   
   /// Returns the size in bytes of the format as described.
   U32 getSizeInBytes() const;
   
   /// Returns the hardware specific vertex declaration for this format.
   GFXVertexDecl* getDecl() const;
   
protected:
   
   /// We disable the copy operator.
   GFXVertexFormat& operator =( const GFXVertexFormat& ) { return *this; }
   
   /// Recreates the description and state when
   /// the format has been modified.
   void _updateDirty();
   
   /// Requests the vertex declaration from the GFX device.
   void _updateDecl();
   
   /// Set when the element list is changed.
   bool mDirty;
   
   /// Is set if there is a NORMAL semantic in this vertex format.
   bool mHasNormal;
   
   /// Is true if there is a TANGENT semantic in this vertex format.
   bool mHasTangent;
   
   /// Is true if there is a COLOR semantic in this vertex format.
   bool mHasColor;
   
   /// The texture coordinate count by counting the
   /// number of "TEXCOORD" semantics.
   U32 mTexCoordCount;
   
   /// The size in bytes of the vertex format as described.
   U32 mSizeInBytes;
   
   /// The elements of the vertex format.
   Vector<GFXVertexElement> mElements;
   
   /// The hardware specific vertex declaration.
   GFXVertexDecl *mDecl;
};


inline bool GFXVertexFormat::isEqual( const GFXVertexFormat &format ) const
{
   // Comparing the strings works because we know both
   // these are interned strings.  This saves one comparison
   // over the string equality operator.
   
   return mElements.size() == format.mElements.size() &&
   dMemcmp(mElements.begin(), format.mElements.begin(), format.mElements.size() * sizeof(GFXVertexElement)) == 0;
}

//-----------------------------------------------------------------------------

END_NS

#endif // _TSRENDER_H_

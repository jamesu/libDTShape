//
//  tsDummyRender.cpp
//  libdts
//
//  Created by James Urquhart on 13/12/2012.
//  Copyright (c) 2012 James Urquhart. All rights reserved.
//

// Dummy implementation for rendering code

#include "platform/platform.h"
#include "ts/tsMaterialManager.h"
#include "core/util/tVector.h"
#include "ts/physicsCollision.h"
#include "ts/tsShape.h"
#include "core/stream/fileStream.h"
#include "ts/tsRenderState.h"
#include "ts/tsRender.h"

//-----------------------------------------------------------------------------

BEGIN_NS(DTShape)

//-----------------------------------------------------------------------------

#ifdef LIBDTSHAPE_DUMMY_RENDER

class TSDummyMaterial;

class TSDummyMaterialInstance : public TSMaterialInstance
{
public:
   TSDummyMaterial *mMaterial;
   
   TSDummyMaterialInstance(TSDummyMaterial *mat);
   
   ~TSDummyMaterialInstance();
   
   
   virtual bool init(const GFXVertexFormat *fmt=NULL);
   virtual TSMaterial *getMaterial();
   
   virtual bool isTranslucent();
   
   virtual bool isValid();
   
   virtual int getStateHint();
   
   virtual const char* getName();
};

class TSDummyMaterial : public TSMaterial
{
public:
   String mMapTo;
   String mName;
   
   TSDummyMaterial()
   {
      
   }
   
   ~TSDummyMaterial()
   {
   }
   
   // Create an instance of this material
   virtual TSMaterialInstance *createMatInstance(const GFXVertexFormat *fmt=NULL)
   {
      TSDummyMaterialInstance *inst = new TSDummyMaterialInstance(this);
      return inst;
   }
   
   // Assign properties from collada material
   virtual bool initFromColladaMaterial(const ColladaAppMaterial *mat)
   {
      return true;
   }
   
   // Get base material definition
   virtual TSMaterial *getMaterial()
   {
      return this;
   }
   
   virtual bool isTranslucent()
   {
      return false;
   }
   
   virtual const char* getName()
   {
      return mName;
   }
};


TSDummyMaterialInstance::TSDummyMaterialInstance(TSDummyMaterial *mat) :
mMaterial(mat)
{
   
}

TSDummyMaterialInstance::~TSDummyMaterialInstance()
{
   
}


bool TSDummyMaterialInstance::init(const GFXVertexFormat *fmt)
{
   // ...
   return true;
}

TSMaterial *TSDummyMaterialInstance::getMaterial()
{
   return mMaterial;
}

bool TSDummyMaterialInstance::isTranslucent()
{
   return false;
}

bool TSDummyMaterialInstance::isValid()
{
   return true;
}

int TSDummyMaterialInstance::getStateHint()
{
   return (int)this;
}

const char* TSDummyMaterialInstance::getName()
{
   return mMaterial->getName();
}

class TSDummyMaterialManager : public TSMaterialManager
{
public:
   TSDummyMaterial *mDummyMaterial;
   Vector<TSDummyMaterial*> mMaterials;
   //Vector<TSDummyMaterialInstance*> mMaterialInstances;
   
   TSDummyMaterialManager()
   {
      mDummyMaterial = NULL;
   }
   
   ~TSDummyMaterialManager()
   {
      for (int i=0; i<mMaterials.size(); i++)
         delete mMaterials[i];
      mMaterials.clear();
   }
   
   TSMaterial *allocateAndRegister(const String &objectName, const String &mapToName = String())
   {
      TSDummyMaterial *mat = new TSDummyMaterial();
      mat->mName = objectName;
      mat->mMapTo = mapToName;
      mMaterials.push_back(mat);
      return mat;
   }
   
   // Grab an existing material
   virtual TSMaterial * getMaterialDefinitionByName(const String &matName)
   {
      Vector<TSDummyMaterial*>::iterator end = mMaterials.end();
      for (Vector<TSDummyMaterial*>::iterator itr = mMaterials.begin(); itr != end; itr++)
      {
         if ((*itr)->mName == matName)
            return *itr;
      }
      return NULL;
   }
   
   // Return instance of named material caller is responsible for memory
   virtual TSMaterialInstance * createMatInstance( const String &matName, const GFXVertexFormat *vertexFormat = NULL )
   {
      TSMaterial *mat = getMaterialDefinitionByName(matName);
      if (mat)
      {
         return mat->createMatInstance(vertexFormat);
      }
      
      return NULL;
   }
   
   virtual TSMaterialInstance * createFallbackMatInstance( const GFXVertexFormat *vertexFormat = NULL )
   {
      if (!mDummyMaterial)
      {
         mDummyMaterial = (TSDummyMaterial*)allocateAndRegister("Dummy", "Dummy");
      }
      return mDummyMaterial->createMatInstance(vertexFormat);
   }

};

// Generic interface which stores vertex buffers and such for TSMeshes
class DummyTSMeshRenderer : public TSMeshRenderer
{
public:
   DummyTSMeshRenderer() {;}
   virtual ~DummyTSMeshRenderer() {;}
   
   /// Prepares vertex buffers and whatnot
   virtual void prepare(TSMesh *mesh)
   {
      Platform::outputDebugString("[TSM:%x]Preparing", this);
   }
   
   /// Renders whatever needs to be drawn
   virtual void doRender(TSMesh *mesh, TSRenderInst *inst)
   {
      Platform::outputDebugString("[TSM:%x]Rendering with material %s", this, inst->matInst ? inst->matInst->getName() : "NIL");
   }
   
   /// Renders whatever needs to be drawn
   virtual bool isDirty(TSMesh *mesh)
   {
      return true;
   }
   
   /// Cleans up Mesh renderer
   virtual void clear()
   {
      Platform::outputDebugString("[TSM:%x]Clearing", this);
   }
};


static TSDummyMaterialManager sMaterialManager;

TSMaterialManager *TSMaterialManager::instance()
{
   return &sMaterialManager;
}

TSMeshRenderer *TSMeshRenderer::create()
{
   return new DummyTSMeshRenderer();
}

#endif


TSShape *TSShape::loadShape(const String& filename)
{
   FileStream fs;
   if (fs.open(filename, FileStream::Read))
   {
      TSShape *shape = new TSShape();
      if (shape->read(&fs))
      {
         shape->mPath = filename;
         return shape;
      }
      delete shape;
   }
   return NULL;
}




class DummyPhysicsCollision : public PhysicsCollision
{
public:
   DummyPhysicsCollision(){;}
   ~DummyPhysicsCollision(){;}
   
   /// Add an infinite plane to the collision shape.
   ///
   /// This shape is assumed to be static in some physics
   /// providers and will at times be faked with a large box.
   ///
   virtual void addPlane( const PlaneF &plane ) {;}
   
   /// Add a box to the collision shape.
   virtual void addBox( const Point3F &halfWidth,
                       const MatrixF &localXfm ) {;}
   
   /// Add a sphere to the collision shape.
   virtual void addSphere( F32 radius,
                          const MatrixF &localXfm ) {;}
   
   /// Add a Y axis capsule to the collision shape.
   virtual void addCapsule(   F32 radius,
                           F32 height,
                           const MatrixF &localXfm ) {;}
   
   /// Add a point cloud convex hull to the collision shape.
   virtual bool addConvex( const Point3F *points,
                          U32 count,
                          const MatrixF &localXfm ) { return false; }
   
   /// Add a triangle mesh to the collision shape.
   virtual bool addTriangleMesh( const Point3F *vert,
                                U32 vertCount,
                                const U32 *index,
                                U32 triCount,
                                const MatrixF &localXfm ) { return false; }
   
   /// Add a heightfield to the collision shape.
   virtual bool addHeightfield(  const U16 *heights,
                               const bool *holes,
                               U32 blockSize,
                               F32 metersPerSample,
                               const MatrixF &localXfm ) { return false; }
};

PhysicsCollision *PhysicsCollision::create()
{
   return new DummyPhysicsCollision();
}

//-----------------------------------------------------------------------------

END_NS

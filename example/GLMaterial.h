#ifndef _GLMATERIAL_H_
#define _GLMATERIAL_H_

#include <GL/glew.h>

// Basic OpenGL Material Instance
class GLTSMaterialInstance : public TSMaterialInstance
{
public:
   GLTSMaterial *mMaterial;
   GLuint mTexture;
   
   GLTSMaterialInstance(GLTSMaterial *mat);
   ~GLTSMaterialInstance();
   
   virtual bool init(const GFXVertexFormat *fmt=NULL);
   
   virtual TSMaterial *getMaterial();
   
   virtual bool isTranslucent();
   
   virtual bool isValid();
   
   virtual int getStateHint();
   
   virtual const char* getName();
   
   void activate();
};

// Basic OpenGL Material
class GLTSMaterial : public TSMaterial
{
public:
   String mMapTo;
   String mName;
   
   GLTSMaterial()
   {
      
   }
   
   ~GLTSMaterial()
   {
   }
   
   // Create an instance of this material
   virtual TSMaterialInstance *createMatInstance(const GFXVertexFormat *fmt=NULL)
   {
      GLTSMaterialInstance *inst = new GLTSMaterialInstance(this);
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

// Basic OpenGL Material Manager
class GLTSMaterialManager : public TSMaterialManager
{
public:
   GLTSMaterial *mDummyMaterial;
   Vector<GLTSMaterial*> mMaterials;
   
   GLTSMaterialManager();
   ~GLTSMaterialManager();
   
   TSMaterial *allocateAndRegister(const String &objectName, const String &mapToName = String());

   // Grab an existing material
   virtual TSMaterial * getMaterialDefinitionByName(const String &matName);
   
   // Return instance of named material caller is responsible for memory
   virtual TSMaterialInstance * createMatInstance( const String &matName, const GFXVertexFormat *vertexFormat = NULL );
   virtual TSMaterialInstance * createFallbackMatInstance( const GFXVertexFormat *vertexFormat = NULL );
};


#endif // _GLMATERIAL_H_

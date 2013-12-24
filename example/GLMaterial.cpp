//
//  GLRenderer.cpp
//  DTSTest
//
//  Created by James Urquhart on 24/12/2013.
//  Copyright (c) 2013 James Urquhart. All rights reserved.
//

#include "GLRenderer.h"

GLTSMaterialInstance::GLTSMaterialInstance(GLTSMaterial *mat) :
mMaterial(mat)
{
   mTexture = NULL;
}

GLTSMaterialInstance::~GLTSMaterialInstance()
{
   if (mTexture)
      glDeleteTextures(1, &mTexture);
}


bool GLTSMaterialInstance::init(const GFXVertexFormat *fmt)
{
   // Load materials
   const char *name = mMaterial->getName();
   
   if (mTexture)
      glDeleteTextures(1, &mTexture);
   
   char nameBuf[256];
   dSprintf(nameBuf, 256, "%s.png", name);
   mTexture = SOIL_load_OGL_texture(GetAssetPath(nameBuf),
                                    SOIL_LOAD_AUTO,
                                    SOIL_CREATE_NEW_ID,
                                    SOIL_FLAG_MIPMAPS);
   
   return mTexture != 0;
}

TSMaterial *GLTSMaterialInstance::getMaterial()
{
   return mMaterial;
}

bool GLTSMaterialInstance::isTranslucent()
{
   return false;
}

bool GLTSMaterialInstance::isValid()
{
   return mTexture != NULL;
}

void GLTSMaterialInstance::activate()
{
   // Set texture
   if (mTexture)
   {
      glEnable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, mTexture);
   }
   else
   {
      glDisable(GL_TEXTURE_2D);
   }
}

int GLTSMaterialInstance::getStateHint()
{
   return (S64)mMaterial;
}

const char* GLTSMaterialInstance::getName()
{
   return mMaterial->getName();
}

GLTSMaterial::GLTSMaterial()
{
  
}

GLTSMaterial::~GLTSMaterial()
{
}

// Create an instance of this material
TSMaterialInstance *GLTSMaterial::createMatInstance(const GFXVertexFormat *fmt=NULL)
{
  GLTSMaterialInstance *inst = new GLTSMaterialInstance(this);
  return inst;
}

// Assign properties from collada material
bool GLTSMaterial::initFromColladaMaterial(const ColladaAppMaterial *mat)
{
  return true;
}

// Get base material definition
TSMaterial *GLTSMaterial::getMaterial()
{
  return this;
}

bool GLTSMaterial::isTranslucent()
{
  return false;
}

const char* GLTSMaterial::getName()
{
  return mName;
}

GLTSMaterialManager::GLTSMaterialManager()
{
  mDummyMaterial = NULL;
}

GLTSMaterialManager::~GLTSMaterialManager()
{
  for (int i=0; i<mMaterials.size(); i++)
     delete mMaterials[i];
  mMaterials.clear();
}

TSMaterial *GLTSMaterialManager::allocateAndRegister(const String &objectName, const String &mapToName = String())
{
  GLTSMaterial *mat = new GLTSMaterial();
  mat->mName = objectName;
  mat->mMapTo = mapToName;
  mMaterials.push_back(mat);
  return mat;
}

// Grab an existing material
TSMaterial *GLTSMaterialManager::getMaterialDefinitionByName(const String &matName)
{
  Vector<GLTSMaterial*>::iterator end = mMaterials.end();
  for (Vector<GLTSMaterial*>::iterator itr = mMaterials.begin(); itr != end; itr++)
  {
     if ((*itr)->mName == matName)
        return *itr;
  }
  return NULL;
}

// Return instance of named material caller is responsible for memory
TSMaterialInstance *GLTSMaterialManager::createMatInstance( const String &matName, const GFXVertexFormat *vertexFormat = NULL )
{
  TSMaterial *mat = getMaterialDefinitionByName(matName);
  if (mat)
  {
     return mat->createMatInstance(vertexFormat);
  }
  
  return NULL;
}

TSMaterialInstance *GLTSMaterialManager::createFallbackMatInstance( const GFXVertexFormat *vertexFormat = NULL )
{
  if (!mDummyMaterial)
  {
     mDummyMaterial = (GLTSMaterial*)allocateAndRegister("Dummy", "Dummy");
  }
  return mDummyMaterial->createMatInstance(vertexFormat);
}


TSMeshRenderer *TSMeshRenderer::create()
{
   return new GLTSMeshRenderer();
}

TSMeshInstanceRenderData *TSMeshInstanceRenderData::create()
{
   return new GLTSMeshInstanceRenderData();
}

/*
Copyright (C) 2013 James S Urquhart

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/

#include "SDL.h"
#include "GLIncludes.h"
#include "GLMaterial.h"
#include "GLSimpleShader.h"
#include "GLTSMeshRenderer.h"

#include "soil/SOIL.h"
#include "core/strings/stringFunctions.h"
#include "core/util/tVector.h"
#include "main.h"

#include "ts/tsShape.h"

static const char *sMaterialTextureExts[] = {
  "png",
  "dds"
};

extern const char* GetAssetPath(const char *file);

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
   mTexture = 0;

   int len = sizeof(sMaterialTextureExts) / sizeof(const char*);

   for (int i=0; i<len; i++)
   {
      dSprintf(nameBuf, 256, "%s.%s", name, sMaterialTextureExts[i]);
      
      if (!Platform::isFile(GetAssetPath(nameBuf)))
         continue;
      
      mTexture = SOIL_load_OGL_texture(GetAssetPath(nameBuf),
                                    SOIL_LOAD_AUTO,
                                    SOIL_CREATE_NEW_ID,
                                    SOIL_FLAG_MIPMAPS);
      
      if (mTexture != 0)
         break;
   }
   
   // Choose correct shader based on skinning requirements
   
   AppState *app = AppState::getInstance();
   mVertexFormat = fmt;
   
   if (DTShape::TSShape::smUseHardwareSkinning && mVertexFormat->hasBlend())
   {
      mShader = app->sSkinningShader;
   }
   else
   {
      mShader = app->sShader;
   }
   
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
   return mTexture != 0;
}

void GLTSMaterialInstance::activate(GLTSSceneRenderState *sceneState, TSRenderInst *renderInst)
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

   // Bind shader parameters. We'll use the current shader from the app instance
   AppState *app = AppState::getInstance();

   // Calculate base matrix for the render inst
   MatrixF invView = sceneState->mViewMatrix;
   invView.inverse();
   MatrixF glModelView = invView;
   glModelView *= sceneState->mWorldMatrix;
   glModelView.mul(*renderInst->objectToWorld);

   // Update transforms using correct shader for mesh
   mShader->setProjectionMatrix(sceneState->mProjectionMatrix);
   mShader->setModelViewMatrix(glModelView);
   mShader->setLightPosition(app->mLightPos);
   mShader->setLightColor(ColorF(1,1,1,1));
   mShader->use();
   
   if (renderInst->mNumNodeTransforms == 0)
   {
      // First (and only) bone transform should be identity
      MatrixF identity(1);
      mShader->updateBoneTransforms(1, &identity);
   }
   else
   {
      // Use matrix list from render instance
      mShader->updateBoneTransforms(renderInst->mNumNodeTransforms, renderInst->mNodeTransforms);
   }
   
   mShader->updateTransforms();
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
TSMaterialInstance *GLTSMaterial::createMatInstance(const GFXVertexFormat *fmt)
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

TSMaterial *GLTSMaterialManager::allocateAndRegister(const String &objectName, const String &mapToName)
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
TSMaterialInstance *GLTSMaterialManager::createMatInstance( const String &matName, const GFXVertexFormat *vertexFormat)
{
  TSMaterial *mat = getMaterialDefinitionByName(matName);
  if (mat)
  {
     return mat->createMatInstance(vertexFormat);
  }
  
  return NULL;
}

TSMaterialInstance *GLTSMaterialManager::createFallbackMatInstance( const GFXVertexFormat *vertexFormat )
{
  if (!mDummyMaterial)
  {
     mDummyMaterial = (GLTSMaterial*)allocateAndRegister("Dummy", "Dummy");
  }
  return mDummyMaterial->createMatInstance(vertexFormat);
}


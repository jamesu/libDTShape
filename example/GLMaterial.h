#ifndef _GLMATERIAL_H_
#define _GLMATERIAL_H_

/*
Copyright (C) 2019 James S Urquhart

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

#include "ts/tsRender.h"
#include "ts/tsRenderState.h"
#include "ts/tsMaterial.h"
#include "ts/tsMaterialManager.h"
#include "core/util/tVector.h"

#include <unordered_map>
#include "GLIncludes.h"

using namespace DTShape;

class GLTSMaterial;
class GLTSSceneRenderState;
class GLSimpleShader;

struct GLPipelineState
{
   GLuint program;
   uint32_t variantIdx;
   
   GLuint ubo;
   GLuint ubo_nodes;
   
   GLuint depthCompare;
   GLuint blendSrc;
   GLuint blendDest;
   GLuint blendEq;
   
   GFXVertexFormat vertexFormat;
   
   GLint bindingUBO;
   GLint bindingNodeUBO;
   
   GLPipelineState() : program(0), ubo(0), ubo_nodes(0) {;}
   
   ~GLPipelineState()
   {
      if (program) glDeleteProgram(program);
      if (ubo) glDeleteBuffers(1, &ubo);
      if (ubo_nodes) glDeleteBuffers(1, &ubo_nodes);
   }
   
   void bindVertexDescriptor();
};

// Basic OpenGL Material Instance
class GLTSMaterialInstance : public TSMaterialInstance
{
public:
   GLTSMaterial *mMaterial;
   GLPipelineState* mPipeline;
   
   GLuint mTexture;
   GLuint mSampler;
   
   const GFXVertexFormat *mVertexFormat;
   
   GLTSMaterialInstance(GLTSMaterial *mat);
   ~GLTSMaterialInstance();
   
   virtual bool init(TSMaterialManager* mgr, const GFXVertexFormat *fmt=NULL);
   
   virtual TSMaterial *getMaterial();
   
   virtual bool isTranslucent();
   
   virtual bool isValid();
   
   virtual int getStateHint();
   
   virtual const char* getName();
   
   void bindVertexBuffers();
   
   void activate(const TSSceneRenderState *sceneState, TSRenderInst *renderInst);
};

// Basic OpenGL Material
class GLTSMaterial : public TSMaterial
{
public:
   String mMapTo;
   String mName;
   
   GLTSMaterial();
   ~GLTSMaterial();
   
   // Create an instance of this material
   virtual TSMaterialInstance *createMatInstance(const GFXVertexFormat *fmt=NULL);
   
   // Assign properties from collada material
   virtual bool initFromColladaMaterial(const ColladaAppMaterial *mat);
   
   // Get base material definition
   virtual TSMaterial *getMaterial();
   
   virtual bool isTranslucent();
   
   virtual const char* getName();
};

// Basic OpenGL Material Manager
class TSGLPipelineCollection;
class GLTSMaterialManager : public TSMaterialManager
{
public:
   
   GLTSMaterial *mDummyMaterial;
   Vector<GLTSMaterial*> mMaterials;
   
   std::unordered_map<const char*, TSGLPipelineCollection*> mPipelines;
   
   GLTSMaterialManager();
   ~GLTSMaterialManager();
   
   void init();
   void cleanup();
   
   TSMaterial *allocateAndRegister(const String &objectName, const String &mapToName = String());

   // Grab an existing material
   virtual TSMaterial * getMaterialDefinitionByName(const String &matName);
   
   // Return instance of named material caller is responsible for memory
   virtual TSMaterialInstance * createMatInstance( const String &matName, const GFXVertexFormat *vertexFormat = NULL );
   virtual TSMaterialInstance * createFallbackMatInstance( const GFXVertexFormat *vertexFormat = NULL );
};


#endif // _GLMATERIAL_H_

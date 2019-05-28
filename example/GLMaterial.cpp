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

#include "SDL.h"
#include "GLMaterial.h"
#include "GLTSMeshRenderer.h"

#include "soil/SOIL.h"
#include "core/strings/stringFunctions.h"
#include "core/util/tVector.h"
#include "main.h"

#include "ts/tsShape.h"
#include <string>
#include <vector>
#include "GLRenderer.h"

static const char *sMaterialTextureExts[] = {
  "png",
  "dds"
};

extern const char* GetAssetPath(const char *file);

class TSGLPipelineCollection;

void GFXToGLVertexFormat(const GFXDeclType &fmt, GLuint &outType, GLuint &outCount)
{
   switch (fmt)
   {
      case GFXDeclType_Float:
         outType = GL_FLOAT;
         outCount = 1;
         break;
         
      case GFXDeclType_Float2:
         outType = GL_FLOAT;
         outCount = 3;
         break;
      case GFXDeclType_Float3:
         outType = GL_FLOAT;
         outCount = 3;
         break;
      case GFXDeclType_Float4:
         outType = GL_FLOAT;
         outCount = 4;
         break;
      case GFXDeclType_Color:
         outType = GL_BYTE;
         outCount = 4;
         break;
      case GFXDeclType_UByte4:
         outType = GL_UNSIGNED_BYTE;
         outCount = 4;
         break;
      default:
         outType = GL_INVALID_ENUM;
         outCount = 0;
         break;
   }
}

void GLPipelineState::bindVertexDescriptor()
{
   U32 offset = 0;
   U32 stride = vertexFormat.getSizeInBytes();
   for (int i=0; i<16; i++)
   {
      glDisableVertexAttribArray(i);
   }
   
   for (U32 i=0; i<vertexFormat.getElementCount(); i++)
   {
      const GFXVertexElement &element = vertexFormat.getElement(i);
      GLuint declFmt;
      GLuint declSize;
      GFXToGLVertexFormat(element.getType(), declFmt, declSize);
      glVertexAttribPointer(i, declSize, declFmt, element.getSemantic() == GFXSemantic::NORMAL ? GL_TRUE : GL_FALSE, stride, (void*)offset);
      offset += element.getSizeInBytes();
      glEnableVertexAttribArray(i);
   }
}


// Up to VARIANT_SIZE
static void GFXGenVertexFormat(GFXVertexFormat &fmt, uint8_t variant)
{
   bool hasTexcoord2 = variant & 0x1;
   bool hasColors = variant & 0x2;
   bool hasSkin = variant & 0x4;
   
   fmt.clear();
   
   fmt.addElement( GFXSemantic::POSITION, GFXDeclType_Float3 );
   fmt.addElement( GFXSemantic::TANGENTW, GFXDeclType_Float, 3 );
   fmt.addElement( GFXSemantic::NORMAL, GFXDeclType_Float3 );
   fmt.addElement( GFXSemantic::TANGENT, GFXDeclType_Float3 );
   
   fmt.addElement( GFXSemantic::TEXCOORD, GFXDeclType_Float2, 0 );
   
   if(hasTexcoord2 || hasColors)
   {
      fmt.addElement( GFXSemantic::TEXCOORD, GFXDeclType_Float2, 1 );
      fmt.addElement( GFXSemantic::COLOR, GFXDeclType_Color );
   }
   
   if (hasSkin)
   {
      fmt.addElement( GFXSemantic::BLENDINDICES, GFXDeclType_UByte4 );
      fmt.addElement( GFXSemantic::BLENDWEIGHT, GFXDeclType_Float4 );
   }
   
   if ( (hasTexcoord2 || hasColors) )
   {
      if (!(hasSkin) )
      {
         fmt.addElement( GFXSemantic::TEXCOORD, GFXDeclType_Float, 2 );
      }
   }
   else if (hasSkin)
   {
      fmt.addElement( GFXSemantic::TEXCOORD, GFXDeclType_Float2, 1 );
      fmt.addElement( GFXSemantic::TEXCOORD, GFXDeclType_Float, 2 );
   }
}


const char* sStandardFragmentProgram = "#version 330 core\n\
\n\
in vec2 vTexCoord0;\n\
in vec4 vColor0;\n\
uniform sampler2D texture0;\n\
out vec4 Color;\n\
\n\
void main()\n\
{\n\
Color = texture(texture0, vTexCoord0);\n\
Color.r = Color.r * vColor0.r * vColor0.a;\n\
Color.g = Color.g * vColor0.g * vColor0.a;\n\
Color.b = Color.b * vColor0.b * vColor0.a;\n\
}\n\
";

const char* sStandardVertexProgram = "#version 330 core\n\
\n\
in vec3 aPosition;\n\
in vec3 aNormal;\n\
in vec2 aTexCoord0;\n\
\n\
layout (std140) uniform PushConstants\n\
{\n\
   mat4 worldMatrixProjection;\n\
   mat4 worldMatrix;\n\
   vec3 lightPos;\n\
   vec3 lightColor;\n\
} PushConstants;\n\
\n\
out vec2 vTexCoord0;\n\
out vec4 vColor0;\n\
\n\
void main()\n\
{\n\
vec3 normal, lightDir;\n\
vec4 diffuse;\n\
float NdotL;\n\
\n\
normal = normalize(mat3(PushConstants.worldMatrix) * aNormal);\n\
\n\
lightDir = normalize(vec3(PushConstants.lightPos));\n\
\n\
NdotL = max(dot(normal, lightDir), 0.0);\n\
\n\
diffuse = vec4(PushConstants.lightColor, 1.0);\n\
\n\
gl_Position = PushConstants.worldMatrixProjection * vec4(aPosition,1);\n\
vTexCoord0 = aTexCoord0;\n\
vColor0 = NdotL * diffuse;\n\
vColor0.a = 1.0;\n\
}\n\
";

const char* sSkinnedVertexProgram = "#version 330 core\n\
\n\
in vec3 aPosition;\n\
in vec4 aColor;\n\
in vec2 aTexCoord0;\n\
in vec3 aNormal;\n\
in vec4 aBlendIndices0;\n\
in vec4 aBlendWeights0;\n\
\n\
layout (std140) uniform PushConstants\n\
{\n\
mat4 worldMatrixProjection;\n\
mat4 worldMatrix;\n\
vec3 lightPos;\n\
vec3 lightColor;\n\
} PushConstants;\n\
layout (std140) uniform ModelTransforms\n\
{\n\
uniform mat4 xfm[64];\n\
} ModelTransforms;\n\
\n\
out vec2 vTexCoord0;\n\
out vec4 vColor0;\n\
\n\
void main()\n\
{\n\
vec3 normal, lightDir;\n\
vec4 diffuse;\n\
float NdotL;\n\
\n\
vec3 inPosition;\n\
vec3 inNormal;\n\
vec3 posePos = vec3(0.0);\n\
vec3 poseNormal = vec3(0.0);\n\
for (int i=0; i<4; i++) {\n\
mat4 mat = ModelTransforms.xfm[int(aBlendIndices0[i])];\n\
mat3 m33 = mat3x3(mat);\n\
posePos += (mat * vec4(aPosition, 1)).xyz * aBlendWeights0[i];\n\
poseNormal += (m33 * aNormal) * aBlendWeights0[i];\n\
}\n\
inPosition = posePos;\n\
inNormal = normalize(poseNormal);\n\
normal = normalize(mat3(PushConstants.worldMatrix) * inNormal);\n\
\n\
lightDir = normalize(vec3(PushConstants.lightPos));\n\
\n\
NdotL = max(dot(normal, lightDir), 0.0);\n\
\n\
diffuse = vec4(PushConstants.lightColor, 1.0);\n\
\n\
gl_Position = PushConstants.worldMatrixProjection * vec4(inPosition,1);\n\
vTexCoord0 = aTexCoord0;\n\
vColor0 = NdotL * diffuse;\n\
vColor0.a = 1.0;\n\
}\n\
";


class GLShaderCollection
{
public:
   std::unordered_map<const char*, GLuint> mShaders;
   
   static GLShaderCollection* smInstance;
   
   GLuint compileShader(GLuint shaderType, const char* data)
   {
      GLuint shader = glCreateShader(shaderType);
      glShaderSource(shader, 1, &data, NULL);
      glCompileShader(shader);
      
      GLint status;
      glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
      if (status == GL_FALSE)
      {
         GLint infoLogLength;
         glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
         GLchar *strInfoLog = new GLchar[infoLogLength + 1];
         glGetShaderInfoLog(shader, infoLogLength, NULL, strInfoLog);
         fprintf(stderr, "Failed to compile shader(%i)...\n%s", shaderType, strInfoLog);
         delete[] strInfoLog;
         return 0;
      }
      
      return shader;
   }
   
   void init()
   {
      mShaders["standard_fragment"] = compileShader(GL_FRAGMENT_SHADER, sStandardFragmentProgram);
      mShaders["standard_vert"] = compileShader(GL_VERTEX_SHADER, sStandardVertexProgram);
      mShaders["skinned_vert"] = compileShader(GL_VERTEX_SHADER, sSkinnedVertexProgram);
   }
   
   static GLShaderCollection* getInstance()
   {
      if (!smInstance)
      {
         smInstance = new GLShaderCollection();
         smInstance->init();
      }
      
      return smInstance;
   }
};

GLShaderCollection* GLShaderCollection::smInstance;

// Provides a collection of possible pipeline states for a fragment function
class TSGLPipelineCollection
{
public:
   enum
   {
      VARIANT_SIZE = 8
   };
   
   TSGLPipelineCollection(const char* fragment_name)
   {
      memset(mPipelines, '\0', sizeof(mPipelines));
      mFragmentName = fragment_name;
   }
   
   ~TSGLPipelineCollection()
   {
      for (int i=0; i<VARIANT_SIZE; i++)
      {
         if (mPipelines[i])
            delete mPipelines[i];
      }
   }
   
   const char* GFXSemToName(const GFXSemantic::GFXSemantic& sem)
   {
      const char* lookup[] = {
         "aPosition",
         "aNormal",
         "aBiNormal",
         "aTangent",
         "aTangentW",
         "aColor",
         "aTexCoord",
         "aBlendIndices",
         "aBlendWeights"
      };
      
      uint32_t semIdx = (uint32_t)sem;
      if (semIdx < sizeof(lookup) / sizeof(lookup[0]))
         return lookup[semIdx];
      else
         return NULL;
   }
   
   GLuint mBasicVertexFunction;
   GLuint mSkinnedVertexFunction;
   GLuint mFragmentFunction;
   std::string mFragmentName;
   
   GLPipelineState *mPipelines[VARIANT_SIZE];
   
   void bindVertexFormat(GLuint programID, GFXVertexFormat &fmt)
   {
      U32 offset = 0;
      for (U32 i=0; i<fmt.getElementCount(); i++)
      {
         const GFXVertexElement &element = fmt.getElement(i);
         
         const char* semName = GFXSemToName(element.getSemantic());
         if (semName)
         {
            if (element.getSemantic() == GFXSemantic::TEXCOORD ||
                element.getSemantic() == GFXSemantic::BLENDWEIGHT ||
                element.getSemantic() == GFXSemantic::BLENDINDICES)
            {
               char buffer[32];
               snprintf(buffer, sizeof(buffer), "%s%u", semName, element.getSemanticIndex());
               if (glGetAttribLocation(programID, buffer) != -1)
               {
                  glBindAttribLocation(programID, i, buffer);
               }
            }
            else
            {
               if (glGetAttribLocation(programID, semName) != -1)
               {
                  glBindAttribLocation(programID, i, semName);
               }
            }
         }
         
         offset += element.getSizeInBytes();
      }
   }
   
   void init()
   {
      GLShaderCollection* shaderCollection = GLShaderCollection::getInstance();
      mBasicVertexFunction = shaderCollection->mShaders["standard_vert"];
      mSkinnedVertexFunction = shaderCollection->mShaders["skinned_vert"];
      mFragmentFunction = shaderCollection->mShaders["standard_fragment"];
      
      // Generate basic pipeline variants for the input vertex format
      // NOTE: Could possibly skip pipelines we will never use, but we'll make all of them for now.
      GFXVertexFormat fmt;
      GLRenderer* renderer = ((GLRenderer*)(AppState::getInstance()->mRenderer));
      
      GLint n = 0;
      glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &n);
      
      for (int i=0; i<VARIANT_SIZE; i++)
      {
         GFXGenVertexFormat(fmt, i);
         
         GLuint programID = glCreateProgram();
         
         glAttachShader(programID, ((i & 0x4) && TSShape::smUseHardwareSkinning && !TSShape::smUseComputeSkinning) ? mSkinnedVertexFunction : mBasicVertexFunction);
         glAttachShader(programID, mFragmentFunction);
         
         glLinkProgram(programID);
         
         GLint status = 0, log_length = 0;
         glGetProgramiv(programID, GL_LINK_STATUS, &status);
         glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &log_length);
         if (status == GL_FALSE)
            fprintf(stderr, "ERROR: failed to link variant %i!\n", i);
         
         if (log_length > 0)
         {
            std::vector<GLchar> buf;
            buf.resize((int)(log_length + 1));
            glGetProgramInfoLog(programID, log_length, NULL, &buf[0]);
            fprintf(stderr, "%s\n", &buf[0]);
         }
         
         bindVertexFormat(programID, fmt);
         
         glLinkProgram(programID);
         
         GLPipelineState* state = new GLPipelineState();
         state->variantIdx = i;
         state->program = programID;
         state->depthCompare = GL_LESS;
         state->blendSrc = GL_NONE;
         state->blendDest = GL_NONE;
         state->blendEq = GL_NONE;
         state->ubo = 0;
         state->ubo_nodes = 0;
         state->bindingUBO = 0;
         state->bindingNodeUBO = 0;
         state->vertexFormat.copy(fmt);
         
         state->bindingUBO = glGetUniformBlockIndex(programID, "PushConstants");
         state->bindingNodeUBO = glGetUniformBlockIndex(programID, "ModelTransforms");
         
         glUniformBlockBinding(programID, state->bindingUBO, 0);
         if (state->bindingNodeUBO != -1)
         {
            glUniformBlockBinding(programID, state->bindingNodeUBO, 1);
         }
         
         glGenBuffers(1, &state->ubo);
         glGenBuffers(1, &state->ubo_nodes);
         
         if (mPipelines[i])
            delete mPipelines[i];
         mPipelines[i] = state;
      }
   }
   
   // Picks a pipeline suitable for specified vertex format
   GLPipelineState* resolvePipeline(const GFXVertexFormat& fmt, bool forceNoBlend)
   {
      uint8_t variant=0;
      if (fmt.hasBlend())
      {
         variant |= 0x4;
      }
      
      for (U32 i=0; i<fmt.getElementCount(); i++)
      {
         const GFXVertexElement &element = fmt.getElement(i);
         if (element.getSemantic() == GFXSemantic::COLOR)
         {
            variant |= 0x2;
            variant |= 0x1;
         }
      }
      
      if (variant >= VARIANT_SIZE)
         return NULL;
      
      return mPipelines[variant];
   }
};

GLTSMaterialInstance::GLTSMaterialInstance(GLTSMaterial *mat) :
mMaterial(mat)
{
   mTexture = 0;
   mSampler = 0;
}

GLTSMaterialInstance::~GLTSMaterialInstance()
{
   if (mSampler)
      glDeleteSamplers(1, &mSampler);
   if (mTexture)
      glDeleteTextures(1, &mTexture);
}


bool GLTSMaterialInstance::init(TSMaterialManager* mgr, const GFXVertexFormat *fmt)
{
   // Load materials
   const char *name = mMaterial->getName();
   
   char nameBuf[256];
   mTexture = 0;
   AppState *app = AppState::getInstance();

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
   
   mVertexFormat = fmt;
   
   glGenSamplers(1, &mSampler);
   
   glSamplerParameteri(mSampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
   glSamplerParameteri(mSampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glSamplerParameteri(mSampler, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glSamplerParameteri(mSampler, GL_TEXTURE_WRAP_T, GL_REPEAT);
   glSamplerParameteri(mSampler, GL_TEXTURE_WRAP_R, GL_REPEAT);
   
   GLTSMaterialManager* mmgr = (GLTSMaterialManager*)mgr;
   TSGLPipelineCollection* collection = mmgr->mPipelines["standard_fragment"];
   mPipeline = collection->resolvePipeline(*fmt, !TSShape::smUseHardwareSkinning);
   
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

void GLTSMaterialInstance::bindVertexBuffers()
{
   mPipeline->bindVertexDescriptor();
}

void GLTSMaterialInstance::activate(const TSSceneRenderState *sceneState, TSRenderInst *renderInst)
{
   GLenum err = glGetError();
   
   // Bind pipeline
   glUseProgram(mPipeline->program);
   glBindTexture(GL_TEXTURE_2D, mTexture);
   glBindSampler(0, mSampler);
   
   
   if (mPipeline->depthCompare != GL_NONE)
   {
      glEnable(GL_DEPTH_TEST);
      glDepthFunc(mPipeline->depthCompare);
   }
   else
   {
      glDisable(GL_DEPTH_TEST);
   }
   
   if (mPipeline->blendSrc != GL_NONE)
   {
      glEnable(GL_BLEND);
      glBlendEquation(mPipeline->blendEq);
      glBlendFunc(mPipeline->blendSrc, mPipeline->blendDest);
   }
   else
   {
      glDisable(GL_BLEND);
   }
   
   glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
   glEnable(GL_CULL_FACE);
   glFrontFace(GL_CW);
   
   err = glGetError();
   
   // Bind shader parameters. We'll use the current shader from the app instance
   AppState *app = AppState::getInstance();
   
   PushConstants renderConstants;
   memcpy(&renderConstants, &app->renderConstants, sizeof(renderConstants));
   
   {
      // Calculate base matrix for the render inst
      MatrixF invView = *sceneState->getViewMatrix();
      invView.inverse();
      
      MatrixF glModelView = invView;
      glModelView *= *sceneState->getWorldMatrix();
      glModelView.mul(*renderInst->objectToWorld);
      
      MatrixF combined = (*sceneState->getProjectionMatrix()) * glModelView;
      combined.transpose();
      memcpy(&renderConstants.worldMatrixProjection, &combined, sizeof(MatrixF));
      
      MatrixF wm = *sceneState->getWorldMatrix();
      wm.mul(*renderInst->objectToWorld);
      wm.transpose();
      memcpy(&renderConstants.worldMatrix, &wm, sizeof(MatrixF));
   }
   
   glBindBuffer(GL_UNIFORM_BUFFER, mPipeline->ubo);
   glBufferData(GL_UNIFORM_BUFFER, sizeof(renderConstants), &renderConstants, GL_STREAM_DRAW);
   glBindBufferRange(GL_UNIFORM_BUFFER, 0, mPipeline->ubo , 0, sizeof(renderConstants));

   err = glGetError();
   
   if (TSShape::smUseComputeSkinning)
   {
      
   }
   else if (TSShape::smUseHardwareSkinning)
   {
      glBindBuffer(GL_UNIFORM_BUFFER, mPipeline->ubo_nodes);
      GLRenderer* renderer = ((GLRenderer*)(AppState::getInstance()->mRenderer));
      if (renderInst->mNumNodeTransforms == 0)
      {
         // First (and only) bone transform should be identity
         MatrixF identity(1);
         glBufferData(GL_UNIFORM_BUFFER, sizeof(MatrixF), &identity, GL_STREAM_DRAW);
         glBindBufferRange(GL_UNIFORM_BUFFER, 1, mPipeline->ubo_nodes , 0, sizeof(MatrixF));
      }
      else
      {
         uint32_t ofs=0;
         MatrixF* data = (MatrixF*)malloc(sizeof(MatrixF) * renderInst->mNumNodeTransforms);
         
         for (int i=0; i<renderInst->mNumNodeTransforms; i++)
         {
            MatrixF inM = renderInst->mNodeTransforms[i];
            inM.transpose();
            memcpy(&data[i], &inM ,sizeof(MatrixF));
         }
         
         glBufferData(GL_UNIFORM_BUFFER, sizeof(MatrixF) * renderInst->mNumNodeTransforms, data, GL_DYNAMIC_DRAW);
         free(data);
         glBindBufferRange(GL_UNIFORM_BUFFER, 1, mPipeline->ubo_nodes , 0, sizeof(MatrixF) * renderInst->mNumNodeTransforms);
         
         err = glGetError();
      }
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
  cleanup();
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

void GLTSMaterialManager::init()
{
   cleanup();
   
   GLShaderCollection* shaders = GLShaderCollection::getInstance();
   
   TSGLPipelineCollection* collection = new TSGLPipelineCollection("standard_fragment");
   collection->init();
   mPipelines["standard_fragment"] = collection;
}

void GLTSMaterialManager::cleanup()
{
   for (std::unordered_map<const char*, TSGLPipelineCollection*>::iterator itr = mPipelines.begin(); itr != mPipelines.end(); itr++)
   {
      delete itr->second;
   }
   mPipelines.clear();
}


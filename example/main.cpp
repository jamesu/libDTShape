//
//  main.m
//  HelloSDL
//
//  Created by James Urquhart on 03/02/2013.
//
//

#include "platform/platform.h"

#define GL_GLEXT_PROTOTYPES

#include "SDL.h"
#include <GL/glew.h>


#include "ts/tsShape.h"
#include "ts/tsShapeInstance.h"
#include "ts/tsRenderState.h"
#include "ts/tsMaterialManager.h"
#include "core/color.h"
#include "math/mMath.h"
#include "math/util/frustum.h"
#include "core/stream/fileStream.h"

#include "soil/SOIL.h"

#define TICK_TIME 32


class GLTSMaterial;

const char* GetAssetPath(const char *file);

const char sVertexProgram[] = "\n\
#version 120\n\
\n\
attribute vec4 aPosition;\n\
attribute vec4 aColor;\n\
attribute vec2 aTexCoord0;\n\
attribute vec3 aNormal;\n\
\n\
uniform mat4 worldMatrixProjection;\n\
uniform mat4 worldMatrix;\n\
uniform vec3 lightPos;\n\
uniform vec3 lightColor;\n\
\n\
varying vec2 vTexCoord0;\n\
varying vec4 vColor0;\n\
\n\
void main()\n\
{\n\
vec3 normal, lightDir;\n\
vec4 diffuse;\n\
float NdotL;\n\
\n\
normal = normalize(mat3(worldMatrix) * aNormal);\n\
\n\
lightDir = normalize(vec3(lightPos));\n\
\n\
NdotL = max(dot(normal, lightDir), 0.0);\n\
\n\
diffuse = vec4(lightColor, 1.0);\n\
\n\
gl_Position = worldMatrixProjection * aPosition;\n\
vTexCoord0 = aTexCoord0;\n\
vColor0 = NdotL * diffuse;\n\
vColor0.a = 1.0;\n\
}\n\
";

const char sFragmentProgram[] = "\n\
#version 120\n\
\n\
varying vec2 vTexCoord0;\n\
varying vec4 vColor0;\n\
uniform sampler2D texture0;\n\
\n\
void main()\n\
{\n\
   gl_FragColor = texture2D(texture0, vTexCoord0);\n\
   gl_FragColor.r = gl_FragColor.r * vColor0.r * vColor0.a;\n\
   gl_FragColor.g = gl_FragColor.g * vColor0.g * vColor0.a;\n\
   gl_FragColor.b = gl_FragColor.b * vColor0.b * vColor0.a;\n\
}\n\
";

// Basic shader code
enum {
   kGLSimpleVertexAttrib_Position,
   kGLSimpleVertexAttrib_Color,
   kGLSimpleVertexAttrib_TexCoords,
   kGLSimpleVertexAttrib_Normal,
   
   kGLSimpleVertexAttrib_MAX,
};

/** vertex attrib flags */
enum {
   kGLSimpleVertexAttribFlag_None        = 0,
   
   kGLSimpleVertexAttribFlag_Position    = 1 << 0,
   kGLSimpleVertexAttribFlag_Color        = 1 << 1,
   kGLSimpleVertexAttribFlag_TexCoords    = 1 << 2,
   kGLSimpleVertexAttribFlag_Normal       = 1 << 3,
   
   kGLSimpleVertexAttribFlag_PosColorTexNormal = ( kGLSimpleVertexAttribFlag_Position | kGLSimpleVertexAttribFlag_Color | kGLSimpleVertexAttribFlag_TexCoords | kGLSimpleVertexAttribFlag_Normal ),
};

enum {
   kGLSimpleUniformMVPMatrix,
   kGLSimpleUniformMVMatrix,
   kGLSimpleUniformLightPos,
   kGLSimpleUniformLightColor,
   kGLSimpleUniformSampler,
   
   kGLSimpleUniform_MAX,
};


// DTS Sequences for sample shape

enum TSAppSequences {
   kTSRootAnim,
   kTSForwardAnim,
   kTSBackAnim,
   kTSSideAnim,
   kTSJumpAnim,
   
   kNumTSAppSequences
};

const char* sTSAppSequenceNames[] = {
   "root",
   "forward",
   "back",
   "side",
   "jump",
   "invalid"
};



// Basic GL state tracker
class GLStateTracker
{
public:
   GLStateTracker() :
   mPositionEnabled(false),
   mColorEnabled(false),
   mTexCoordEnabled(false),
   mNormalEnabled(false)
   {
   }
   
   void setVertexAttribs(unsigned int flags)
   {
      bool posOn = flags & kGLSimpleVertexAttribFlag_Position;
      bool colorOn = flags & kGLSimpleVertexAttribFlag_Color;
      bool texOn = flags & kGLSimpleVertexAttribFlag_TexCoords;
      bool normalOn = flags & kGLSimpleVertexAttribFlag_Normal;
      
      if (posOn != mPositionEnabled)
      {
         if (posOn)
            glEnableVertexAttribArray(kGLSimpleVertexAttrib_Position);
         else
            glDisableVertexAttribArray(kGLSimpleVertexAttrib_Position);
         mPositionEnabled = posOn;
      }
      
      if (colorOn != mColorEnabled)
      {
         if (colorOn)
            glEnableVertexAttribArray(kGLSimpleVertexAttrib_Color);
         else
            glDisableVertexAttribArray(kGLSimpleVertexAttrib_Color);
         mColorEnabled = colorOn;
      }
      
      if (texOn != mTexCoordEnabled)
      {
         if (texOn)
            glEnableVertexAttribArray(kGLSimpleVertexAttrib_TexCoords);
         else
            glDisableVertexAttribArray(kGLSimpleVertexAttrib_TexCoords);
         mTexCoordEnabled = texOn;
      }
      
      if (normalOn != mNormalEnabled)
      {
         if (texOn)
            glEnableVertexAttribArray(kGLSimpleVertexAttrib_Normal);
         else
            glDisableVertexAttribArray(kGLSimpleVertexAttrib_Normal);
         mNormalEnabled = normalOn;
      }
   }
   
   bool mPositionEnabled;
   bool mColorEnabled;
   bool mTexCoordEnabled;
   bool mNormalEnabled;
};

GLStateTracker gGLStateTracker;


// Simple Shader compiler
class GLSimpleShader
{
public:
   GLSimpleShader() :
   mProjectionMatrix(1),
   mModelViewMatrix(1)
   {
      GLuint program[2];
      
      program[0] = compile(GL_VERTEX_SHADER, sVertexProgram);
      program[1] = compile(GL_FRAGMENT_SHADER, sFragmentProgram);
      
      mProgram = linkProgram(program);
      
      glDeleteShader(program[0]);
      glDeleteShader(program[1]);
      
      dMemset(mLightPos, '\0', sizeof(mLightPos));
      dMemset(mLightColor, '\0', sizeof(mLightColor));
   }
   
   ~GLSimpleShader()
   {
      if (mProgram != 0)
         glDeleteProgram(mProgram);
   }
   
   GLuint linkProgram(GLuint *shaders)
   {
      GLuint program = glCreateProgram();
      
      glAttachShader(program, shaders[0]);
      glAttachShader(program, shaders[1]);
      
      glBindAttribLocation(program, kGLSimpleVertexAttrib_Position, "aPosition");
      glBindAttribLocation(program, kGLSimpleVertexAttrib_Color,  "aColor");
      glBindAttribLocation(program, kGLSimpleVertexAttrib_TexCoords, "aTexCoord0");
      glBindAttribLocation(program, kGLSimpleVertexAttrib_Normal, "aNormal");

      glLinkProgram(program);
      
      GLint status;
      glGetProgramiv (program, GL_LINK_STATUS, &status);
      if (status == GL_FALSE)
      {
         GLint infoLogLength;
         glGetShaderiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
         
         GLchar *strInfoLog = new GLchar[infoLogLength + 1];
         glGetShaderInfoLog(program, infoLogLength, NULL, strInfoLog);
         
         Log::printf("Failed to link shader...\n%s\n", strInfoLog);
         
         delete[] strInfoLog;
         glDeleteProgram(program);
         return 0;
      }
      
      glDetachShader(program, shaders[0]);
      glDetachShader(program, shaders[1]);
      
      glUseProgram(program);
      
      mUniforms[0] = glGetUniformLocation(program, "worldMatrixProjection");
      mUniforms[1] = glGetUniformLocation(program, "worldMatrix");
      mUniforms[2] = glGetUniformLocation(program, "lightPos");
      mUniforms[3] = glGetUniformLocation(program, "lightColor");
      
      return program;
   }
   
   GLuint compile(GLenum shaderType, const char *data)
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
         
         Log::errorf("Failed to compile shader(%i)...\n%s", shaderType, strInfoLog);
         
         delete[] strInfoLog;
         return 0;
      }
      
      return shader;
   }
   
   void use()
   {
      glUseProgram(mProgram);
   }
   
   void setLightPosition(Point3F pos)
   {
      mLightPos[0] = pos.x;
      mLightPos[1] = pos.y;
      mLightPos[2] = pos.z;
   }
   
   void setLightColor(ColorF col)
   {
      mLightColor[0] = col.red;
      mLightColor[1] = col.green;
      mLightColor[2] = col.blue;
   }
   
   void setProjectionMatrix(MatrixF &proj)
   {
      mProjectionMatrix = proj;
   }
   
   void setModelViewMatrix(MatrixF &modelview)
   {
      mModelViewMatrix = modelview;
   }
   
   void updateTransforms()
   {
      MatrixF combined = mProjectionMatrix * mModelViewMatrix;
      combined.transpose();
      glUniformMatrix4fv(mUniforms[kGLSimpleUniformMVPMatrix], 1, GL_FALSE, (F32*)combined);
      combined = mModelViewMatrix;
      combined.transpose();
      glUniformMatrix4fv(mUniforms[kGLSimpleUniformMVMatrix], 1, GL_FALSE, (F32*)combined);
      
      glUniform3fv(mUniforms[kGLSimpleUniformLightColor], 1, mLightColor);
      glUniform3fv(mUniforms[kGLSimpleUniformLightPos], 1, mLightPos);
   }
   
   MatrixF mProjectionMatrix;
   MatrixF mModelViewMatrix;
   
   F32 mLightPos[3];
   F32 mLightColor[3];
   
   GLuint            mProgram;
   GLint             mUniforms[kGLSimpleUniform_MAX];
};

// Generic interface which provides scene info to the rendering code
class GLTSSceneRenderState : public TSSceneRenderState
{
public:
   MatrixF mWorldMatrix;
   MatrixF mViewMatrix;
   MatrixF mProjectionMatrix;
   
   GLTSSceneRenderState()
   {
      mWorldMatrix = MatrixF(1);
      mViewMatrix = MatrixF(1);
      mProjectionMatrix = MatrixF(1);
   }
   
   ~GLTSSceneRenderState()
   {
   }
   
   virtual TSMaterialInstance *getOverrideMaterial( TSMaterialInstance *inst ) const
   {
      return inst;
   }
   
   virtual Point3F getCameraPosition() const
   {
      return mViewMatrix.getPosition();
   }
   
   virtual Point3F getDiffuseCameraPosition() const
   {
      return mViewMatrix.getPosition();
   }
   
   virtual RectF getViewport() const
   {
      return RectF(0,0,800,600);
   }
   
   virtual Point2F getWorldToScreenScale() const
   {
      return Point2F(1,1);
   }
   
   virtual const MatrixF *getWorldMatrix() const
   {
      return &mWorldMatrix;
   }
   
   virtual bool isShadowPass() const
   {
      return false;
   }
   
   // Shared matrix stuff
   virtual const MatrixF *getViewMatrix() const
   {
      return &mViewMatrix;
   }
   
   virtual const MatrixF *getProjectionMatrix() const
   {
      return &mProjectionMatrix;
   }
};

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


// Basic OpenGL Material Manager
class GLTSMaterialManager : public TSMaterialManager
{
public:
   GLTSMaterial *mDummyMaterial;
   Vector<GLTSMaterial*> mMaterials;
   
   GLTSMaterialManager()
   {
      mDummyMaterial = NULL;
   }
   
   ~GLTSMaterialManager()
   {
      for (int i=0; i<mMaterials.size(); i++)
         delete mMaterials[i];
      mMaterials.clear();
   }
   
   TSMaterial *allocateAndRegister(const String &objectName, const String &mapToName = String())
   {
      GLTSMaterial *mat = new GLTSMaterial();
      mat->mName = objectName;
      mat->mMapTo = mapToName;
      mMaterials.push_back(mat);
      return mat;
   }
   
   // Grab an existing material
   virtual TSMaterial * getMaterialDefinitionByName(const String &matName)
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
         mDummyMaterial = (GLTSMaterial*)allocateAndRegister("Dummy", "Dummy");
      }
      return mDummyMaterial->createMatInstance(vertexFormat);
   }
   
};

// Basic OpenGL Buffer Information
class GLTSMeshInstanceRenderData : public TSMeshInstanceRenderData
{
public:
   GLTSMeshInstanceRenderData() : mVB(0), mNumVerts(0) {;}
   
   ~GLTSMeshInstanceRenderData()
   {
      if (mVB != 0)
         glDeleteBuffers(1, &mVB);
      mVB = 0;
      mNumVerts = 0;
   }
   
   GLuint mVB;
   U32 mNumVerts;
};

static GLenum drawTypes[] = { GL_TRIANGLES, GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN };
#define getDrawType(a) (drawTypes[a])

// Generic interface which stores vertex buffers and such for TSMeshes
class GLTSMeshRenderer : public TSMeshRenderer
{
public:
   GLTSMeshRenderer() : mPB(0), mNumIndices(0) {;}
   virtual ~GLTSMeshRenderer() { clear(); }
   
   GLuint mPB;
   int mNumIndices;
   
   virtual void prepare(TSMesh *mesh, TSMeshInstanceRenderData *meshRenderData)
   {
      bool reloadPB = false;
      
      // Generate primitive buffer if neccesary
      if (mPB == 0 || mesh->indices.size() != mNumIndices)
      {
         if (mPB == 0)
         {
            glGenBuffers(1, &mPB);
         }
         reloadPB = true;
      }
      
      mNumIndices = mesh->indices.size();
      
      U32 size;
      GLTSMeshInstanceRenderData *renderData = (GLTSMeshInstanceRenderData*)meshRenderData;
      
      // Update vertex buffer
      // (NOTE: if we're a Skin mesh and want to save a copy, we can also use mapVerts)
      if (renderData)
      {
         // First, ensure the buffer is valid
         if (renderData->mVB == 0 || renderData->mNumVerts != mesh->mNumVerts)
         {
            renderData->mNumVerts = mesh->mNumVerts;
            size = mesh->mVertexFormat->getSizeInBytes() * renderData->mNumVerts;
            
            // Load vertices
            if (renderData->mVB == 0)
               glGenBuffers(1, &renderData->mVB);
            glBindBuffer(GL_ARRAY_BUFFER, renderData->mVB);
            glBufferData(GL_ARRAY_BUFFER, size, NULL, mesh->mDynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
         }
         else
         {
            glBindBuffer(GL_ARRAY_BUFFER, renderData->mVB);
            size = mesh->mVertexFormat->getSizeInBytes() * renderData->mNumVerts;
         }
         
         // Now, load the transformed vertices straight from mVertexData
         char *vertexPtr = (char*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
         
         dMemcpy( vertexPtr, mesh->mVertexData.address(), size );
         
         glUnmapBuffer(GL_ARRAY_BUFFER);
         glBindBuffer(GL_ARRAY_BUFFER, 0);
      }
      
      // Load indices
      if (reloadPB)
      {
         size = sizeof(U16)*mNumIndices;
         glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mPB);
         glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, NULL, GL_STATIC_DRAW);
         
         // NOTE: Essentially all mesh indices in primitives are placed within 16bit boundaries
         // (see ColladaAppMesh::getPrimitives), so we can safely just convert them to U16s.
         U16 *indexPtr = (U16*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
         dCopyArray( indexPtr, mesh->indices.address(), mesh->indices.size() );
         
         glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
         glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
      }
   }
   
   virtual U8* mapVerts(TSMesh *mesh, TSMeshInstanceRenderData *meshRenderData)
   {
      return NULL; // transformed vertices will be stored in mesh->mVertexData
   }
   
   virtual void unmapVerts(TSMesh *mesh, TSMeshInstanceRenderData *meshRenderData)
   {
      
   }
   
   void bindArrays(const GFXVertexFormat *fmt)
   {
      U32 stride = fmt->getSizeInBytes();
      
      bool hasPos = false;
      bool hasColor = false;
      bool hasTexcoord = false;
      bool hasNormal = false;
      U32 attribs = 0;
      
      // Loop thru the vertex format elements adding the array state...
      U32 texCoordIndex = 0;
      U8 *buffer = 0;
      GLenum __error = glGetError();
      
      for ( U32 i=0; i < fmt->getElementCount(); i++ )
      {
         const GFXVertexElement &element = fmt->getElement( i );
         
         if ( element.isSemantic( GFXSemantic::POSITION ) )
         {
            attribs |= kGLSimpleVertexAttribFlag_Position;
            if (!hasPos)
            {
               glVertexAttribPointer(kGLSimpleVertexAttrib_Position, element.getSizeInBytes() / 4, GL_FLOAT, GL_FALSE, stride, buffer );
               hasPos = true;
            }
            buffer += element.getSizeInBytes();
         }
         else if ( element.isSemantic( GFXSemantic::NORMAL ) )
         {
            attribs |= kGLSimpleVertexAttribFlag_Normal;
            if (!hasNormal)
            {
               glVertexAttribPointer(kGLSimpleVertexAttrib_Normal, element.getSizeInBytes() / 3, GL_FLOAT, GL_TRUE, stride, buffer );
               hasNormal = true;
            }
            buffer += element.getSizeInBytes();
         }
         else if ( element.isSemantic( GFXSemantic::COLOR ))
         {
            attribs |= kGLSimpleVertexAttribFlag_Color;
            if (!hasColor)
            {
               glVertexAttribPointer(kGLSimpleVertexAttrib_Color, element.getSizeInBytes(), GL_UNSIGNED_BYTE, GL_TRUE, stride, buffer );
               hasColor = true;
            }
            buffer += element.getSizeInBytes();
         }
         else // Everything else is a texture coordinate.
         {
            if (!hasTexcoord && (element.getSizeInBytes() / 4) == 2)
            {
               attribs |= kGLSimpleVertexAttribFlag_TexCoords;
               glVertexAttribPointer(kGLSimpleVertexAttrib_TexCoords, element.getSizeInBytes() / 4, GL_FLOAT, GL_FALSE, stride, buffer);
               hasTexcoord = true;
            }
            buffer += element.getSizeInBytes();
            ++texCoordIndex;
         }
      }
      __error = glGetError();
      
      gGLStateTracker.setVertexAttribs( attribs );
   }
   
   void unbindArrays(const GFXVertexFormat *fmt)
   {
      // reverse what we did in bindArrays
   }
   
   virtual void onAddRenderInst(TSMesh *mesh, TSRenderInst *inst, TSRenderState *renderState)
   {
      // we don't need to do anything here since
   }
   
   /// Renders whatever needs to be drawn
   virtual void doRenderInst(TSMesh *mesh, TSRenderInst *inst, TSRenderState *renderState)
   {
      //Platform::outputDebugString("[TSM:%x:%x]Rendering with material %s", this, inst->renderData, inst->matInst ? inst->matInst->getName() : "NIL");
      
      ((GLTSMaterialInstance*)inst->matInst)->activate();
      
      S32 matIdx = -1;
      
      // Bind VBOS
      GLTSMeshInstanceRenderData *renderData = (GLTSMeshInstanceRenderData*)inst->renderData;
      glBindBuffer(GL_ARRAY_BUFFER, renderData->mVB);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mPB);
      
      bindArrays(mesh->mVertexFormat);
      GLenum __error = glGetError();
      
      // basically, go through primitives & draw
      for (int i=0; i<mesh->primitives.size(); i++)
      {
         TSDrawPrimitive prim = mesh->primitives[i];
         // Use the first index to determine which 16-bit address space we are operating in
         U32 startVtx = mesh->indices[prim.start] & 0xFFFF0000;
         U32 numVerts = getMin((U32)0x10000, renderData->mNumVerts - startVtx);
         
         matIdx = prim.matIndex & (TSDrawPrimitive::MaterialMask|TSDrawPrimitive::NoMaterial);
         
         if (matIdx & TSDrawPrimitive::NoMaterial)
            continue;
         
         U16 *ptr = NULL;
         glDrawElements(getDrawType(prim.matIndex >> 30), prim.numElements, GL_UNSIGNED_SHORT, ptr+prim.start);
      }
      
      unbindArrays(mesh->mVertexFormat);
      
      // unbind
      glBindBuffer(GL_ARRAY_BUFFER, 0);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
   }
   
   /// Renders whatever needs to be drawn
   virtual bool isDirty(TSMesh *mesh, TSMeshInstanceRenderData *meshRenderData)
   {
      // Basically: are the buffers the correct size?
      GLTSMeshInstanceRenderData *renderData = (GLTSMeshInstanceRenderData*)meshRenderData;
      
      const bool vertsChanged = renderData->mVB == 0 || renderData->mNumVerts != mesh->mNumVerts;
      const bool primsChanged = mPB == 0 || mNumIndices != mesh->indices.size();
      
      if (vertsChanged || primsChanged)
         return true;
      else
         return false;
   }
   
   /// Cleans up Mesh renderer
   virtual void clear()
   {
      //Platform::outputDebugString("[TSM:%x]Clearing", this);
      
      if (mPB)
         glDeleteBuffers(1, &mPB);
      
      mPB = 0;
      mNumIndices = 0;
   }
};

TSMeshRenderer *TSMeshRenderer::create()
{
   return new GLTSMeshRenderer();
}

TSMeshInstanceRenderData *TSMeshInstanceRenderData::create()
{
   return new GLTSMeshInstanceRenderData();
}

class AppState
{
public:
   SDL_Window *window;
   SDL_GLContext glcontext;
   
   bool running;
   
   char shapeToLoad[256];
   
   static AppState *sInstance;
   
   TSShape *sShape;
   TSShapeInstance *sShapeInstance;
   TSThread *sThread;
   
   U32 sCurrentSequenceIdx;
   S32 sSequences[kNumTSAppSequences];
   
   GLTSMaterialManager sMaterialManager;
   
   GLSimpleShader *sShader;
   
   float sRot;
   float sDeltaRot;
   Point3F sCameraPosition;
   Point3F sModelPosition;
   MatrixF sProjectionMatrix;
   
   Point3F deltaCameraPos;
   
   U32 sOldTicks;
   
   AppState();
   ~AppState();
   
   int main(int argc, char **argv);
   void mainLoop();
   
   // Input handler
   void onKeyChanged(int key, int state);
   
   // Loads sample shape
   bool LoadShape();
   
   // Unloads shape
   void CleanupShape();
   
   // Animates and draws shape
   void DrawShape(F32 dt);
   
   // Transitions to a new sequence
   void SwitchToSequence(U32 seqIdx, F32 transitionTime, bool doTransition);
   
   // Sets up projection matrix
   void SetupProjection();
   
   static void OnAppLog(U32 level, LogEntry *logEntry);
   static AppState *getInstance();
};

AppState *AppState::sInstance = NULL;

TSMaterialManager *TSMaterialManager::instance()
{
   return &AppState::getInstance()->sMaterialManager;
}

AppState::AppState()
{
   running = false;
   window = NULL;
   dStrcpy(shapeToLoad, "player.dts");
   sInstance = this;
   
   sShape = NULL;
   sShapeInstance = NULL;
   
   sThread = NULL;
   sCurrentSequenceIdx = 0;
   for (int i=0; i<kNumTSAppSequences; i++)
      sSequences[i] = -1;
   
   sShader = NULL;
   
   sRot = 180.0f;
   sDeltaRot = 0.0f;
   sCameraPosition = Point3F(0,-10,1);
   sModelPosition = Point3F(0,0,0);
   deltaCameraPos = Point3F(0,0,0);
   
   sOldTicks = 0;
}

AppState::~AppState()
{
   if (sShader)
      delete sShader;
   CleanupShape();
   
   if (window)
   {
      SDL_GL_DeleteContext(glcontext);
      SDL_DestroyWindow(window);
   }
   sInstance = NULL;
}

void AppState::onKeyChanged(int key, int state)
{
   switch (key)
   {
      case SDLK_ESCAPE:
         SDL_Quit();
         break;
      case SDLK_w:
         deltaCameraPos.y = state == 1 ? 1.0 : 0.0;
         break;
      case SDLK_a:
         deltaCameraPos.x = state == 1 ? -1.0 : 0.0;
         break;
      case SDLK_s:
         deltaCameraPos.y = state == 1 ? -1.0 : 0.0;
         break;
      case SDLK_d:
         deltaCameraPos.x = state == 1 ? 1.0 : 0.0;
         break;
      case SDLK_q:
         sDeltaRot = state == 1 ? -30 : 0.0;
         break;
      case SDLK_e:
         sDeltaRot = state == 1 ? 30 : 0.0;
         break;
         
      case SDLK_k:
         // Rotate shape left
         if (sShape && state == 0)
         {
            if (sCurrentSequenceIdx == 0)
               sCurrentSequenceIdx = kNumTSAppSequences-1;
            else
               sCurrentSequenceIdx--;
            SwitchToSequence(sSequences[sCurrentSequenceIdx], 0.5f, true);
         }
         break;
      case SDLK_l:
         // Rotate shape right
         if (sShape && state == 0)
         {
            if (sCurrentSequenceIdx == kNumTSAppSequences-1)
               sCurrentSequenceIdx = 0;
            else
               sCurrentSequenceIdx++;
            SwitchToSequence(sSequences[sCurrentSequenceIdx], 0.5f, true);
         }
         break;
   }
}

void AppState::mainLoop()
{
   // Process events
   U32 tickTime = SDL_GetTicks();
   U32 deltaTick = (tickTime - sOldTicks);
   if (deltaTick > 1000)
      deltaTick = 1000;
   F32 deltaTime = deltaTick / 1000.0f;
   sOldTicks = tickTime;
   
   SDL_Event e;
   
   // Check input
   while (SDL_PollEvent(&e))
   {
      switch (e.type)
      {
         case SDL_QUIT:
            running = false;
            return;
            break;
         case SDL_KEYDOWN:
            onKeyChanged(e.key.keysym.sym, 1);
            break;
         case SDL_KEYUP:
            onKeyChanged(e.key.keysym.sym, 0);
            break;
      }
   }
   
   // Draw scene
   glClearColor(1,1,1,1); // Use OpenGL commands, see the OpenGL reference.
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clearing screen
   
   SetupProjection();
   DrawShape(deltaTime);
   
   SDL_GL_SwapWindow(window);
   
   // Slow down if we're going too fast
   U32 tickEnd = SDL_GetTicks();
   deltaTick = (tickEnd - tickTime);
   if (deltaTick < TICK_TIME)
      SDL_Delay(TICK_TIME - deltaTick);
}

bool AppState::LoadShape()
{
   TSMaterial *playerMat = TSMaterialManager::instance()->allocateAndRegister("player_blue");
   TSMaterialManager::instance()->mapMaterial("player_blue", "player_blue");
   
   const char *fullPath = GetAssetPath("player.dts");
   sShape = TSShape::loadShape(fullPath);
   
   if (!sShape)
   {
      return false;
   }
   
   sShapeInstance = new TSShapeInstance(sShape, true);
   
   // Load all dsq files
   for (int i=0; i<kNumTSAppSequences; i++)
   {
      FileStream dsqFile;
      char pathName[64];
      dSprintf(pathName, 64, "player_%s.dsq", sTSAppSequenceNames[i]);
      if (dsqFile.open(GetAssetPath(pathName), FileStream::Read) && sShape->importSequences(&dsqFile, ""))
      {
         Log::printf("Sequence file %s loaded", pathName);
      }
   }
   
   // Resolve all sequences
   for (int i=0; i<kNumTSAppSequences; i++)
   {
      sSequences[i] = sShape->findSequence(sTSAppSequenceNames[i]);
   }
   
   sThread = sShapeInstance->addThread();
   sShapeInstance->setSequence(sThread, sSequences[kTSRootAnim], 0);
   sShapeInstance->setTimeScale(sThread, 1.0f);
   
   // Load DTS
   sShape->initRender();
   return true;
}

void AppState::CleanupShape()
{
   if (sShapeInstance)
   {
      delete sShapeInstance;
   }
   
   if (sShape)
   {
      delete sShape;
   }
}

void AppState::DrawShape(F32 dt)
{
   GLTSSceneRenderState dummyScene;
   TSRenderState renderState;
   
   sCameraPosition += deltaCameraPos * dt;
   sRot += sDeltaRot * dt;
   
   glEnable(GL_DEPTH_TEST);
   
   MatrixF calc(1);
   
   // Set render transform
   dummyScene.mProjectionMatrix = sProjectionMatrix;
   dummyScene.mViewMatrix = MatrixF(1);
   dummyScene.mWorldMatrix = MatrixF(1);
   dummyScene.mWorldMatrix.setPosition(sModelPosition);
   
   // Apply model rotation
   calc = MatrixF(1);
   AngAxisF rot2 = AngAxisF(Point3F(0,0,1), mDegToRad(sRot));
   rot2.setMatrix(&calc);
   dummyScene.mWorldMatrix *= calc;
   
   // Set camera position
   calc = MatrixF(1);
   calc.setPosition(sCameraPosition);
   dummyScene.mViewMatrix *= calc;
   
   // Rotate camera
   rot2 = AngAxisF(Point3F(1,0,0), mDegToRad(-90.0f));
   calc = MatrixF(1);
   rot2.setMatrix(&calc);
   dummyScene.mViewMatrix *= calc;
   
   // Calculate lighting vector
   Point3F lightPos = -sCameraPosition;
   dummyScene.mViewMatrix.mulV(lightPos);
   
   // Calculate base modelview
   MatrixF glModelView = dummyScene.mWorldMatrix;
   MatrixF invView = dummyScene.mViewMatrix;
   invView.inverse();
   glModelView *= invView;
   
   // Set shader uniforms
   sShader->setProjectionMatrix(dummyScene.mProjectionMatrix);
   sShader->setModelViewMatrix(glModelView);
   sShader->setLightPosition(lightPos);
   sShader->setLightColor(ColorF(1,1,1,1));
   sShader->use();
   
   // Now we can render
   renderState.setSceneState(&dummyScene);
   renderState.reset();
   
   // Animate & render shape to the renderState
   sShapeInstance->advanceTime(dt);
   sShapeInstance->setCurrentDetail(0);
   sShapeInstance->animateNodeSubtrees(true);
   sShapeInstance->animate();
   sShapeInstance->render(renderState);
   
   // Ensure generated primitives are in the right z-order
   renderState.sortRenderInsts();
   
   // Render solid stuff
   for (int i=0; i<renderState.mRenderInsts.size(); i++)
   {
      glModelView = invView;
      glModelView *= dummyScene.mWorldMatrix;
      glModelView.mul(*renderState.mRenderInsts[i]->objectToWorld);
      
      // Update transform, & render buffers
      sShader->setModelViewMatrix(glModelView);
      sShader->updateTransforms();
      renderState.mRenderInsts[i]->render(&renderState);
   }
   
   // Render translucent stuff
   for (int i=0; i<renderState.mTranslucentRenderInsts.size(); i++)
   {
      glModelView = invView;
      glModelView *= dummyScene.mWorldMatrix;
      glModelView.mul(*renderState.mTranslucentRenderInsts[i]->objectToWorld);
      
      // Update transform, & render buffers
      sShader->setModelViewMatrix(glModelView);
      sShader->updateTransforms();
      renderState.mTranslucentRenderInsts[i]->render(&renderState);
   }
   
   glDisable(GL_DEPTH_TEST);
}

void AppState::SwitchToSequence(U32 seqIdx, F32 transitionTime, bool doTransition)
{
   Log::printf("Changing to sequence %s", sTSAppSequenceNames[seqIdx]);
   if (doTransition)
   {
      // Smoothly transition to new sequence
      if (sSequences[seqIdx] != -1)
      {
         F32 pos = 0.0f;
         if (sThread->getSeqIndex() == sSequences[seqIdx])
         {
            // Keep existing position if same sequence
            pos = sShapeInstance->getPos(sThread);
         }
         sShapeInstance->transitionToSequence(sThread,sSequences[seqIdx],
                                              pos, transitionTime, true);
      }
   }
   else
   {
      // Reset to new sequence
      if (sSequences[seqIdx] != -1)
      {
         sShapeInstance->setSequence(sThread, sSequences[seqIdx], 0.0f);
      }
   }
}

void AppState::SetupProjection()
{
   F32 viewportWidth = 800;
   F32 viewportHeight = 600;
   F32 viewportFOV = 70.0f;
   F32 aspectRatio = viewportWidth / viewportHeight;
   
   F32 viewportNear = 0.1f;
   F32 viewportFar = 200.0f;
   
   F32 wheight = viewportNear * mTan(viewportFOV / 2.0f);
   F32 wwidth = aspectRatio * wheight;
   
   F32 hscale = wwidth * 2.0f / viewportWidth;
   F32 vscale = wheight * 2.0f / viewportHeight;
   
   F32 left = (0) * hscale - wwidth;
   F32 right = (viewportWidth) * hscale - wwidth;
   F32 top = wheight - vscale * (0);
   F32 bottom = wheight - vscale * (viewportHeight);
   
   // Set & grab frustum
   Frustum frustum;
   frustum.set( false, left, right, top, bottom, viewportNear, viewportFar );
   frustum.getProjectionMatrix(&sProjectionMatrix, false);
}

int AppState::main(int argc, char **argv)
{
   Log::init();
   Log::addConsumer(OnAppLog);
   
   if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
      Log::errorf("Couldn't initialize SDL: %s\n", SDL_GetError());
      return (1);
   }
   
   window = SDL_CreateWindow("libdts Example", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
   
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
   
   SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
   SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
   
   // Create an OpenGL context associated with the window.
   glcontext = SDL_GL_CreateContext(window);
   
   GLenum err = glewInit();
   if (GLEW_OK != err)
   {
      Log::errorf("Error: %s\n", glewGetErrorString(err));
      return false;
   }
   
   Log::errorf("Using GLEW %s", glewGetString(GLEW_VERSION));
   
   sShader = new GLSimpleShader();
   bool loaded = LoadShape();
   
   if (!loaded)
   {
      Log::errorf("Couldn't load shape %s", shapeToLoad);
      return false;
   }
   
   running = true;
   return true;
}

void AppState::OnAppLog(U32 level, LogEntry *logEntry)
{
   switch (logEntry->mLevel)
   {
      case LogEntry::Normal:
         fprintf(stdout, "%s\n", logEntry->mData);
         break;
      case LogEntry::Warning:
         fprintf(stdout, "%s\n", logEntry->mData);
         break;
      case LogEntry::Error:
         fprintf(stderr, "%s\n", logEntry->mData);
         break;
   }
}

AppState *AppState::getInstance()
{
   return sInstance;
}

int main(int argc, char *argv[])
{
   AppState *app = new AppState();
   if (app->main(argc, argv))
   {
      while (app->running)
         app->mainLoop();
   }
   
   delete app;
   SDL_Quit();
   
   return (0);
}
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
#include "MetalMaterial.h"
#include "MetalTSMeshRenderer.h"

#include "soil/SOIL.h"
#include "core/strings/stringFunctions.h"
#include "core/util/tVector.h"
#include "main.h"

#include "ts/tsShape.h"
#include <string>
#include "MetalRenderer.h"

static const char *sMaterialTextureExts[] = {
  "png",
  "dds"
};

extern const char* GetAssetPath(const char *file);

class TSPipelineCollection;


MTLVertexFormat GFXToMetalVertexFormat(const GFXDeclType &fmt)
{
   switch (fmt)
   {
      case GFXDeclType_Float:
         return MTLVertexFormatFloat;
      case GFXDeclType_Float2:
         return MTLVertexFormatFloat2;
      case GFXDeclType_Float3:
         return MTLVertexFormatFloat3;
      case GFXDeclType_Float4:
         return MTLVertexFormatFloat4;
      case GFXDeclType_Color:
         return MTLVertexFormatUChar4Normalized;
      case GFXDeclType_UByte4:
         return MTLVertexFormatUChar4;
      default:
         return MTLVertexFormatInvalid;
   }
}

// Up to VARIANT_SIZE
void GFXGenVertexFormat(GFXVertexFormat &fmt, uint8_t variant)
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
      printf("METAL GEN SKIN OFFSET == %u\n", fmt.getSizeInBytes());
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

// Provides a collection of possible pipeline states for a fragment function
class TSPipelineCollection
{
public:
   enum
   {
      VARIANT_SIZE = 8
   };
   
   TSPipelineCollection(const char* fragment_name)
   {
      memset(mPipelines, '\0', sizeof(mPipelines));
      mFragmentName = fragment_name;
   }
   
   ~TSPipelineCollection()
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
   
   id<MTLFunction> mBasicVertexFunction;
   id<MTLFunction>  mSkinnedVertexFunction;
   id<MTLFunction> mFragmentFunction;
   std::string mFragmentName;
   
   MetalPipelineState *mPipelines[VARIANT_SIZE];
   
   MTLVertexDescriptor* GFXToMetalVertexDescriptor(GFXVertexFormat fmt, id<MTLFunction> &func)
   {
      MTLVertexDescriptor *vertexDescriptor = [[MTLVertexDescriptor alloc] init];
      
      NSMutableDictionary* dict = [NSMutableDictionary dictionaryWithCapacity:func.vertexAttributes.count];
      
      // We need to map from the internal vertex function to our vertex format.
      for (MTLVertexAttribute *attr in func.vertexAttributes)
      {
         printf("FUNC %s ATTR %i == %s\n", [func.name UTF8String], attr.attributeIndex, [attr.name UTF8String]);
         [dict setObject:[NSNumber numberWithUnsignedInt:attr.attributeIndex]  forKey:attr.name];
      }
      
      U32 offset = 0;
      printf("GFXToMetalVertexDescriptor size=%u\n", fmt.getSizeInBytes());
      for (U32 i=0; i<fmt.getElementCount(); i++)
      {
         const GFXVertexElement &element = fmt.getElement(i);
         
         const char* semName = GFXSemToName(element.getSemantic());
         if (semName)
         {
            NSNumber* attrIdx = nil;
            if (element.getSemantic() == GFXSemantic::TEXCOORD ||
                element.getSemantic() == GFXSemantic::BLENDWEIGHT ||
                element.getSemantic() == GFXSemantic::BLENDINDICES)
            {
               char buffer[32];
               snprintf(buffer, sizeof(buffer), "%s%u", semName, element.getSemanticIndex());
               attrIdx = [dict objectForKey:[NSString stringWithUTF8String:buffer]];
               
               printf("   fmt[%s@%u] @ %u\n", buffer, element.getSemanticIndex(), offset);
            }
            else
            {
               attrIdx = [dict objectForKey:[NSString stringWithUTF8String:semName]];
               printf("   fmt[%s@%u] @ %u\n", semName, element.getSemanticIndex(), offset);
            }
            
            if (attrIdx)
            {
               uint32_t index = attrIdx.integerValue;
               printf("     ^^ ASSIGNED TO IDX %u\n", index);
               MTLVertexFormat mtlVtxFmt = GFXToMetalVertexFormat(element.getType());
               vertexDescriptor.attributes[index].format = mtlVtxFmt;
               vertexDescriptor.attributes[index].offset = offset;
               vertexDescriptor.attributes[index].bufferIndex = element.getStreamIndex()+1; // 0 is for push constants
            }
         }
         
         offset += element.getSizeInBytes();
      }
      
      vertexDescriptor.layouts[1].stride = fmt.getSizeInBytes();
      vertexDescriptor.layouts[1].stepRate = 1;
      vertexDescriptor.layouts[1].stepFunction = MTLVertexStepFunctionPerVertex;
      
      return vertexDescriptor;
   }
   
   void init(id<MTLDevice> &device)
   {
      id<MTLLibrary> defaultLibrary = [device newDefaultLibrary];
      mBasicVertexFunction = [defaultLibrary newFunctionWithName:@"standard_vert"];
      mSkinnedVertexFunction = [defaultLibrary newFunctionWithName:@"skinned_vert"];
      mFragmentFunction = [defaultLibrary newFunctionWithName:[NSString stringWithUTF8String:mFragmentName.c_str()]];
      
      // Generate basic pipeline variants for the input vertex format
      // NOTE: Could possibly skip pipelines we will never use, but we'll make all of them for now.
      GFXVertexFormat fmt;
      MetalRenderer* renderer = ((MetalRenderer*)(AppState::getInstance()->mRenderer));
      MTLPixelFormat colorFormat = renderer->mColorFormat;
      for (int i=0; i<VARIANT_SIZE; i++)
      {
         GFXGenVertexFormat(fmt, i);
         printf("MTL GEN VARIANT %u\n", i);
         MTLVertexDescriptor *vertexDescriptor = GFXToMetalVertexDescriptor(fmt, ((i & 0x4) && TSShape::smUseHardwareSkinning && !TSShape::smUseComputeSkinning) ? mSkinnedVertexFunction : mBasicVertexFunction);
         NSError* err = nil;
         
         MTLRenderPipelineDescriptor *pipelineDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
         pipelineDescriptor.colorAttachments[0].pixelFormat = colorFormat;
         pipelineDescriptor.colorAttachments[0].blendingEnabled = NO;
         pipelineDescriptor.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;
         pipelineDescriptor.vertexFunction = ((i & 0x4) && TSShape::smUseHardwareSkinning && !TSShape::smUseComputeSkinning) ? mSkinnedVertexFunction : mBasicVertexFunction;
         pipelineDescriptor.vertexDescriptor = vertexDescriptor;
         pipelineDescriptor.fragmentFunction = mFragmentFunction;
         
         MTLDepthStencilDescriptor *depthDescriptor = [[MTLDepthStencilDescriptor alloc] init];
         [depthDescriptor setDepthCompareFunction:MTLCompareFunctionLessEqual];
         [depthDescriptor setDepthWriteEnabled:YES];
         
         MetalPipelineState* state = new MetalPipelineState();
         state->variantIdx = i;
         state->pipeline = [MetalTSMeshRenderer::smDevice newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&err];
         
         if (err)
         {
            printf("Error compiling pipeline: %s\n", [[err description] UTF8String]);
         }
         
         state->depth = [MetalTSMeshRenderer::smDevice newDepthStencilStateWithDescriptor:depthDescriptor];
         
         if (mPipelines[i])
            delete mPipelines[i];
         mPipelines[i] = state;
      }
   }
   
   // Picks a pipeline suitable for specified vertex format
   MetalPipelineState* resolvePipeline(const GFXVertexFormat& fmt, bool forceNoBlend)
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

MetalTSMaterialInstance::MetalTSMaterialInstance(MetalTSMaterial *mat) :
mMaterial(mat)
{
   mTexture = nil;
   mSampler = nil;
}

MetalTSMaterialInstance::~MetalTSMaterialInstance()
{
}


bool MetalTSMaterialInstance::init(TSMaterialManager* mgr, const GFXVertexFormat *fmt)
{
   // Load materials
   const char *name = mMaterial->getName();
   
   char nameBuf[256];
   mTexture = nil;
   AppState *app = AppState::getInstance();

   int len = sizeof(sMaterialTextureExts) / sizeof(const char*);

   for (int i=0; i<len; i++)
   {
      dSprintf(nameBuf, 256, "%s.%s", name, sMaterialTextureExts[i]);
      
      if (!Platform::isFile(GetAssetPath(nameBuf)))
         continue;
      
      int32_t width, height, channels;
      unsigned char* img = SOIL_load_image( GetAssetPath(nameBuf), &width, &height, &channels, 4 );
      
      if (img)
      {
         MTLTextureDescriptor *texDescriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm width:width height:height mipmapped:NO];
         texDescriptor.usage = MTLTextureUsageUnknown;
         texDescriptor.storageMode = MTLStorageModeManaged;
         texDescriptor.resourceOptions = MTLResourceStorageModeManaged;
         
         mTexture = [MetalTSMeshRenderer::smDevice newTextureWithDescriptor:texDescriptor];
         [mTexture replaceRegion:MTLRegionMake2D(0,0,width,height) mipmapLevel:0 withBytes:img bytesPerRow:width*4];
         free(img);
      }
      
      if (mTexture != 0)
         break;
   }
   
   // Choose correct shader based on skinning requirements
   
   mVertexFormat = fmt;
   
   MTLSamplerDescriptor *samplerDescriptor = [[MTLSamplerDescriptor alloc] init];
   
   
   samplerDescriptor.minFilter = MTLSamplerMinMagFilterLinear;
   samplerDescriptor.magFilter = MTLSamplerMinMagFilterLinear;
   samplerDescriptor.sAddressMode = MTLSamplerAddressModeRepeat;
   samplerDescriptor.tAddressMode = MTLSamplerAddressModeRepeat;
   samplerDescriptor.rAddressMode = MTLSamplerAddressModeRepeat;
   samplerDescriptor.mipFilter = MTLSamplerMipFilterLinear;
   samplerDescriptor.normalizedCoordinates = YES;
   
   mSampler = [MetalTSMeshRenderer::smDevice newSamplerStateWithDescriptor:samplerDescriptor];
   
   MetalTSMaterialManager* mmgr = (MetalTSMaterialManager*)mgr;
   TSPipelineCollection* collection = mmgr->mPipelines["standard_fragment"];
   mPipeline = collection->resolvePipeline(*fmt, !TSShape::smUseHardwareSkinning);
   
   return mTexture != 0;
}

TSMaterial *MetalTSMaterialInstance::getMaterial()
{
   return mMaterial;
}

bool MetalTSMaterialInstance::isTranslucent()
{
   return false;
}

bool MetalTSMaterialInstance::isValid()
{
   return mTexture != 0;
}

void MetalTSMaterialInstance::activate(const TSSceneRenderState *sceneState, TSRenderInst *renderInst)
{
   // Bind pipeline
   [MetalTSMeshRenderer::smCurrentRenderEncoder setRenderPipelineState:mPipeline->pipeline];
   [MetalTSMeshRenderer::smCurrentRenderEncoder setDepthStencilState:mPipeline->depth];
   
   [MetalTSMeshRenderer::smCurrentRenderEncoder setFragmentTexture:mTexture atIndex:0];
   [MetalTSMeshRenderer::smCurrentRenderEncoder setFragmentSamplerState:mSampler atIndex:0];
   
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
      
      // Calc new matrices
      MatrixF correctMat(1);
      correctMat.setColumn(0, Point4F(1,0,0,0));
      correctMat.setColumn(1, Point4F(0,1,0,0));
      correctMat.setColumn(2, Point4F(0,0,0.5,0));
      correctMat.setColumn(3, Point4F(0,0,0.5,1));
      
      MatrixF combined = *sceneState->getProjectionMatrix() * correctMat * glModelView;
      combined.transpose();
      memcpy(&renderConstants.worldMatrixProjection, &combined, sizeof(MatrixF));
      
      MatrixF wm = *sceneState->getWorldMatrix();
      wm.mul(*renderInst->objectToWorld);
      wm.transpose();
      memcpy(&renderConstants.worldMatrix, &wm, sizeof(MatrixF));
   }
   
   [MetalTSMeshRenderer::smCurrentRenderEncoder setVertexBytes:&renderConstants length:sizeof(renderConstants) atIndex:TS_PUSH_VBO];

   if (TSShape::smUseComputeSkinning)
   {
      
   }
   else if (TSShape::smUseHardwareSkinning)
   {
      MetalRenderer* renderer = ((MetalRenderer*)(AppState::getInstance()->mRenderer));
      if (renderInst->mNumNodeTransforms == 0)
      {
         // First (and only) bone transform should be identity
         MatrixF identity(1);
         uint32_t ofs=0;
         id<MTLBuffer> buffer;
         
         char* data = renderer->allocFrameVBO_Second(sizeof(MatrixF), buffer, ofs);
         memcpy(data, &identity, sizeof(MatrixF));
         
         //[buffer didModifyRange:NSMakeRange(ofs, sizeof(MatrixF))];
         [MetalTSMeshRenderer::smCurrentRenderEncoder setVertexBuffer:buffer offset:ofs atIndex:TS_BONE_VBO];
      }
      else
      {
         MatrixF identity(1);
         uint32_t ofs=0;
         id<MTLBuffer> buffer;
         MatrixF* data = (MatrixF*)renderer->allocFrameVBO_Second(sizeof(MatrixF) * renderInst->mNumNodeTransforms, buffer, ofs);
         
         for (int i=0; i<renderInst->mNumNodeTransforms; i++)
         {
            MatrixF inM = renderInst->mNodeTransforms[i];
            inM.transpose();
            memcpy(&data[i], &inM ,sizeof(MatrixF));
         }
         
         
         //[buffer didModifyRange:NSMakeRange(ofs, sizeof(MatrixF) * renderInst->mNumNodeTransforms)];
         [MetalTSMeshRenderer::smCurrentRenderEncoder setVertexBuffer:buffer offset:ofs atIndex:TS_BONE_VBO];
      }
   }
   
}

int MetalTSMaterialInstance::getStateHint()
{
   return (S64)mMaterial;
}

const char* MetalTSMaterialInstance::getName()
{
   return mMaterial->getName();
}

MetalTSMaterial::MetalTSMaterial()
{
  
}

MetalTSMaterial::~MetalTSMaterial()
{
}

// Create an instance of this material
TSMaterialInstance *MetalTSMaterial::createMatInstance(const GFXVertexFormat *fmt)
{
  MetalTSMaterialInstance *inst = new MetalTSMaterialInstance(this);
  return inst;
}

// Assign properties from collada material
bool MetalTSMaterial::initFromColladaMaterial(const ColladaAppMaterial *mat)
{
  return true;
}

// Get base material definition
TSMaterial *MetalTSMaterial::getMaterial()
{
  return this;
}

bool MetalTSMaterial::isTranslucent()
{
  return false;
}

const char* MetalTSMaterial::getName()
{
  return mName;
}

MetalTSMaterialManager::MetalTSMaterialManager()
{
  mDummyMaterial = NULL;
}

MetalTSMaterialManager::~MetalTSMaterialManager()
{
  for (int i=0; i<mMaterials.size(); i++)
     delete mMaterials[i];
  mMaterials.clear();
  cleanup();
}

TSMaterial *MetalTSMaterialManager::allocateAndRegister(const String &objectName, const String &mapToName)
{
  MetalTSMaterial *mat = new MetalTSMaterial();
  mat->mName = objectName;
  mat->mMapTo = mapToName;
  mMaterials.push_back(mat);
  return mat;
}

// Grab an existing material
TSMaterial *MetalTSMaterialManager::getMaterialDefinitionByName(const String &matName)
{
  Vector<MetalTSMaterial*>::iterator end = mMaterials.end();
  for (Vector<MetalTSMaterial*>::iterator itr = mMaterials.begin(); itr != end; itr++)
  {
     if ((*itr)->mName == matName)
        return *itr;
  }
  return NULL;
}

// Return instance of named material caller is responsible for memory
TSMaterialInstance *MetalTSMaterialManager::createMatInstance( const String &matName, const GFXVertexFormat *vertexFormat)
{
  TSMaterial *mat = getMaterialDefinitionByName(matName);
  if (mat)
  {
     return mat->createMatInstance(vertexFormat);
  }
  
  return NULL;
}

TSMaterialInstance *MetalTSMaterialManager::createFallbackMatInstance( const GFXVertexFormat *vertexFormat )
{
  if (!mDummyMaterial)
  {
     mDummyMaterial = (MetalTSMaterial*)allocateAndRegister("Dummy", "Dummy");
  }
  return mDummyMaterial->createMatInstance(vertexFormat);
}

void MetalTSMaterialManager::init(id<MTLDevice> &device, MTLPixelFormat backFormat)
{
   cleanup();
   
   mDevice = device;
   
   TSPipelineCollection* collection = new TSPipelineCollection("standard_fragment");
   collection->init(device);
   mPipelines["standard_fragment"] = collection;
}

void MetalTSMaterialManager::cleanup()
{
   for (std::unordered_map<const char*, TSPipelineCollection*>::iterator itr = mPipelines.begin(); itr != mPipelines.end(); itr++)
   {
      delete itr->second;
   }
   mPipelines.clear();
}


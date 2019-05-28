//
//  MetalRenderer.m
//  DTSTest
//
//  Created by James Urquhart on 06/05/2019.
//  Copyright © 2019 James Urquhart. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include "main.h"
#include "libdtshape.h"
#include "ts/tsMaterialManager.h"
#include "ts/tsShapeInstance.h"
#include "SDL.h"
#include "MetalTSMeshRenderer.h"
#include "MetalMaterial.h"
#include "MetalRenderer.h"

uint32_t MetalRenderer::smVBOAlignment;

SDL_Renderer* SetupMetalSDL(SDL_Window* window)
{
   int metalDriverIdx = -1;
   int drivers = SDL_GetNumRenderDrivers();
   for (int i=0; i<drivers; i++)
   {
      SDL_RendererInfo info;
      SDL_GetRenderDriverInfo(i, &info);
      
      if (strcasecmp(info.name, "metal") == 0)
      {
         metalDriverIdx = i;
      }
      //printf("Render driver[%i] == %s\n", i, info.name);
   }
   
   if (metalDriverIdx == -1)
   {
      fprintf(stderr, "Unable to find metal render driver for SDL\n");
      return NULL;
   }
   
   SDL_Renderer* renderer = SDL_CreateRenderer(window, metalDriverIdx, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
   
   SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
   SDL_RenderClear(renderer);
   SDL_RenderPresent(renderer);
   
   return renderer;
}

void MetalGetDrawableFormat(SDL_Renderer* renderer, MTLPixelFormat &pixelFormat, uint32_t &w, uint32_t &h)
{
   CAMetalLayer* layer = (__bridge CAMetalLayer*)(SDL_RenderGetMetalLayer(renderer));
   pixelFormat = layer.pixelFormat;
   w = layer.drawableSize.width;
   h = layer.drawableSize.height;
}

class DispatchSem : public SemBase
{
public:
   dispatch_semaphore_t value;
   
   DispatchSem(uint32_t count)
   {
      value = dispatch_semaphore_create(count);
   }
   
   ~DispatchSem()
   {
   }
   
   virtual bool Wait()
   {
      return dispatch_semaphore_wait(value, DISPATCH_TIME_FOREVER) == 0;
   }
   
   virtual void Signal()
   {
      dispatch_semaphore_signal(value);
   }
};

SemBase* SemBase::create(uint32_t count)
{
   return new DispatchSem(count);
}


MetalRenderer::MetalRenderer()
{
   mFrameSemaphore = NULL;
   mCurrentVBOffset = 0;
   mCurrentRenderEncoder = nil;
   mCurrentComputeEncoder = nil;
   
   mCurrentVB = nil;
   memset(mBufferList, '\0', sizeof(mBufferList));
}

MetalRenderer::~MetalRenderer()
{
   cleanup();
   if (mFrameSemaphore)
   {
      delete mFrameSemaphore;
   }
}

void MetalRenderer::cleanup()
{
   mDevice = nil;
   mCurrentCommandQueue = nil;
   mCurrentCommandBuffer = nil;
   mCurrentRenderEncoder = nil;
   mDepthBuffer = nil;
   mRenderPassDescriptor = nil;
}


void MetalRenderer::Init(SDL_Window* window, SDL_Renderer* renderer)
{
   mRenderer = renderer;
   mFrameSemaphore = SemBase::create(2);
   mDevice = nil;
   SDL_RenderGetLogicalSize(mRenderer, &mRenderDimensions.x, &mRenderDimensions.y);
   
   CAMetalLayer* layer = (__bridge CAMetalLayer*)(SDL_RenderGetMetalLayer(mRenderer));
   mDevice = layer.device;
   mColorFormat = layer.pixelFormat;
   MetalTSMeshRenderer::smDevice = mDevice;
   
   mCurrentCommandQueue = [mDevice newCommandQueueWithMaxCommandBufferCount:2];
   
   MetalTSMaterialManager* mmgr = static_cast<MetalTSMaterialManager*>(AppState::getInstance()->mMaterialManager);
   mmgr->init(mDevice, mColorFormat);
   
#if TARGET_OS_OSX
   smVBOAlignment = 256;
   
   if ( [mDevice supportsFeatureSet: MTLFeatureSet_OSX_GPUFamily1_v1] )
   {
      smVBOAlignment = 256;
   }
   
#elif TARGET_OS_IOS
   
   if ( [mDevice supportsFeatureSet: MTLFeatureSet_iOS_GPUFamily1_v1] ) {
      smVBOAlignment = 64;
   }
   
   if ( [mDevice supportsFeatureSet: MTLFeatureSet_iOS_GPUFamily3_v1] ) {
      smVBOAlignment = 16;
   }
#endif
   
   SetupBuffers();
}

bool MetalRenderer::BeginFrame()
{
   prepareFrameVBO(1024*1024*4, AppState::getInstance()->mFrame%2);
   
   if (!mFrameSemaphore->Wait())
   {
      return false;
   }

   // Link in swap chain image

   CAMetalLayer* layer = (__bridge CAMetalLayer*)(SDL_RenderGetMetalLayer(mRenderer));
   mCurrentDrawable = layer.nextDrawable;
   
   if (!mCurrentDrawable)
   {
      printf("SEM AVAIL BUT NO DRAWABLE!\n");
      mFrameSemaphore->Signal();
      return false;
   }
   
   mCurrentCommandBuffer = [mCurrentCommandQueue commandBuffer];
   
   return true;
}

void MetalRenderer::SetPushConstants(const PushConstants &constants)
{
   mRenderConstants = constants;
}

void MetalRenderer::EndFrame()
{
   [mCurrentCommandBuffer addCompletedHandler:^(id<MTLCommandBuffer> buffer)
    {
       mFrameSemaphore->Signal();
    }];
   
   if (mCurrentComputeEncoder)
   {
      [mCurrentComputeEncoder endEncoding];
   }
   else if (mCurrentRenderEncoder)
   {
      [mCurrentRenderEncoder endEncoding];
   }
   
   [mCurrentCommandBuffer presentDrawable:mCurrentDrawable];
   [mCurrentCommandBuffer commit];
   
   mCurrentDrawable = nil;
   mCurrentRenderEncoder = nil;
   mCurrentCommandBuffer = nil;
   mCurrentComputeEncoder = nil;
   MetalTSMeshRenderer::smCurrentRenderEncoder = nil;
}

void MetalRenderer::SetupBuffers()
{
   uint32_t color_w=0, color_h=0;
   MetalGetDrawableFormat(mRenderer, mColorFormat, color_w, color_h);
   
   MTLTextureDescriptor * depthDescriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float width:color_w height:color_h mipmapped:NO];
   depthDescriptor.usage = MTLTextureUsageUnknown;
   depthDescriptor.storageMode = MTLStorageModePrivate;
   depthDescriptor.resourceOptions = MTLResourceStorageModePrivate;
   mDepthBuffer = [mDevice newTextureWithDescriptor:depthDescriptor];
   
   mRenderPassDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
   
   mRenderPassDescriptor.colorAttachments[0].texture = nil;
   mRenderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
   mRenderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(0.0, 1.0, 1.0, 1.0);
   mRenderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
   
   mRenderPassDescriptor.depthAttachment.texture = mDepthBuffer;
   mRenderPassDescriptor.depthAttachment.loadAction = MTLLoadActionClear;
   mRenderPassDescriptor.depthAttachment.storeAction = MTLStoreActionDontCare;
   mRenderPassDescriptor.depthAttachment.clearDepth = 1.0;
   
   NSError* error = nil;
   id<MTLFunction> kernelFunction = [[mDevice newDefaultLibrary] newFunctionWithName:@"skin_verts_cs"];
   mSkinningPipelineState = [mDevice newComputePipelineStateWithFunction:kernelFunction error:&error];
}

void MetalRenderer::prepareFrameVBO(uint32_t size, uint32_t bufferID)
{
   if (!mBufferList[bufferID] || mBufferList[bufferID].length < size)
   {
      mBufferList[bufferID] = [mDevice newBufferWithLength:size options:MTLResourceCPUCacheModeWriteCombined];
      mSecondBufferList[bufferID] = [mDevice newBufferWithLength:size options:MTLResourceCPUCacheModeWriteCombined];
   }
   
   mCurrentVB = mBufferList[bufferID];
   mSecondCurrentVB = mSecondBufferList[bufferID];
   mCurrentVBOffset = 0;
   mSecondCurrentVBOffset = 0;
}

char* MetalRenderer::allocFrameVBO(uint32_t size, id<MTLBuffer> &outBuffer, uint32_t &outOffset)
{
   uint32_t length = mCurrentVB.length;
   if (mCurrentVBOffset + size > length)
   {
      assert(false);
   }
   
   char* newPtr = (char*)mCurrentVB.contents + mCurrentVBOffset;
   outOffset = mCurrentVBOffset;
   mCurrentVBOffset += size;
   outBuffer = mCurrentVB;
   
   // Align next offset to device alignment
   const uint32_t align_mod = mCurrentVBOffset % smVBOAlignment;
   uint32_t newVBOOffset = (align_mod == 0) ? mCurrentVBOffset : (mCurrentVBOffset + smVBOAlignment - align_mod);
   
   mCurrentVBOffset = newVBOOffset;
   
   return newPtr;
}

char* MetalRenderer::allocFrameVBO_Second(uint32_t size, id<MTLBuffer> &outBuffer, uint32_t &outOffset)
{
   uint32_t length = mSecondCurrentVB.length;
   if (mSecondCurrentVBOffset + size > length)
   {
      assert(false);
   }
   
   char* newPtr = (char*)mSecondCurrentVB.contents + mSecondCurrentVBOffset;
   outOffset = mSecondCurrentVBOffset;
   mSecondCurrentVBOffset += size;
   outBuffer = mSecondCurrentVB;
   
   // Align next offset to device alignment
   const uint32_t align_mod = mSecondCurrentVBOffset % smVBOAlignment;
   uint32_t newVBOOffset = (align_mod == 0) ? mSecondCurrentVBOffset : (mSecondCurrentVBOffset + smVBOAlignment - align_mod);
   
   mSecondCurrentVBOffset = newVBOOffset;
   
   return newPtr;
}



void MetalRenderer::DoComputeSkinning(DTShape::TSShapeInstance* shapeInst)
{
   if (mCurrentComputeEncoder == nil)
      return;
   
   // Get a list of buffers we need to update
   /*DTShape::TSShape* shape = shapeInst->getShape();
   for (TSMesh* mesh : shape->meshes)
   {
      mesh->createBatchData();
   }*/
   
   for (const DTShape::TSShapeInstance::MeshObjectInstance &inst : shapeInst->mMeshObjects)
   {
      MetalTSMeshInstanceRenderData* metalData = (MetalTSMeshInstanceRenderData*)inst.renderInstData;
      // Don't update if invisible or we're not skinned
      if (inst.visible <= 0.0f || !inst.lastMesh || inst.mActiveTransforms.size() == 0)
         continue;
      
      TSMesh* mesh = inst.lastMesh;
      MetalTSMeshRenderer* metalBase = (MetalTSMeshRenderer*)mesh->mRenderer;
      
      [mCurrentComputeEncoder setComputePipelineState:mSkinningPipelineState];
      // Params
      [mCurrentComputeEncoder setBytes:&metalData->mComputeParams length:sizeof(SkinParams) atIndex:0];
      // Input buffer
      [mCurrentComputeEncoder setBuffer:metalBase->mVB offset:0 atIndex:1];
      // Output buffer
      [mCurrentComputeEncoder setBuffer:metalData->mVB offset:metalData->mVBOffset atIndex:2];
      // Bones
      
      {
         MatrixF identity(1);
         uint32_t ofs=0;
         id<MTLBuffer> buffer;
         MatrixF* data = (MatrixF*)allocFrameVBO_Second(sizeof(MatrixF) * inst.mActiveTransforms.size(), buffer, ofs);
         
         for (int i=0; i<inst.mActiveTransforms.size(); i++)
         {
            MatrixF inM = inst.mActiveTransforms[i];
            inM.transpose();
            memcpy(&data[i], &inM ,sizeof(MatrixF));
         }
         
         //[buffer didModifyRange:NSMakeRange(ofs, sizeof(MatrixF) * inst.mActiveTransforms.size())];
         [mCurrentComputeEncoder setBuffer:buffer offset:ofs atIndex:3];
      }
      
      // Set the compute kernel's threadgroup size of 1x1
      MTLSize _threadgroupSize = MTLSizeMake(1, 1, 1);
      MTLSize _threadgroupCount = MTLSizeMake(metalBase->mNumVerts, 1, 1);
      
      [mCurrentComputeEncoder dispatchThreadgroups:_threadgroupCount
                     threadsPerThreadgroup:_threadgroupSize];
   }
}

bool MetalRenderer::BeginRasterPass()
{
   if (mCurrentComputeEncoder)
   {
      [mCurrentComputeEncoder endEncoding];
      mCurrentComputeEncoder = nil;
   }
   
   mRenderPassDescriptor.colorAttachments[0].texture = mCurrentDrawable.texture;
   
   {
      mCurrentRenderEncoder = [mCurrentCommandBuffer renderCommandEncoderWithDescriptor:mRenderPassDescriptor];
      MetalTSMeshRenderer::smCurrentRenderEncoder = mCurrentRenderEncoder;
      
      uint32_t color_w=0, color_h=0;
      MetalGetDrawableFormat(mRenderer, mColorFormat, color_w, color_h);
      
      MTLViewport viewport;
      viewport.width = color_w;
      viewport.height = color_h;
      viewport.originX = 0;
      viewport.originY = 0;
      viewport.znear = -1.0;
      viewport.zfar = 1.0;
      [mCurrentRenderEncoder setViewport:viewport];
   }
   
   return true;
}

bool MetalRenderer::BeginComputePass()
{
   if (mCurrentRenderEncoder != nil)
      return false;
   
   mCurrentComputeEncoder = [mCurrentCommandBuffer computeCommandEncoder];

   return true;
}

void MetalRenderer::EndComputePass()
{
   if (mCurrentComputeEncoder == nil)
      return;
   
   [mCurrentComputeEncoder endEncoding];
   mCurrentComputeEncoder = nil;
}

#if defined(USE_METAL) && !defined(USE_BOTH_RENDER)
void AppState::createRenderer()
{
   mRenderer = new MetalRenderer();
   mMaterialManager = new MetalTSMaterialManager();
}
#endif


//
//  MetalRenderer.h
//  DTSTest
//
//  Created by James Urquhart on 06/05/2019.
//  Copyright © 2019 James Urquhart. All rights reserved.
//

#ifndef MetalRenderer_h
#define MetalRenderer_h
#import <QuartzCore/CAMetalLayer.h>


class MetalRenderer : public BaseRenderer
{
public:
   
   MetalRenderer();
   
   ~MetalRenderer();
   
   void cleanup();
   
   
   void Init(SDL_Window* window, SDL_Renderer* renderer);
   
   virtual bool BeginFrame();
   
   virtual void SetPushConstants(const PushConstants &constants);
   
   virtual void EndFrame();
   
   void DoComputeSkinning(DTShape::TSShapeInstance* shape);
   bool BeginRasterPass();
   bool BeginComputePass();
   void EndComputePass();
   
   void SetupBuffers();
   
   void prepareFrameVBO(uint32_t size, uint32_t bufferID);
   char* allocFrameVBO(uint32_t size, id<MTLBuffer> &outBuffer, uint32_t &outOffset);
   char* allocFrameVBO_Second(uint32_t size, id<MTLBuffer> &outBuffer, uint32_t &outOffset);
   
   // State
   
   SDL_Renderer* mRenderer;
   
   id<CAMetalDrawable> mCurrentDrawable;
   id<MTLDevice> mDevice;
   id<MTLCommandQueue> mCurrentCommandQueue;
   id<MTLCommandBuffer> mCurrentCommandBuffer;
   id<MTLRenderCommandEncoder> mCurrentRenderEncoder;
   id<MTLComputeCommandEncoder> mCurrentComputeEncoder;
   id<MTLTexture> mDepthBuffer;
   id<MTLComputePipelineState> mSkinningPipelineState;
   
   MTLRenderPassDescriptor* mRenderPassDescriptor;
   
   MTLPixelFormat mColorFormat;
   
   SemBase* mFrameSemaphore;
   
   DTShape::Point2I mRenderDimensions;
   PushConstants mRenderConstants;
   
   id<MTLBuffer> mCurrentVB;
   id<MTLBuffer> mSecondCurrentVB;
   id<MTLBuffer> mBufferList[2];
   id<MTLBuffer> mSecondBufferList[2];
   
   static uint32_t smVBOAlignment;
   U32 mCurrentVBOffset;
   U32 mSecondCurrentVBOffset;
};

#endif /* MetalRenderer_h */

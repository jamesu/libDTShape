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

#ifndef GLRenderer_h
#define GLRenderer_h

class GLRenderer : public BaseRenderer
{
public:
   
   GLRenderer();
   
   ~GLRenderer();
   
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
   char* allocFrameVBO(uint32_t size, GLuint &outBuffer, uint32_t &outOffset);
   
   // State
   
   SDL_Renderer* mRenderer;
   SDL_Window* mWindow;
   
   DTShape::Point2I mRenderDimensions;
   PushConstants mRenderConstants;
   
   GLuint mCurrentVB;
   GLuint mBufferList[2];
   GLuint mBufferSize[2];
   
   static uint32_t smVBOAlignment;
   U32 mCurrentVBOffset;
   U32 mSecondCurrentVBOffset;
};

#endif /* GLRenderer_h */

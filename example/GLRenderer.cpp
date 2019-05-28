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

#include <stdlib.h>
#include <stdint.h>
#include "GLIncludes.h"

#include "main.h"
#include "libdtshape.h"
#include "ts/tsMaterialManager.h"
#include "ts/tsShapeInstance.h"
#include "SDL.h"
#include "GLTSMeshRenderer.h"
#include "GLMaterial.h"
#include "GLRenderer.h"

uint32_t GLRenderer::smVBOAlignment;

GLRenderer::GLRenderer()
{
   mCurrentVBOffset = 0;
   
   mCurrentVB = 0;
   memset(mBufferList, '\0', sizeof(mBufferList));
   memset(mBufferSize, '\0', sizeof(mBufferSize));
}

GLRenderer::~GLRenderer()
{
   cleanup();
}

void GLRenderer::cleanup()
{
   mCurrentVBOffset = 0;
   mCurrentVB = 0;
}


void GLRenderer::Init(SDL_Window* window, SDL_Renderer* renderer)
{
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
   SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
   SDL_GL_SetSwapInterval(1);
   
   SDL_GLContext mainContext = SDL_GL_CreateContext(window);
   SDL_GL_MakeCurrent(window, mainContext);
   
   if (gl3wInit()) {
      fprintf(stderr, "failed to initialize OpenGL\n");
      return;
   }
   printf("OpenGL %s, GLSL %s\n", glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION));
   
   SDL_GL_MakeCurrent(window, mainContext);
   
   mRenderer = renderer;
   mWindow = window;
   
   SDL_GetWindowSize(mWindow, &mRenderDimensions.x, &mRenderDimensions.y);
   
   GLTSMaterialManager* mmgr = static_cast<GLTSMaterialManager*>(AppState::getInstance()->mMaterialManager);
   mmgr->init();
   
   smVBOAlignment = 256;
   
   SetupBuffers();
}

bool GLRenderer::BeginFrame()
{
   prepareFrameVBO(1024*1024*4, AppState::getInstance()->mFrame%2);
   
   return true;
}

void GLRenderer::SetPushConstants(const PushConstants &constants)
{
   mRenderConstants = constants;
}

void GLRenderer::EndFrame()
{
   SDL_GL_SwapWindow(mWindow);
}

void GLRenderer::SetupBuffers()
{
   // setup already by SDL
   glGenBuffers(2, &mBufferList[0]);
   
   GLuint vao = 0;
   glGenVertexArrays(1, &vao);
   glBindVertexArray(vao);
}

void GLRenderer::prepareFrameVBO(uint32_t size, uint32_t bufferID)
{
   if (mBufferSize[bufferID] < size)
   {
      glBindBuffer(GL_ARRAY_BUFFER, mBufferList[bufferID]);
      glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_DYNAMIC_DRAW);
      mBufferSize[bufferID] = size;
   }
   
   mCurrentVB = mBufferList[bufferID];
   mCurrentVBOffset = 0;
}

char* GLRenderer::allocFrameVBO(uint32_t size, GLuint &outBuffer, uint32_t &outOffset)
{
   uint32_t length = mBufferSize[0];
   if (mCurrentVBOffset + size > length)
   {
      assert(false);
   }
   
   glBindBuffer(GL_ARRAY_BUFFER, mCurrentVB);
   char* newPtr = ((char*)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE));
   if (newPtr == NULL)
   {
      assert(false);
   }
   newPtr += mCurrentVBOffset;
   
   outOffset = mCurrentVBOffset;
   mCurrentVBOffset += size;
   outBuffer = mCurrentVB;
   
   // Align next offset to device alignment
   const uint32_t align_mod = mCurrentVBOffset % smVBOAlignment;
   uint32_t newVBOOffset = (align_mod == 0) ? mCurrentVBOffset : (mCurrentVBOffset + smVBOAlignment - align_mod);
   
   mCurrentVBOffset = newVBOOffset;
   
   return newPtr;
}

void GLRenderer::DoComputeSkinning(DTShape::TSShapeInstance* shapeInst)
{
   // TODO
}

bool GLRenderer::BeginRasterPass()
{
   int w,h;
   SDL_GetWindowSize(mWindow, &w, &h);
   
   glViewport(0, 0, w, h);
   glClearColor(0,1,1,1);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   
   return true;
}

bool GLRenderer::BeginComputePass()
{
   return false;
}

void GLRenderer::EndComputePass()
{
}

#if !defined(USE_METAL) && !defined(USE_BOTH_RENDER)
void AppState::createRenderer()
{
   mRenderer = new GLRenderer();
   mMaterialManager = new GLTSMaterialManager();
}
#endif


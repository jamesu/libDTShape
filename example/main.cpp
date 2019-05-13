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

#include "platform/platform.h"

#define GL_GLEXT_PROTOTYPES

#include "SDL.h"
#include <stdio.h>

#include "libdtshape.h"

#include "ts/tsShape.h"
#include "ts/tsShapeInstance.h"
#include "ts/tsRenderState.h"
#include "ts/tsMaterialManager.h"
#include "core/color.h"
#include "math/mMath.h"
#include "math/util/frustum.h"
#include "core/stream/fileStream.h"


extern "C"
{
#include "soil/SOIL.h"
}


#include "main.h"

using namespace DTShape;

// Generic interface which provides scene info to the rendering code
class MetalTSSceneRenderState : public TSSceneRenderState
{
public:
   MatrixF mWorldMatrix;
   MatrixF mViewMatrix;
   MatrixF mProjectionMatrix;
   
   MetalTSSceneRenderState()
   {
      mWorldMatrix = MatrixF(1);
      mViewMatrix = MatrixF(1);
      mProjectionMatrix = MatrixF(1);
   }
   
   ~MetalTSSceneRenderState()
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

#define TICK_TIME 15

const char* GetAssetPath(const char *file);

// DTS Sequences for sample shape

const char* sTSAppSequenceNames[] = {
   "Root",
   "Forward",
   "Crouch_Backward",
   "Side",
   "Jump",
   "invalid"
};

AnimSequenceInfo sAppSequenceInfos[] = {
   {50, 109, true},
   {150, 169, true},
   {460, 489, true},
   {200, 219, true},
   {1000, 1010, false}
};

AppState *AppState::sInstance = NULL;

using namespace DTShape;

TSMaterialManager *TSMaterialManager::instance()
{
   return AppState::getInstance()->mMaterialManager;
}

AppState::AppState()
{
   running = false;
   mWindow = NULL;
   dStrcpy(shapeToLoad, "player.dts");//cube.dae");
   sInstance = this;
   
   sShape = NULL;
   sShapeInstance = NULL;
   
   sThread = NULL;
   sCurrentSequenceIdx = 0;
   for (int i=0; i<kNumTSAppSequences; i++)
      sSequences[i] = -1;
      
   sRot = 180.0f;
   sDeltaRot = 0.0f;
   sCameraPosition = Point3F(0,-10,1);
   sModelPosition = Point3F(0,0,0);
   deltaCameraPos = Point3F(0,0,0);
   
   sOldTicks = 0;
   mFrame = 0;
   sRenderState = NULL;
}

AppState::~AppState()
{
   if (sRenderState)
      delete sRenderState;
   if (mMaterialManager)
      delete mMaterialManager;
   CleanupShape();
   
   if (mRenderer)
      delete mRenderer;
   
   if (mWindow)
   {
      SDL_DestroyRenderer(mSDLRenderer);
      SDL_DestroyWindow(mWindow);
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
            if (sSequences[sCurrentSequenceIdx] != -1)
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
            if (sSequences[sCurrentSequenceIdx] != -1)
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
   SetupProjection();
   
   
   if (mRenderer->BeginFrame())
   {
      sRenderState->reset();
      PrepShape(deltaTime);
      ComputeShapeMeshes(); // mesh info is setup in PrepShape
      
      mRenderer->BeginRasterPass();
      DrawShapes();
      mRenderer->EndFrame();
   }
   
   // Slow down if we're going too fast
   U32 tickEnd = SDL_GetTicks();
   deltaTick = (tickEnd - tickTime);
   if (deltaTick < TICK_TIME)
      SDL_Delay(TICK_TIME - deltaTick);
   
   mFrame++;
}

bool AppState::LoadShape()
{
   TSMaterialManager::instance()->allocateAndRegister("Soldier_Dif");
   TSMaterialManager::instance()->allocateAndRegister("Soldier_Dazzle");
   TSMaterialManager::instance()->mapMaterial("base_Soldier_Main", "Soldier_Dif");
   TSMaterialManager::instance()->mapMaterial("base_Soldier_Dazzle", "Soldier_Dazzle");
   //TSMaterialManager::instance()->mapMaterial("player_blue", "Soldier_Dazzle");
   
   
   TSMaterial *cubeMat = TSMaterialManager::instance()->allocateAndRegister("cube");
   TSMaterialManager::instance()->mapMaterial("Cube", "cube");
   
   TSMaterialManager::instance()->allocateAndRegister("player");
   TSMaterialManager::instance()->mapMaterial("Player", "player");
   
   const char* fullPath = GetAssetPath(shapeToLoad);
   sShape = TSShape::createFromPath(fullPath);
   
   if (!sShape)
   {
      return false;
   }
   
   sShapeInstance = new TSShapeInstance(sShape, sRenderState, true);
   
   // Load all dsq files
   for (int i=0; i<kNumTSAppSequences; i++)
   {
      FileStream dsqFile;
      char pathName[64];
      dSprintf(pathName, 64, "player_%s.dsq", sTSAppSequenceNames[i]);
      Log::printf("Attempting to load sequence file %s", pathName);
      
      if (dsqFile.open(GetAssetPath(pathName), FileStream::Read) && sShape->importSequences(&dsqFile, ""))
      {
         Log::printf("Sequence file %s loaded", pathName);
      }
      if (sShape->addSequence(GetAssetPath(pathName), "", sTSAppSequenceNames[i], sAppSequenceInfos[i].start, sAppSequenceInfos[i].end, true, false))
      {
         Log::printf("Sequence file %s loaded", pathName);
      }
   }
   
   // Resolve all sequences
   for (int i=0; i<kNumTSAppSequences; i++)
   {
      sSequences[i] = sShape->findSequence(sTSAppSequenceNames[i]);
      if (sSequences[i] != -1)
      {
         if (sAppSequenceInfos[i].cyclic)
            sShape->sequences[sSequences[i]].flags |= TSShape::Cyclic;
         else
            sShape->sequences[sSequences[i]].flags &= ~TSShape::Cyclic;
      }
   }
   
   sThread = sShapeInstance->addThread();
   if (sSequences[kTSRootAnim] != -1)
   {
      sShapeInstance->setSequence(sThread, sSequences[kTSForwardAnim], 0);
      sShapeInstance->setTimeScale(sThread, 1.0f);
   }
   
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

void AppState::PrepShape(F32 dt)
{
   MetalTSSceneRenderState *dummyScene = sRenderState->allocCustom<MetalTSSceneRenderState>();
   constructInPlace(dummyScene);
   
   sCameraPosition += deltaCameraPos * dt;
   sRot += sDeltaRot * dt;
   
   MatrixF calc(1);
   
   // Set render transform
   dummyScene->mProjectionMatrix = sProjectionMatrix;
   dummyScene->mViewMatrix = MatrixF(1);
   dummyScene->mWorldMatrix = MatrixF(1);
   dummyScene->mWorldMatrix.setPosition(sModelPosition);
   
   // Apply model rotation
   calc = MatrixF(1);
   AngAxisF rot2 = AngAxisF(Point3F(0,0,1), mDegToRad(sRot));
   rot2.setMatrix(&calc);
   dummyScene->mWorldMatrix *= calc;
   
   // Set camera position
   calc = MatrixF(1);
   calc.setPosition(sCameraPosition);
   dummyScene->mViewMatrix *= calc;
   
   // Rotate camera
   rot2 = AngAxisF(Point3F(1,0,0), mDegToRad(-90.0f));
   calc = MatrixF(1);
   rot2.setMatrix(&calc);
   dummyScene->mViewMatrix *= calc;
   
   // Calculate lighting vector
   mLightPos = sCameraPosition;
   
   // Set base shader uniforms
   setLightPosition(mLightPos);
   setLightColor(ColorF(1,1,1,1));
   
   mRenderer->SetPushConstants(renderConstants);
   
   // Now we can render
   sRenderState->setSceneState(dummyScene);
   
   // Animate & render shape to the renderState
   sShapeInstance->beginUpdate(sRenderState);
   sShapeInstance->advanceTime(dt);
   sShapeInstance->setCurrentDetail(0);
   sShapeInstance->animateNodeSubtrees(true);
   sShapeInstance->animate();
   sShapeInstance->render(*sRenderState);
}

void AppState::ComputeShapeMeshes()
{
   if (!TSShape::smUseComputeSkinning)
      return;
   
   mRenderer->BeginComputePass();
   mRenderer->DoComputeSkinning(sShapeInstance);
   mRenderer->EndComputePass();
}

void AppState::DrawShapes()
{
   // Ensure generated primitives are in the right z-order
   sRenderState->sortRenderInsts();
   
   // Render solid stuff
   for (int i=0; i<sRenderState->mRenderInsts.size(); i++)
   {
      sRenderState->mRenderInsts[i]->render(sRenderState);
   }
   
   // Render translucent stuff
   /*for (int i=0; i<sRenderState->mTranslucentRenderInsts.size(); i++)
    {
    // Update transform, & render buffers
    sRenderState->mTranslucentRenderInsts[i]->render(sRenderState);
    }*/
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

SDL_Renderer* SetupMetalSDL(SDL_Window* window);

int AppState::main(int argc, char **argv)
{
   DTShapeInit::init();
   
   Log::addConsumer(OnAppLog);
   
   if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
      Log::errorf("Couldn't initialize SDL: %s\n", SDL_GetError());
      return (1);
   }
   
   int screenW = 800;
   int screenH = 600;
   SDL_DisplayMode mode;
   
   int windowFlags = 0;
   
   windowFlags = SDL_WINDOW_RESIZABLE;
   
   SDL_GetCurrentDisplayMode(0, &mode);
   if (mode.w < screenW)
	   screenW = mode.w;
   if (mode.h < screenH)
	   screenH = mode.h;
   
   mWindow = SDL_CreateWindow("libDTShape Example", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screenW, screenH, windowFlags);
   mSDLRenderer = SetupMetalSDL(mWindow);
   
   SDL_ShowWindow(mWindow);
   
   createRenderer();
   mRenderer->Init(mSDLRenderer);

   // Init render state
   sRenderState = new TSRenderState();
   
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
      default:
         break;
   }
}

void AppState::setLightPosition(const DTShape::Point3F &pos)
{
   renderConstants.lightPos = simd_make_float3(pos.x, pos.y, pos.z);
}

void AppState::setLightColor(const DTShape::ColorF &color)
{
   renderConstants.lightColor = simd_make_float3(color.red, color.green, color.blue);
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

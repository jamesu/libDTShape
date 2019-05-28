#ifndef _APPSTATE_H_
#define _APPSTATE_H_

#ifndef _CORE_LOG_H_
#include "core/log.h"
#endif

#ifndef _MMATH_H_
#include "math/mMath.h"
#endif

#ifndef _COLOR_H_
#include "core/color.h"
#endif

#include "SDL.h"
#include "mtl_shared.h"

class MetalSimpleShader;
class MetalTSMaterial;

BEGIN_NS(DTShape)
class TSShape;
class TSShapeInstance;
class TSThread;
class TSRenderState;
class TSMaterialManager;
END_NS

enum TSAppSequences {
   kTSRootAnim,
   kTSForwardAnim,
   kTSBackAnim,
   kTSSideAnim,
   kTSJumpAnim,
   
   kNumTSAppSequences
};

class SemBase
{
public:
   virtual ~SemBase() {;}
   virtual bool Wait() = 0;
   virtual void Signal() = 0;
   
   static SemBase* create(uint32_t count);
};

typedef struct AnimSequenceInfo {
   int start;
   int end;
   bool cyclic;
} AnimSequenceInfo;

// Base class for renderer
class BaseRenderer
{
public:
   virtual ~BaseRenderer() {;}
   
   virtual void Init(SDL_Window* window, SDL_Renderer* renderer) = 0;
   
   virtual bool BeginFrame() = 0;
   virtual void EndFrame() = 0;
   virtual void SetPushConstants(const PushConstants &constants) = 0;
   
   virtual bool BeginRasterPass() = 0;
   virtual bool BeginComputePass() = 0;
   virtual void EndComputePass() = 0;
   virtual void DoComputeSkinning(DTShape::TSShapeInstance* shape) = 0;
};

class AppState
{
public:
   SDL_Window *mWindow;
   SDL_Renderer* mSDLRenderer;
   BaseRenderer *mRenderer;
   
   bool running;
   
   char shapeToLoad[256];
   
   static AppState *sInstance;
   
   DTShape::TSShape *sShape;
   DTShape::TSShapeInstance *sShapeInstance;
   DTShape::TSThread *sThread;
   
   DTShape::TSRenderState *sRenderState;
   
   U32 sCurrentSequenceIdx;
   S32 sSequences[kNumTSAppSequences];
   
   DTShape::TSMaterialManager *mMaterialManager;
   
   PushConstants renderConstants;
   
   float sRot;
   float sDeltaRot;
   DTShape::Point3F sCameraPosition;
   DTShape::Point3F sModelPosition;
   DTShape::MatrixF sProjectionMatrix;
   
   DTShape::Point3F deltaCameraPos;
   DTShape::Point3F mLightPos;
   
   uint32_t mFrame;
   U32 sOldTicks;
   
   AppState();
   ~AppState();
   
   void createRenderer();
   
   int main(int argc, char **argv);
   void mainLoop();
   
   // Input handler
   void onKeyChanged(int key, int state);
   
   // Loads sample shape
   bool LoadShape();
   
   // Unloads shape
   void CleanupShape();
   
   // Transitions to a new sequence
   void SwitchToSequence(U32 seqIdx, F32 transitionTime, bool doTransition);
   
   // Sets up projection matrix
   void SetupProjection();
   
   void PrepShape(F32 dt);
   void ComputeShapeMeshes();
   void DrawShapes();
   
   void setLightPosition(const DTShape::Point3F &pos);
   void setLightColor(const DTShape::ColorF &color);
   static void OnAppLog(U32 level, DTShape::LogEntry *logEntry);
   
   static AppState *getInstance();
   
   
};

#endif

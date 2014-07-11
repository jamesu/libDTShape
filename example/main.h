#ifndef _APPSTATE_H_
#define _APPSTATE_H_

#ifndef _CORE_LOG_H_
#include "core/log.h"
#endif

#ifndef _MMATH_H_
#include "math/mMath.h"
#endif

class GLSimpleShader;
class GLTSMaterial;
class GLTSMaterialManager;

BEGIN_NS(DTShape)
class TSShape;
class TSShapeInstance;
END_NS

enum TSAppSequences {
   kTSRootAnim,
   kTSForwardAnim,
   kTSBackAnim,
   kTSSideAnim,
   kTSJumpAnim,
   
   kNumTSAppSequences
};

typedef struct AnimSequenceInfo {
   int start;
   int end;
   bool cyclic;
} AnimSequenceInfo;

class AppState
{
public:
   SDL_Window *window;
   SDL_GLContext glcontext;
   
   bool running;
   
   char shapeToLoad[256];
   
   static AppState *sInstance;
   
   DTShape::TSShape *sShape;
   DTShape::TSShapeInstance *sShapeInstance;
   DTShape::TSThread *sThread;
   
   DTShape::TSRenderState *sRenderState;
   
   U32 sCurrentSequenceIdx;
   S32 sSequences[kNumTSAppSequences];
   
   GLTSMaterialManager *sMaterialManager;
   
   GLSimpleShader *mCurrentShader;
   GLSimpleShader *sShader;
   GLSimpleShader *sSkinningShader;
   
   float sRot;
   float sDeltaRot;
   DTShape::Point3F sCameraPosition;
   DTShape::Point3F sModelPosition;
   DTShape::MatrixF sProjectionMatrix;
   
   DTShape::Point3F deltaCameraPos;
   
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
   
   static void OnAppLog(U32 level, DTShape::LogEntry *logEntry);
   static AppState *getInstance();
};

#endif

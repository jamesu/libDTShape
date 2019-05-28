//
//  GLMetalRenderHooks.cpp
//  DTSTest
//
//  Created by James Urquhart on 27/05/2019.
//  Copyright © 2019 James Urquhart. All rights reserved.
//

#include <stdio.h>

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


extern bool gUseMetal;

#if defined(USE_BOTH_RENDER)
void AppState::createRenderer()
{
   if (gUseMetal)
   {
      mRenderer = new MetalRenderer();
      mMaterialManager = new MetalTSMaterialManager();
   }
   else
   {
      mRenderer = new GLRenderer();
      mMaterialManager = new GLTSMaterialManager();
   }
}

TSMeshRenderer *TSMeshRenderer::create()
{
   if (gUseMetal)
   {
      return new MetalTSMeshRenderer();
   }
   else
   {
      return new GLTSMeshRenderer();
   }
}

TSMeshInstanceRenderData *TSMeshInstanceRenderData::create()
{
   if (gUseMetal)
   {
      return new MetalTSMeshInstanceRenderData();
   }
   else
   {
      return new GLTSMeshInstanceRenderData();
   }
}

#endif




#ifndef _GLINCLUDES_H_
#define _GLINCLUDES_H_

#include <GL/gl3w.h>

#ifdef HAVE_OPENGLES2
#define GL_GLEXT_PROTOTYPES
#include "SDL_opengles2.h"
#else
#include "SDL_opengl.h"
#endif

#endif

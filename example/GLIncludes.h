#ifndef _GLINCLUDES_H_
#define _GLINCLUDES_H_

#ifdef HAVE_OPENGLES2
#define GL_GLEXT_PROTOTYPES
#include "SDL_opengles2.h"
#define GL_CLAMP GL_CLAMP_TO_EDGE
#define glMapBuffer glMapBufferOES
#define glUnmapBuffer glUnmapBufferOES
#define GL_WRITE_ONLY GL_WRITE_ONLY_OES
#else
#include <GL/glew.h>
#endif

#endif

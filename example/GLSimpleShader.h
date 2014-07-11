#ifndef _GLSIMPLESHADER_H_
#define _GLSIMPLESHADER_H_

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

#include "GLIncludes.h"

#include "math/mMath.h"
#include "core/color.h"

using namespace DTShape;

// Basic shader code
enum {
   kGLSimpleVertexAttrib_Position,
   kGLSimpleVertexAttrib_Color,
   kGLSimpleVertexAttrib_TexCoords,
   kGLSimpleVertexAttrib_Normal,
   kGLSimpleVertexAttrib_BlendIndices,
   kGLSimpleVertexAttrib_BlendWeights,
   
   kGLSimpleVertexAttrib_MAX,
};

/** vertex attrib flags */
enum {
   kGLSimpleVertexAttribFlag_None        = 0,
   
   kGLSimpleVertexAttribFlag_Position    = 1 << 0,
   kGLSimpleVertexAttribFlag_Color        = 1 << 1,
   kGLSimpleVertexAttribFlag_TexCoords    = 1 << 2,
   kGLSimpleVertexAttribFlag_Normal       = 1 << 3,

   kGLSimpleVertexAttribFlag_BlendIndices = 1 << 4,
   kGLSimpleVertexAttribFlag_BlendWeights = 1 << 5,
   
   kGLSimpleVertexAttribFlag_PosColorTexNormal = ( kGLSimpleVertexAttribFlag_Position | kGLSimpleVertexAttribFlag_Color | kGLSimpleVertexAttribFlag_TexCoords | kGLSimpleVertexAttribFlag_Normal ),
};

enum {
   kGLSimpleUniformMVPMatrix,
   kGLSimpleUniformMVMatrix,
   kGLSimpleUniformLightPos,
   kGLSimpleUniformLightColor,
   kGLSimpleUniformSampler,

   kGLSimpleUniformBoneTransforms,
   
   kGLSimpleUniform_MAX,
};

// Basic GL state tracker
class GLStateTracker
{
public:
   GLStateTracker() :
   mPositionEnabled(false),
   mColorEnabled(false),
   mTexCoordEnabled(false),
   mNormalEnabled(false),
   mWeightsEnabled(false),
   mIndicesEnabled(false)
   {
   }
   
   void setVertexAttribs(unsigned int flags)
   {
      bool posOn = flags & kGLSimpleVertexAttribFlag_Position;
      bool colorOn = flags & kGLSimpleVertexAttribFlag_Color;
      bool texOn = flags & kGLSimpleVertexAttribFlag_TexCoords;
      bool normalOn = flags & kGLSimpleVertexAttribFlag_Normal;
      bool blendWeightsOn = flags & kGLSimpleVertexAttribFlag_BlendWeights;
      bool blendIndicesOn = flags & kGLSimpleVertexAttribFlag_BlendIndices;
      
      if (posOn != mPositionEnabled)
      {
         if (posOn)
            glEnableVertexAttribArray(kGLSimpleVertexAttrib_Position);
         else
            glDisableVertexAttribArray(kGLSimpleVertexAttrib_Position);
         mPositionEnabled = posOn;
      }
      
      if (colorOn != mColorEnabled)
      {
         if (colorOn)
            glEnableVertexAttribArray(kGLSimpleVertexAttrib_Color);
         else
            glDisableVertexAttribArray(kGLSimpleVertexAttrib_Color);
         mColorEnabled = colorOn;
      }
      
      if (texOn != mTexCoordEnabled)
      {
         if (texOn)
            glEnableVertexAttribArray(kGLSimpleVertexAttrib_TexCoords);
         else
            glDisableVertexAttribArray(kGLSimpleVertexAttrib_TexCoords);
         mTexCoordEnabled = texOn;
      }
      
      if (normalOn != mNormalEnabled)
      {
         if (texOn)
            glEnableVertexAttribArray(kGLSimpleVertexAttrib_Normal);
         else
            glDisableVertexAttribArray(kGLSimpleVertexAttrib_Normal);
         mNormalEnabled = normalOn;
      }
      
      if (blendWeightsOn != mWeightsEnabled)
      {
         if (blendWeightsOn)
            glEnableVertexAttribArray(kGLSimpleVertexAttrib_BlendWeights);
         else
            glDisableVertexAttribArray(kGLSimpleVertexAttrib_BlendWeights);
         mWeightsEnabled = blendWeightsOn;
      }
      
      if (blendIndicesOn != mIndicesEnabled)
      {
         if (blendIndicesOn)
            glEnableVertexAttribArray(kGLSimpleVertexAttrib_BlendIndices);
         else
            glDisableVertexAttribArray(kGLSimpleVertexAttrib_BlendIndices);
         mIndicesEnabled = blendIndicesOn;
      }
   }
   
   bool mPositionEnabled;
   bool mColorEnabled;
   bool mTexCoordEnabled;
   bool mNormalEnabled;
   bool mWeightsEnabled;
   bool mIndicesEnabled;
};

// Simple Shader compiler
class GLSimpleShader
{
public:
   GLSimpleShader(const char *vert, const char *fragment);
   ~GLSimpleShader();
   
   GLuint linkProgram(GLuint *shaders);
   
   GLuint compile(GLenum shaderType, const char *data);
   
   void use();
   
   void setLightPosition(Point3F pos);
   void setLightColor(ColorF col);
   void setProjectionMatrix(MatrixF &proj);
   void setModelViewMatrix(MatrixF &modelview);
   
   void updateBoneTransforms(U32 numTransforms, MatrixF *transformList);
   void updateTransforms();
   
   MatrixF mProjectionMatrix;
   MatrixF mModelViewMatrix;
   
   F32 mLightPos[3];
   F32 mLightColor[3];
   
   GLuint            mProgram;
   GLint             mUniforms[kGLSimpleUniform_MAX];

   static const char *sStandardVertexProgram;
   static const char *sSkinnedVertexProgram;
   static const char *sStandardFragmentProgram;
};

#endif

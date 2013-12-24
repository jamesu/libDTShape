#ifndef _GLSIMPLESHADER_H_
#define _GLSIMPLESHADER_H_

// Basic shader code
enum {
   kGLSimpleVertexAttrib_Position,
   kGLSimpleVertexAttrib_Color,
   kGLSimpleVertexAttrib_TexCoords,
   kGLSimpleVertexAttrib_Normal,
   
   kGLSimpleVertexAttrib_MAX,
};

/** vertex attrib flags */
enum {
   kGLSimpleVertexAttribFlag_None        = 0,
   
   kGLSimpleVertexAttribFlag_Position    = 1 << 0,
   kGLSimpleVertexAttribFlag_Color        = 1 << 1,
   kGLSimpleVertexAttribFlag_TexCoords    = 1 << 2,
   kGLSimpleVertexAttribFlag_Normal       = 1 << 3,
   
   kGLSimpleVertexAttribFlag_PosColorTexNormal = ( kGLSimpleVertexAttribFlag_Position | kGLSimpleVertexAttribFlag_Color | kGLSimpleVertexAttribFlag_TexCoords | kGLSimpleVertexAttribFlag_Normal ),
};

enum {
   kGLSimpleUniformMVPMatrix,
   kGLSimpleUniformMVMatrix,
   kGLSimpleUniformLightPos,
   kGLSimpleUniformLightColor,
   kGLSimpleUniformSampler,
   
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
   mNormalEnabled(false)
   {
   }
   
   void setVertexAttribs(unsigned int flags)
   {
      bool posOn = flags & kGLSimpleVertexAttribFlag_Position;
      bool colorOn = flags & kGLSimpleVertexAttribFlag_Color;
      bool texOn = flags & kGLSimpleVertexAttribFlag_TexCoords;
      bool normalOn = flags & kGLSimpleVertexAttribFlag_Normal;
      
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
   }
   
   bool mPositionEnabled;
   bool mColorEnabled;
   bool mTexCoordEnabled;
   bool mNormalEnabled;
};

// Simple Shader compiler
class GLSimpleShader
{
public:
   GLSimpleShader();
   ~GLSimpleShader();
   
   GLuint linkProgram(GLuint *shaders);
   
   GLuint compile(GLenum shaderType, const char *data);
   
   void use();
   
   void setLightPosition(Point3F pos);
   void setLightColor(ColorF col);
   void setProjectionMatrix(MatrixF &proj);
   void setModelViewMatrix(MatrixF &modelview);
   
   void updateTransforms();
   
   MatrixF mProjectionMatrix;
   MatrixF mModelViewMatrix;
   
   F32 mLightPos[3];
   F32 mLightColor[3];
   
   GLuint            mProgram;
   GLint             mUniforms[kGLSimpleUniform_MAX];
};

#endif

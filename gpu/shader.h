#ifndef _SHADER_H_
#define _SHADER_H_

#include <GL/glew.h>
#include <GL/glut.h>

void printShaderInfoLog( GLuint obj );
void printProgramInfoLog( GLuint obj );
GLuint CreateShader( const char* shaderPath, GLenum shaderType );
GLuint CreateProgram( GLuint vertID, GLuint fragID );
void DestroyProgram( GLuint progID, GLuint vertID, GLuint fragID );

#endif

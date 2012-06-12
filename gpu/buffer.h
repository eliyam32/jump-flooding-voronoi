#ifndef _BUFFER_H_
#define _BUFFER_H_

#include <iostream>

#include <GL/glew.h>
#include <GL/glut.h>

bool checkFramebufferStatus();
std::string convertInternalFormatToString(GLenum format);
std::string getTextureParameters(GLuint id);
std::string getRenderbufferParameters(GLuint id);
void printFramebufferInfo();

#endif

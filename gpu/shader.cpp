#include <stdio.h>

#include "shader.h"
#include "textfile.h"

void printShaderInfoLog( GLuint obj )
{
	int infologLength = 0;
	int charsWritten  = 0;
	char *infoLog;

	glGetShaderiv(obj, GL_INFO_LOG_LENGTH, &infologLength);

	if (infologLength > 1)
	{
		infoLog = (char*)malloc(infologLength);
		glGetShaderInfoLog(obj, infologLength, &charsWritten, infoLog);
		printf("%s\n",infoLog);
		free(infoLog);
	}
}

void printProgramInfoLog( GLuint obj )
{
	int infologLength = 0;
	int charsWritten  = 0;
	char *infoLog;

	glGetProgramiv(obj, GL_INFO_LOG_LENGTH, &infologLength);

	if (infologLength > 1)
	{
		infoLog = (char*)malloc(infologLength);
		glGetProgramInfoLog(obj, infologLength, &charsWritten, infoLog);
		printf("%s\n",infoLog);
		free(infoLog);
	}
}

GLuint CreateShader( const char* shaderPath, GLenum shaderType )
{
	GLuint shaderID = glCreateShader( shaderType );

	if( shaderID > 0 ) {
		char* shaderSource = textFileRead( shaderPath );
		const char* shaderSrc = shaderSource;
		glShaderSource( shaderID, 1, &shaderSrc, NULL );
		free( shaderSource );
		glCompileShader( shaderID );
		printShaderInfoLog( shaderID );
	}

	return shaderID;
}

GLuint CreateProgram( GLuint vertID, GLuint fragID )
{
	GLuint progID = glCreateProgram();
	glAttachShader( progID, vertID );
	glAttachShader( progID, fragID );
	glLinkProgram( progID );
	printProgramInfoLog( progID );
	return progID;
}

void DestroyProgram( GLuint progID, GLuint vertID, GLuint fragID ) {

	glDetachShader( progID, vertID );
	glDetachShader( progID, fragID );
	glDeleteShader( vertID );
	glDeleteShader( fragID );
	glDeleteProgram( progID );

}

/*=================================================================================================
  Author: Renato Farias (renatomdf@gmail.com)
  Created on: June 12th, 2012
  Purpose: A GPU implementation of the Jump Flooding algorithm from the paper "Jump Flooding in
   GPU With Applications to Voronoi Diagram and Distance Transform" [Rong 2006]. The result is
   a Voronoi diagram generated from randomly placed seeds.
=================================================================================================*/

/*=================================================================================================
  INCLUDES
=================================================================================================*/

#include <GL/glew.h>
#include <GL/glut.h>

#include <assert.h>
#include <stdio.h>
#include <vector>

#include "shader.h"
#include "buffer.h"
#include "rfUtil.h"

using namespace std;

/*=================================================================================================
  DEFINES
=================================================================================================*/

// Initial window dimensions
#define INIT_WINDOW_WIDTH  512
#define INIT_WINDOW_HEIGHT 512

// Initial screen position
#define INIT_WINDOW_POS_X 0
#define INIT_WINDOW_POS_Y 0

/*=================================================================================================
  STRUCTS
=================================================================================================*/

// A seed with (x,y) coordinates
typedef struct {
	float x,y;
} Seed;

/*=================================================================================================
  GLOBALS
=================================================================================================*/

// Current window dimensions
int WindowWidth  = INIT_WINDOW_WIDTH;
int WindowHeight = INIT_WINDOW_HEIGHT;

// List of seeds
vector<Seed> Seeds;

// How large to draw each seed, used with glPointSize()
int SeedSize = 10;

// Buffer dimensions
int BufferWidth  = INIT_WINDOW_WIDTH;
int BufferHeight = INIT_WINDOW_HEIGHT;

// Which buffer are we reading from?
bool ReadingBufferA = true;

// Is the window currently fullscreen?
bool FullScreen = false;

// Shaders
#define numShaders 3
enum ShaderEnum {
	CPOS_SHADER = 0,
	JUMP_SHADER,
	TEXTURE_SHADER
};
GLuint vertID[ numShaders ], fragID[ numShaders ], progID[ numShaders ];

// Render to texture
GLuint framebufferId, renderbufferId, textureId[2];
GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
int curTexture = 1;

// Pop-up menu
int MenuId;
enum MenuEntries {
	ENTRY_QUIT = 0,
	ENTRY_FULLSCREEN_ENTER,
	ENTRY_FULLSCREEN_LEAVE,
	ENTRY_GENERATE_SEEDS
};

/*=================================================================================================
  FUNCTIONS
=================================================================================================*/

// Create the framebuffer object and its components
void CreateFBO( void ) {

	// Create a texture object
	printf( "Creating texture object. " );
	glGenTextures( 2, &textureId[0] );
	for( int i = 0; i < 2; ++i ) {
		glBindTexture( GL_TEXTURE_RECTANGLE, textureId[ i ] );
		glTexParameterf( GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
		glTexParameterf( GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		glTexImage2D( GL_TEXTURE_RECTANGLE, 0, GL_RGBA32F, INIT_WINDOW_WIDTH, INIT_WINDOW_HEIGHT, 0, GL_RGBA, GL_FLOAT, 0 );
	}
	printf( "Finished.\n" );

	// Create a renderbuffer object to store depth info
	printf( "Creating renderbuffer object. " );
	glGenRenderbuffers( 1, &renderbufferId );
	glBindRenderbuffer( GL_RENDERBUFFER, renderbufferId );
	glRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH_COMPONENT, INIT_WINDOW_WIDTH, INIT_WINDOW_HEIGHT );
	glBindRenderbuffer( GL_RENDERBUFFER, 0 );
	printf( "Finished.\n" );

	// Create a framebuffer object
	printf( "Creating framebuffer object. " );
	glGenFramebuffers( 1, &framebufferId );
	glBindFramebuffer( GL_FRAMEBUFFER, framebufferId );
	printf( "Finished.\n" );

	// Attach the texture object to the framebuffer color attachment point
	printf( "Attaching texture object to the FBO. " );
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE, textureId[0], 0 );
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_RECTANGLE, textureId[1], 0 );
	printf( "Finished.\n" );

	// Attach the renderbuffer to the framebuffer depth attachment point
	printf( "Attaching renderbuffer object to the FBO. " );
	glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderbufferId );
	printf( "Finished.\n" );

	// Check status
	checkFramebufferStatus();

	// Switch back to window-system-provided framebuffer
	glBindFramebuffer( GL_FRAMEBUFFER, 0 );

}

// Destroy shaders and release program
void DestroyShaderPrograms( void ) {

	for( int i = 0; i < numShaders; ++i ) {
		DestroyProgram( progID[i], vertID[i], fragID[i] );
	}

}

// Create shaders and programs
void CreateShaderPrograms( void ) {

	vertID[ CPOS_SHADER ] = CreateShader( "shaders/cpos.vert", GL_VERTEX_SHADER );
	fragID[ CPOS_SHADER ] = CreateShader( "shaders/cpos.frag", GL_FRAGMENT_SHADER );
	progID[ CPOS_SHADER ] = CreateProgram( vertID[ CPOS_SHADER ], fragID[ CPOS_SHADER ] );

	vertID[ JUMP_SHADER ] = CreateShader( "shaders/jump.vert", GL_VERTEX_SHADER );
	fragID[ JUMP_SHADER ] = CreateShader( "shaders/jump.frag", GL_FRAGMENT_SHADER );
	progID[ JUMP_SHADER ] = CreateProgram( vertID[ JUMP_SHADER ], fragID[ JUMP_SHADER ] );

	vertID[ TEXTURE_SHADER ] = CreateShader( "shaders/tex.vert", GL_VERTEX_SHADER );
	fragID[ TEXTURE_SHADER ] = CreateShader( "shaders/tex.frag", GL_FRAGMENT_SHADER );
	progID[ TEXTURE_SHADER ] = CreateProgram( vertID[ TEXTURE_SHADER ], fragID[ TEXTURE_SHADER ] );

}

// Create a random number of seeds with random coordinates and colors
void CreateRandomSeeds( void ) {

	Seeds.clear();

	int numSeeds = my_rand( 9 ) + 3;

	for( int i = 0; i < numSeeds; ++i ) {

		Seed newSeed;
		newSeed.x = my_rand( INIT_WINDOW_WIDTH  ) / (float)INIT_WINDOW_WIDTH;
		newSeed.y = my_rand( INIT_WINDOW_HEIGHT ) / (float)INIT_WINDOW_HEIGHT;

		Seeds.push_back( newSeed );
	}

	printf( "Number of seeds total: %zu.\n", Seeds.size() );

}

// Set up an orthogonal view
void SetOrthoView( void ) {

	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();

	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();

	glOrtho( 0, 1, 0, 1, -1 , 1 );

}

// Draws a plane covering the entire screen
void plane( void ) {

	glBegin( GL_QUADS );
		glVertex2f( 0.0f, 0.0f );
		glVertex2f( 0.0f, 1.0f );
		glVertex2f( 1.0f, 1.0f );
		glVertex2f( 1.0f, 0.0f );
	glEnd();

}

// Renders the next frame and puts it on the display
void DisplayFunc( void ) {

	SetOrthoView();

	/*===============================================================================
	  RENDER POINTS TO TEXTURE
	===============================================================================*/

	// Bind our framebuffer
	glBindFramebuffer( GL_FRAMEBUFFER, framebufferId );

	// Set rendering destination to first color attachment
	glDrawBuffer( GL_COLOR_ATTACHMENT0 );

	// Clear the buffer
	glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
	glClear( GL_COLOR_BUFFER_BIT );

	// Shader that simply stores the point's pixel position
	glUseProgram( progID[ CPOS_SHADER ] );

	// Draw the seeds into the texture
	glPointSize( 1 );
	glBegin( GL_POINTS );
		for( int i = 0; i < Seeds.size(); ++i ) {
			glColor4f( Seeds[i].x, Seeds[i].y, 0.0f, 1.0f );
			glVertex4f( Seeds[i].x, Seeds[i].y, 0.0f, 1.0f );
		}
	glEnd();

	/*===============================================================================
	  EXECUTE JUMP FLOODING
	===============================================================================*/

	// Use the jump flooding shader
	glUseProgram( progID[ JUMP_SHADER ] );

	// Activate textures and send uniform variables to the shader program
	char tex[12];
	GLint uTexLoc;
	for( int i = 0; i < 2; ++i ) {
		glActiveTexture( GL_TEXTURE0 + i );
		glBindTexture( GL_TEXTURE_RECTANGLE, textureId[i] );
		sprintf( tex, "tex%i", i );
		uTexLoc = glGetUniformLocation( progID[ JUMP_SHADER ], tex );
		glUniform1i( uTexLoc, i );
	}

	GLint uIterLoc = glGetUniformLocation( progID[ JUMP_SHADER ], "iter" );
	GLint uStepLoc = glGetUniformLocation( progID[ JUMP_SHADER ], "step" );

	GLint uWidthLoc = glGetUniformLocation( progID[ JUMP_SHADER ], "width" );
	glUniform1f( uWidthLoc, (float)WindowWidth );

	GLint uHeightLoc = glGetUniformLocation( progID[ JUMP_SHADER ], "height" );
	glUniform1f( uHeightLoc, (float)WindowHeight );

	bool readingAttach0 = true;
	int step = INIT_WINDOW_WIDTH > INIT_WINDOW_HEIGHT ? INIT_WINDOW_WIDTH/2 : INIT_WINDOW_HEIGHT/2;

	// Jump flooding iterations
	while( step >= 1 ) {

		glUniform1f( uStepLoc, (float)step );

		if( readingAttach0 == true ) {
			// Set rendering destination to second buffer
			glDrawBuffer( GL_COLOR_ATTACHMENT1 );
			glUniform1i( uIterLoc, 0 );
		}
		else {
			// Set rendering destination to first buffer
			glDrawBuffer( GL_COLOR_ATTACHMENT0 );
			glUniform1i( uIterLoc, 1 );
		}

		// Draw a plane over the entire screen to invoke shaders
		plane();

		// Halve the step
		step /= 2;

		// Swap read/write buffers
		readingAttach0 = !readingAttach0;
	}

	// For rendering, use the texture that was written to last
	if( readingAttach0 == true )
		curTexture = 1;
	else
		curTexture = 2;

	/*===============================================================================
	  NORMAL RENDER
	===============================================================================*/

	// Switch back to window-system-provided framebuffer
	glBindFramebuffer( GL_FRAMEBUFFER, 0 );
	glDrawBuffer( GL_BACK );

	// Clear the buffer
	glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
	glClear( GL_COLOR_BUFFER_BIT );

	// Renders the texture content on the screen
	glUseProgram( progID[ TEXTURE_SHADER ] );

	// Activate textures and send uniform variables to the shader program
	for( int i = 0; i < 2; ++i ) {
		glActiveTexture( GL_TEXTURE0 + i );
		glBindTexture( GL_TEXTURE_RECTANGLE, textureId[ i ] );
		sprintf( tex, "tex%i", i );
		uTexLoc = glGetUniformLocation( progID[ TEXTURE_SHADER ], tex );
		glUniform1i( uTexLoc, i );
	}

	uWidthLoc = glGetUniformLocation( progID[ TEXTURE_SHADER ], "width" );
	glUniform1f( uWidthLoc, (float)WindowWidth );

	uHeightLoc = glGetUniformLocation( progID[ TEXTURE_SHADER ], "height" );
	glUniform1f( uHeightLoc, (float)WindowHeight );

	GLint uTexIdLoc = glGetUniformLocation( progID[ TEXTURE_SHADER ], "texId" );
	glUniform1i( uTexIdLoc, curTexture-1 );

	// Draw a plane over the entire screen to invoke shaders
	plane();

	// Reset shader and texture usage
	glUseProgram( 0 );
	glBindTexture( GL_TEXTURE_RECTANGLE, 0 );

	// Draw the seeds so we can see where they are
	glPointSize( SeedSize );
	glColor4f( 0.0f, 0.0f, 0.0f, 1.0f );
	glBegin( GL_POINTS );
		for( int i = 0; i < Seeds.size(); ++i )
			glVertex4f( Seeds[i].x, Seeds[i].y, 0.0f, 1.0f );
	glEnd();

	glPointSize( SeedSize-2 );
	glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
	glBegin( GL_POINTS );
		for( int i = 0; i < Seeds.size(); ++i )
			glVertex4f( Seeds[i].x, Seeds[i].y, 0.0f, 1.0f );
	glEnd();

	// Swap the buffers, flushing to screen.
	glutSwapBuffers();

}

// Called when there is nothing to do
void IdleFunc( void ) {

	// Uncomment the following to repeatedly update the window
	//glutPostRedisplay();

}

// Makes the necessary adjustments if the window is resized
void ReshapeFunc( int width, int height ) {

	// Adjust the viewport
	glViewport( 0, 0, (GLsizei)width, (GLsizei)height );

	// Update current window dimensions
	WindowWidth  = width;
	WindowHeight = height;

}

// Called when a (ASCII) keyboard key is pressed
void KeyboardFunc( unsigned char key, int x, int y ) {  

	switch( key ) {
		// ESC exits the program
		case 27:
			exit(0);

		// f enters and leaves fullscreen mode
		case 'f':
			FullScreen = !FullScreen;
			FullScreen ? glutFullScreen() : glutPositionWindow(0,0);
			break;

		// Create another set of random seeds
		case 'r':
			CreateRandomSeeds();
			break;
	}

	// Request a redisplay
	glutPostRedisplay();

}

// Called when an item from a pop-up menu is selected
void MenuFunc( int value ) {

	switch( value ) {
		case ENTRY_GENERATE_SEEDS:
			CreateRandomSeeds();
			break;

		case ENTRY_FULLSCREEN_ENTER:
			if( !FullScreen ) {
				glutFullScreen();
				FullScreen = true;
			}
			break;

		case ENTRY_FULLSCREEN_LEAVE:
			if( FullScreen ) {
				glutPositionWindow(0,0);
				FullScreen = false;
			}
			break;

		case ENTRY_QUIT:
			exit(0);
	}

	// Request a redisplay
	glutPostRedisplay();

}

// Initializes variables and OpenGL settings
void Initialize( void ) {

	glDisable( GL_DEPTH_TEST );
	glDisable( GL_BLEND );

	// Create shaders
	CreateShaderPrograms();

	// Create framebuffer and its components
	CreateFBO();

	// Create random seeds
	CreateRandomSeeds();

	// Create main pop-up menu and add entries
	MenuId = glutCreateMenu( &MenuFunc );
	glutAddMenuEntry( "Generate Random Seeds", ENTRY_GENERATE_SEEDS );
	glutAddMenuEntry( "Enter FullScreen", ENTRY_FULLSCREEN_ENTER );
	glutAddMenuEntry( "Leave FullScreen", ENTRY_FULLSCREEN_LEAVE );
	glutAddMenuEntry( "Quit", ENTRY_QUIT );

	// Attach menu to right mouse button
	glutAttachMenu( GLUT_RIGHT_BUTTON );

}

// Where it all begins...
int main( int argc, char **argv ) {

	// Initialize GLUT and window properties
	glutInit( &argc, argv );
	glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGBA );
	glutInitWindowSize( INIT_WINDOW_WIDTH, INIT_WINDOW_HEIGHT );
	glutInitWindowPosition( INIT_WINDOW_POS_X, INIT_WINDOW_POS_Y );

	// Create main OpenGL window
	glutCreateWindow( "Jump Flooding Voronoi" );

	// Initialize GLEW for shaders
	glewInit();

	// Initialize variables and settings
	Initialize();

	// Register callback functions
	glutDisplayFunc( DisplayFunc );
	glutIdleFunc( IdleFunc );
	glutReshapeFunc( ReshapeFunc );

	glutKeyboardFunc( KeyboardFunc );

	// Enter the main loop
	glutMainLoop();

	return 0;

}

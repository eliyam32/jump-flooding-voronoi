/*=================================================================================================
  Author: Renato Farias (renatomdf@gmail.com)
  Created on: June 12th, 2012
  About: This is a GPU implementation of the Jump Flooding algorithm from the paper "Jump Flooding
   in GPU With Applications to Voronoi Diagram and Distance Transform" [Rong 2006]. I use
   render-to-texture and shaders to execute the algorithm, as per the paper. The result is a
   Voronoi diagram generated from randomly placed seeds. The seeds are given velocities so that
   they move about the screen.
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
#define INIT_WINDOW_WIDTH  1024
#define INIT_WINDOW_HEIGHT  768

// Initial screen position
#define INIT_WINDOW_POS_X 0
#define INIT_WINDOW_POS_Y 0

/*=================================================================================================
  STRUCTS
=================================================================================================*/

// A seed with coordinates, color, and velocity vector
typedef struct {
	float x,y; // location
	float r,g,b; // color
	float i,j; // velocity
} Seed;

/*=================================================================================================
  GLOBALS
=================================================================================================*/

// Current window dimensions
int WindowWidth  = INIT_WINDOW_WIDTH;
int WindowHeight = INIT_WINDOW_HEIGHT;

// List of seeds
vector<Seed> Seeds;

// Number of seeds to be created
int NumSeeds = -1;

// How large to draw each seed, used with glPointSize()
int SeedSize = 10;

// Buffer dimensions
int BufferWidth  = INIT_WINDOW_WIDTH;
int BufferHeight = INIT_WINDOW_HEIGHT;

// Which buffer are we reading from?
bool ReadingBufferA = true;

// Is the window currently fullscreen?
bool FullScreen = false;

// Show FPS in the title bar?
bool ShowFPS = true;

// Time when the last frame was drawn
double LastRefreshTime;

// For calculating FPS
double FPS_StartTime, FPS_EndTime;
int FrameCount = 0, FPS = 0, FPS_Update_Interval = 500;

// Shaders
#define numShaders 3
enum ShaderEnum {
	CPOS_SHADER = 0,
	JUMP_SHADER,
	TEXTURE_SHADER
};
GLuint vertID[ numShaders ], fragID[ numShaders ], progID[ numShaders ];

// Render to texture
#define numTextures 4
GLuint framebufferId, renderbufferId, textureId[ numTextures ];
GLenum buffersA[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
GLenum buffersB[] = { GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
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
	glGenTextures( numTextures, &textureId[0] );
	for( int i = 0; i < numTextures; ++i ) {
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
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_RECTANGLE, textureId[2], 0 );
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_RECTANGLE, textureId[3], 0 );
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

// Apply seeds' velocity vectors
void UpdateSeedPositions( double delta ) {

	for( int i = 0; i < Seeds.size(); ++i ) {

		float newX = Seeds[i].x + Seeds[i].i * delta / 1000;
		float newY = Seeds[i].y + Seeds[i].j * delta / 1000;

		if( newX < 0.0f || newX >= 1.0f )
			Seeds[i].i *= -1;
		else
			Seeds[i].x = newX;

		if( newY < 0.0f || newY >= 1.0f )
			Seeds[i].j *= -1;
		else
			Seeds[i].y = newY;

	}

}

// Create a random number of seeds with random coordinates and colors
void CreateRandomSeeds( bool forceNewNumSeeds = false ) {

	Seeds.clear();

	if( NumSeeds == -1 || forceNewNumSeeds == true )
		NumSeeds = my_rand( 27 ) + 4;

	int vMax = 200;
	int vMin = 100;

	for( int i = 0; i < NumSeeds; ++i ) {

		Seed newSeed;

		newSeed.x = my_rand( INIT_WINDOW_WIDTH  ) / (float)INIT_WINDOW_WIDTH;
		newSeed.y = my_rand( INIT_WINDOW_HEIGHT ) / (float)INIT_WINDOW_HEIGHT;

		newSeed.r = my_rand( 100 ) / (float)100;
		newSeed.g = my_rand( 100 ) / (float)100;
		newSeed.b = my_rand( 100 ) / (float)100;

		//newSeed.i = ( my_rand( vMax - vMin ) + vMin - 1 ) / (float)INIT_WINDOW_WIDTH;
		//newSeed.j = ( my_rand( vMax - vMin ) + vMin - 1 ) / (float)INIT_WINDOW_HEIGHT;

		newSeed.i = ( my_rand( 2*vMax ) - vMax ) / (float)INIT_WINDOW_WIDTH;
		newSeed.j = ( my_rand( 2*vMax ) - vMax ) / (float)INIT_WINDOW_HEIGHT;

		Seeds.push_back( newSeed );

	}

	printf( "Number of seeds: %zu.\n", Seeds.size() );

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

	//for FPS
	if( ShowFPS == true && FrameCount == 0 )
		FPS_StartTime = get_clock_msec();

	// Time since last frame for update operations
	double time = get_clock_msec();
	double delta = time - LastRefreshTime;

	// Update last refresh time
	LastRefreshTime = time;

	// Apply velocities
	UpdateSeedPositions( delta );

	SetOrthoView();

	// Uniform location  variables
	GLint uIterLoc,uStepLoc,uTex0Loc,uTex1Loc;
	GLint uWidthLoc,uHeightLoc;

	/*===============================================================================
	  RENDER POINTS TO TEXTURE
	===============================================================================*/

	// Bind our framebuffer
	glBindFramebuffer( GL_FRAMEBUFFER, framebufferId );

	// Set the rendering destination to first set of buffers
	glDrawBuffers( 2, buffersA );

	// Clear the buffer
	glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
	glClear( GL_COLOR_BUFFER_BIT );

	// Shader that simply stores the point's pixel position and color
	glUseProgram( progID[ CPOS_SHADER ] );

	// Draw the seeds into the texture
	glPointSize( 1 );
	glBegin( GL_POINTS );
		for( int i = 0; i < Seeds.size(); ++i ) {
			glColor4f( Seeds[i].r, Seeds[i].g, Seeds[i].b, 1.0f );
			glVertex4f( Seeds[i].x, Seeds[i].y, 0.0f, 1.0f );
		}
	glEnd();

	/*===============================================================================
	  EXECUTE JUMP FLOODING
	===============================================================================*/

	// Use the jump flooding shader
	glUseProgram( progID[ JUMP_SHADER ] );

	// Activate textures and send uniform variables to the shader program
	for( int i = 0; i < numTextures; ++i ) {
		glActiveTexture( GL_TEXTURE0 + i );
		glBindTexture( GL_TEXTURE_RECTANGLE, textureId[i] );
	}

	uStepLoc = glGetUniformLocation( progID[ JUMP_SHADER ], "step" );
	uTex0Loc = glGetUniformLocation( progID[ JUMP_SHADER ], "tex0" );
	uTex1Loc = glGetUniformLocation( progID[ JUMP_SHADER ], "tex1" );

	uWidthLoc = glGetUniformLocation( progID[ JUMP_SHADER ], "width" );
	glUniform1f( uWidthLoc, (float)WindowWidth );

	uHeightLoc = glGetUniformLocation( progID[ JUMP_SHADER ], "height" );
	glUniform1f( uHeightLoc, (float)WindowHeight );

	bool readingAttach0 = true;
	int step = INIT_WINDOW_WIDTH > INIT_WINDOW_HEIGHT ? INIT_WINDOW_WIDTH/2 : INIT_WINDOW_HEIGHT/2;

	// Jump flooding iterations
	while( step >= 1 ) {

		glUniform1f( uStepLoc, (float)step );

		if( readingAttach0 == true ) {
			// Set rendering destination to second set of buffers
			glDrawBuffers( 2, buffersB );
			glUniform1i( uTex0Loc, 0 );
			glUniform1i( uTex1Loc, 1 );
		}
		else {
			// Set rendering destination to first set of buffers
			glDrawBuffers( 2, buffersA );
			glUniform1i( uTex0Loc, 2 );
			glUniform1i( uTex1Loc, 3 );
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
		curTexture = 3;

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
	for( int i = 0; i < numTextures; ++i ) {
		glActiveTexture( GL_TEXTURE0 + i );
		glBindTexture( GL_TEXTURE_RECTANGLE, textureId[ i ] );
	}

	uTex0Loc = glGetUniformLocation( progID[ TEXTURE_SHADER ], "tex" );
	glUniform1i( uTex0Loc, curTexture );

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

	//for FPS
	if( ShowFPS == true ) {

		++FrameCount;
		FPS_EndTime = get_clock_msec();

		if( FPS_EndTime - FPS_StartTime > FPS_Update_Interval ) {

			FPS = FrameCount / ( FPS_EndTime - FPS_StartTime ) * 1000;

			char title[128];
			sprintf( title, "Jump Flooding Voronoi | %i Seeds | FPS: %i", NumSeeds, FPS );
			glutSetWindowTitle( title );

			FrameCount = 0;

		}

	}

}

// Called when there is nothing to do
void IdleFunc( void ) {

	// Uncomment the following to repeatedly update the window
	glutPostRedisplay();

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
		/*case 'f':
			FullScreen = !FullScreen;
			FullScreen ? glutFullScreen() : glutPositionWindow(0,0);
			break;*/

		// Create another set of random seeds
		case 'r':
			CreateRandomSeeds( true );
			break;
	}

	// Request a redisplay
	glutPostRedisplay();

}

// Called when an item from a pop-up menu is selected
void MenuFunc( int value ) {

	switch( value ) {
		case ENTRY_GENERATE_SEEDS:
			CreateRandomSeeds( true );
			break;

		/*case ENTRY_FULLSCREEN_ENTER:
			if( !FullScreen ) {
				glutFullScreen();
				FullScreen = true;
			}
			break;*/

		/*case ENTRY_FULLSCREEN_LEAVE:
			if( FullScreen ) {
				glutPositionWindow(0,0);
				FullScreen = false;
			}
			break;*/

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
	//glutAddMenuEntry( "Enter FullScreen", ENTRY_FULLSCREEN_ENTER );
	//glutAddMenuEntry( "Leave FullScreen", ENTRY_FULLSCREEN_LEAVE );
	glutAddMenuEntry( "Quit", ENTRY_QUIT );

	// Attach menu to right mouse button
	glutAttachMenu( GLUT_RIGHT_BUTTON );

	// Initialize refresh time
	LastRefreshTime = get_clock_msec();

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

	// Read number of seeds from the command line
	if( argc > 1 )
		NumSeeds = atoi( argv[1] );

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

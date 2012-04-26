/*=================================================================================================
  Author: Renato Farias (renatomdf@gmail.com)
  Created on: April 13th, 2012
  Purpose: A CPU implementation of the Jump Flooding Algorithm from the paper "Jump Flooding in
   GPU With Applications to Voronoi Diagram and Distance Transform" [Rong 2006]. The result is
   a Voronoi diagram generated from a number of seeds which the user provides with mouse clicks.
=================================================================================================*/

/*=================================================================================================
  INCLUDES
=================================================================================================*/

#include <GL/glut.h>

#include <assert.h>
#include <stdio.h>
#include <vector>

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

// Represents a point with (x,y) coordinates
typedef struct {
	int x,y;
} Point;

/*=================================================================================================
  GLOBALS
=================================================================================================*/

// Current window dimensions
int WindowWidth  = INIT_WINDOW_WIDTH;
int WindowHeight = INIT_WINDOW_HEIGHT;

// List of seeds
vector<Point> Seeds;

// Buffers
Point* BufferA = NULL;
Point* BufferB = NULL;

// Buffer dimensions
int BufferWidth  = INIT_WINDOW_WIDTH;
int BufferHeight = INIT_WINDOW_HEIGHT;

// Which buffer are we reading from?
bool ReadingBufferA = true;

// Is the window currently fullscreen?
bool FullScreen = false;

/*=================================================================================================
  FUNCTIONS
=================================================================================================*/

// Jump Flooding Algorithm
void ExecuteJumpFlooding( void ) {

	printf( "Executing the Jump Flooding Algorithm...\n" );

	// If the buffers already exist, delete them first
	if( BufferA != NULL ) {
		free( BufferA );
		BufferA = NULL;
	}
	if( BufferB != NULL ) {
		free( BufferB );
		BufferB = NULL;
	}

	// Allocate memory for the two buffers
	BufferA = (Point*)malloc( sizeof( Point ) * BufferWidth * BufferHeight );
	BufferB = (Point*)malloc( sizeof( Point ) * BufferWidth * BufferHeight );

	assert( BufferA != NULL && BufferB != NULL );

	// Initialize BufferA with (-1,-1), indicating an invalid closest seed.
	// We don't need to initialize BufferB because it will be written to in the first round.
	for( int y = 0; y < BufferHeight; ++y ) {
		for( int x = 0; x < BufferWidth; ++x ) {
			int idx = ( y * BufferWidth ) + x;
			BufferA[ idx ].x = -1;
			BufferA[ idx ].y = -1;
		}
	}

	// Put the seeds into the first buffer
	for( int i = 0; i < Seeds.size(); ++i ) {
		Point& p = Seeds[i];
		BufferA[ ( p.y * BufferWidth ) + p.x ] = p;
	}

	// Initial step length is half the image's size. If the image isn't square,
	// we use the largest dimension.
	int step = BufferWidth > BufferHeight ? BufferWidth/2 : BufferHeight/2;

	// We use this boolean to know which buffer we are reading from
	ReadingBufferA = true;

	// We read from the RBuffer and write into the WBuffer
	Point* RBuffer;
	Point* WBuffer;

	// Carry out the rounds of Jump Flooding
	while( step >= 1 ) {

		printf( "Jump Flooding with Step %i.", step );

		// Set which buffers we'll be using
		if( ReadingBufferA == true ) {
			printf( " Reading from BufferA and writing to BufferB.\n" );
			RBuffer = BufferA;
			WBuffer = BufferB;
		}
		else {
			printf( " Reading from BufferB and writing to BufferA.\n" );
			RBuffer = BufferB;
			WBuffer = BufferA;
		}

		// Iterate over each point to find its closest seed
		for( int y = 0; y < BufferHeight; ++y ) {
			for( int x = 0; x < BufferWidth; ++x ) {

				// The point's absolute index in the buffer
				int idx = ( y * BufferWidth ) + x;

				// The point's current closest seed (if any)
				Point& p = RBuffer[ idx ];

				// Go ahead and write our current closest seed, if any. If we don't do this
				// we might lose this information if we don't update our seed this round.
				WBuffer[ idx ] = p;

				// This is a seed, so skip this point
				if( p.x == x && p.y == y )
					continue;

				// This variable will be used to judge which seed is closest
				float dist;

				if( p.x == -1 || p.y == -1 )
					dist = -1; // No closest seed has been found yet
				else
					dist = (p.x-x)*(p.x-x) + (p.y-y)*(p.y-y); // Current closest seed's distance

				// To find each point's closest seed, we look at its 8 neighbors thusly:
				//   (x-step,y-step) (x,y-step) (x+step,y-step)
				//   (x-step,y     ) (x,y     ) (x+step,y     )
				//   (x-step,y+step) (x,y+step) (x+step,y+step)

				for( int ky = -1; ky <= 1; ++ky ) {
					for( int kx = -1; kx <= 1; ++kx ) {

						// Calculate neighbor's row and column
						int ny = y + ky * step;
						int nx = x + kx * step;

						// If the neighbor is outside the bounds of the buffer, skip it
						if( nx < 0 || nx >= BufferWidth || ny < 0 || ny >= BufferHeight )
							continue;

						// Calculate neighbor's absolute index
						int nidx = ( ny * BufferWidth ) + nx;

						// Retrieve the neighbor
						Point& pk = RBuffer[ nidx ];

						// If the neighbor doesn't have a closest seed yet, skip it
						if( pk.x == -1 || pk.y == -1 )
							continue;

						// Calculate the distance from us to the neighbor's closest seed
						float newDist = (pk.x-x)*(pk.x-x) + (pk.y-y)*(pk.y-y);

						// If dist is -1, it means we have no closest seed, so we might as well take this one
						// Otherwise, only adopt this new seed if it's closer than our current closest seed
						if( dist == -1 || newDist < dist ) {
							WBuffer[ idx ] = pk;
							dist = newDist;
						}

					}
				}

			}
		}

		// Halve the step.
		step /= 2;

		// Swap the buffers for the next round
		ReadingBufferA = !ReadingBufferA;

	}

	// Lets us know which buffer we end up reading from
	if( ReadingBufferA == true )
		printf( "Reading from BufferA.\n" );
	else
		printf( "Reading from BufferB.\n" );

}

// Renders the next frame and puts it on the display
void DisplayFunc( void ) {

	// Clear the window.
	glClear( GL_COLOR_BUFFER_BIT );

	// Draw the buffer, if possible
	Point* Buffer = ReadingBufferA == true ? BufferA : BufferB;
	if( Buffer != NULL ) {

		glPointSize( 1 );
		glBegin( GL_POINTS );
			for( int y = 0; y < WindowHeight; ++y ) {
				for( int x = 0; x < WindowWidth; ++x ) {

					// The window might not be the same size as the buffer, so
					// we need to find which point corresponds to this pixel.

					float fx = (float)x / WindowWidth;
					float fy = (float)y / WindowHeight;

					int bx = fx * BufferWidth;
					int by = fy * BufferHeight;

					int idx = by * BufferWidth + bx;

					// Calculate color using seed positions for now
					glColor3f( (float)Buffer[idx].x / BufferWidth,
					           (float)Buffer[idx].y / BufferHeight,
					           0.0f );

					glVertex2f( fx, fy );

				}
			}
		glEnd();

	}

	// Draw the seeds
	glColor3f( 0.0f, 0.0f, 1.0f );
	glPointSize( 8 );
	glBegin( GL_POINTS );
		for( int i = 0; i < Seeds.size(); ++i ) {
			glVertex2f( (float)Seeds[i].x / BufferWidth, (float)Seeds[i].y / BufferHeight );
		}
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

		// Switch from which buffer we're reading from
		// In effect, this lets us see the second-to-last iteration
		case 'b':
			ReadingBufferA = !ReadingBufferA;
			if( ReadingBufferA == true )
				printf( "Reading from BufferA.\n" );
			else
				printf( "Reading from BufferB.\n" );
			break;

		// Clear Seeds and buffers
		case 'c':
			Seeds.clear();

			if( BufferA != NULL ) {
				free( BufferA );
				BufferA = NULL;
			}

			if( BufferB != NULL ) {
				free( BufferB );
				BufferB = NULL;
			}

			printf( "Clear.\n" );
			break;

		// Execute the algorithm
		case 'e':
			ExecuteJumpFlooding();
			break;

		// f enters and leaves fullscreen mode
		case 'f':
			FullScreen = !FullScreen;
			FullScreen ? glutFullScreen() : glutPositionWindow(0,0);
			break;
	}

	// Request a redisplay
	glutPostRedisplay();

}

// Called when a mouse button is pressed or released
void MouseFunc( int button, int state, int x, int y ) {

	// Key 0: left button
	// Key 1: middle button
	// Key 2: right button
	// Key 3: scroll up
	// Key 4: scroll down

	if( button == 0 && state == GLUT_DOWN ) {

		Point* Buffer = ReadingBufferA == true ? BufferA : BufferB;

		if( Buffer == NULL ) {

			printf( "Creating new seed at (%i,%i).", x, y );

			// The window might not be the same size as the buffer, so we need to
			// convert the pixel's (x,y) to its corresponding buffer point's (x,y).

			float fx = (float)x / WindowWidth;
			float fy = (float)y / WindowHeight;

			int bx = fx * BufferWidth;
			int by = fy * BufferHeight;

			Point p = { bx, by };
			Seeds.push_back( p );

			printf( " %zi seeds total.\n", Seeds.size() );

		}

	}

	// Request a redisplay
	glutPostRedisplay();

}

// Initializes variables and OpenGL settings
void Initialize( void ) {

	// Set the background color to white
	glClearColor( 1.0f, 1.0f, 1.0f, 1.0f );

	// Set up orthogonal projection with (0,0) at the top-left corner of the window
	glOrtho( 0, 1, 1, 0, -1 ,1 );

}

// Where it all begins...
int main( int argc, char **argv ) {

	// Initialize GLUT and window properties
	glutInit( &argc, argv );
	glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH );
	glutInitWindowSize( INIT_WINDOW_WIDTH, INIT_WINDOW_HEIGHT );
	glutInitWindowPosition( INIT_WINDOW_POS_X, INIT_WINDOW_POS_Y );

	// Create main OpenGL window
	glutCreateWindow( "Jump Flooding Voronoi" );

	// Initialize variables and settings
	Initialize();

	// Register callback functions
	glutDisplayFunc( DisplayFunc );
	glutIdleFunc( IdleFunc );
	glutReshapeFunc( ReshapeFunc );
	glutKeyboardFunc( KeyboardFunc );
	glutMouseFunc( MouseFunc );

	// Enter the main loop
	glutMainLoop();

	return 0;

}

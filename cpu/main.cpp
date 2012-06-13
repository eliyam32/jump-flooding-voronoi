/*=================================================================================================
  Author: Renato Farias (renatomdf@gmail.com)
  Created on: April 13th, 2012
  Updated on: June 12th, 2012
  About: This is a simple CPU implementation of the Jump Flooding algorithm from the paper "Jump
   Flooding in GPU With Applications to Voronoi Diagram and Distance Transform" [Rong 2006]. The
   result is a Voronoi diagram generated from a number of seeds which the user provides with mouse
   clicks. You can also click on and drag around a seed to reposition it, if that's your thing.
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

// Index of the seed currently selected, if any
int CurSeedIdx = -1;

// How large to draw each seed, used with glPointSize()
int SeedSize = 8;

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

// Pop-up menu
int MenuId;
enum MenuEntries {
	ENTRY_QUIT = 0,
	ENTRY_GENERATE_VORONOI,
	ENTRY_CLEAR_ALL,
	ENTRY_FULLSCREEN_ENTER,
	ENTRY_FULLSCREEN_LEAVE
};

/*=================================================================================================
  FUNCTIONS
=================================================================================================*/

// If the buffers exist, delete them
void ClearBuffers( void ) {

	if( BufferA != NULL ) {
		free( BufferA );
		BufferA = NULL;
	}

	if( BufferB != NULL ) {
		free( BufferB );
		BufferB = NULL;
	}

}

// Jump Flooding Algorithm
void ExecuteJumpFlooding( void ) {

	// No seeds will just give us a black screen :P
	if( Seeds.size() < 1 ) {
		printf( "Please create at least 1 seed.\n" );
		return;
	}

	printf( "Executing the Jump Flooding algorithm...\n" );

	// Clear the buffers before we start
	ClearBuffers();

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

		// Set which buffers we'll be using
		if( ReadingBufferA == true ) {
			RBuffer = BufferA;
			WBuffer = BufferB;
		}
		else {
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
	glPointSize( SeedSize );
	glColor3f( 0.0f, 0.0f, 1.0f );
	glBegin( GL_POINTS );
		for( int i = 0; i < Seeds.size(); ++i ) {

			// If this is the selected seed, use red...
			if( i == CurSeedIdx )
				glColor3f( 1.0f, 0.0f, 0.0f );

			glVertex2f( (float)Seeds[i].x / BufferWidth, (float)Seeds[i].y / BufferHeight );

			// ...then set the color back to normal
			if( i == CurSeedIdx )
				glColor3f( 0.0f, 0.0f, 1.0f );

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

		// Clear Seeds and buffers
		case 'c':
			Seeds.clear();
			ClearBuffers();
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

	if( button == 0 ) {
		if( state == GLUT_DOWN ) {

			// The window might not be the same size as the buffer, so we need to
			// convert the pixel's (x,y) to its corresponding buffer (x,y)

			float fx = (float)x / WindowWidth;
			float fy = (float)y / WindowHeight;

			int bx = fx * BufferWidth;
			int by = fy * BufferHeight;

			// Get a pointer to the buffer we're currently using
			Point* Buffer = ReadingBufferA == true ? BufferA : BufferB;

			// If the Voronoi diagram has yet to be created, add a seed
			// Otherwise, check if one of the seeds has been selected
			if( Buffer == NULL ) {

				printf( "Creating new seed at (%i,%i).", x, y );

				Point p = { bx, by };
				Seeds.push_back( p );

				printf( " %zi seeds total.\n", Seeds.size() );

			}
			else {

				// Has one of the seeds been selected?
				for( int i = 0; i < Seeds.size(); ++i ) {

					Point& p = Seeds[i];

					float dist = (bx-p.x)*(bx-p.x) + (by-p.y)*(by-p.y);

					if( dist <= SeedSize*SeedSize ) {
						CurSeedIdx = i;
						break;
					}

				}

			}

		}
		else { // state == GLUT_UP

			if( CurSeedIdx != -1 ) {

				// Recreate the Voronoi diagram
				ExecuteJumpFlooding();

				// There's no longer a seed selected
				CurSeedIdx = -1;

			}

		}
	}

	// Request a redisplay
	glutPostRedisplay();

}

// Called when the mouse cursor is moved while a mouse button is pressed
void MotionFunc( int x, int y ) {

	// If there's a seed selected, move it
	if( CurSeedIdx != -1 ) {

		// Calculate the seed's new position
		float fx = (float)x / WindowWidth;
		float fy = (float)y / WindowHeight;

		int bx = fx * BufferWidth;
		int by = fy * BufferHeight;

		// Update it with its new position
		Seeds[ CurSeedIdx ].x = bx;
		Seeds[ CurSeedIdx ].y = by;

		// Request a redisplay
		glutPostRedisplay();

	}

}

// Called when an item from a pop-up menu is selected
void MenuFunc( int value ) {

	switch( value ) {
		case ENTRY_QUIT:
			exit(0);

		case ENTRY_GENERATE_VORONOI:
			ExecuteJumpFlooding();
			break;

		case ENTRY_CLEAR_ALL:
			Seeds.clear();
			ClearBuffers();
			printf( "Clear.\n" );
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
	}

	glutPostRedisplay();

}

// Initializes variables and OpenGL settings
void Initialize( void ) {

	// Set the background color to white
	glClearColor( 1.0f, 1.0f, 1.0f, 1.0f );

	// Set up orthogonal projection with (0,0) at the top-left corner of the window
	glOrtho( 0, 1, 1, 0, -1 ,1 );

	// Create main pop-up menu and add entries
	MenuId = glutCreateMenu( &MenuFunc );
	glutAddMenuEntry( "Generate Voronoi Diagram", ENTRY_GENERATE_VORONOI );
	glutAddMenuEntry( "Clear Seeds", ENTRY_CLEAR_ALL );
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

	// Initialize variables and settings
	Initialize();

	// Register callback functions
	glutDisplayFunc( DisplayFunc );
	glutIdleFunc( IdleFunc );
	glutReshapeFunc( ReshapeFunc );

	glutKeyboardFunc( KeyboardFunc );
	glutMouseFunc( MouseFunc );
	glutMotionFunc( MotionFunc );

	// Enter the main loop
	glutMainLoop();

	return 0;

}

#include <stdlib.h>
#include <sys/time.h>

#include "rfUtil.h"

bool rfRandInit = false;

double get_clock_sec( void )
{
	struct timeval t;
	struct timezone tz;
	gettimeofday(&t,&tz);
	return (double) t.tv_sec + (double) t.tv_usec * 1E-6;
}

double get_clock_msec( void )
{
	struct timeval t;
	struct timezone tz;
	gettimeofday(&t,&tz);
	return (double) t.tv_sec * 1E+3 + (double) t.tv_usec * 1E-3;
}

int my_rand( int range )
{
	if( rfRandInit == false ) {
		srand ( get_clock_msec() );
		rfRandInit = true;
	}
	return (int)(1 + (int)( (double)range * ( rand() / ( RAND_MAX + 1.0 ) ) ) );
}

#ifndef RF_UTIL_H
#define RF_UTIL_H

double get_clock_sec( void );
double get_clock_msec( void );

int my_rand( int range );

/*
Notes
The versions of rand() and srand() in the Linux C Library use the same random number generator as random() and srandom(), so the lower-order bits should be as random as the higher-order bits. However, on older rand() implementations, and on current implementations on different systems, the lower-order bits are much less random than the higher-order bits. Do not use this function in applications intended to be portable when good randomness is needed.

In Numerical Recipes in C: The Art of Scientific Computing (William H. Press, Brian P. Flannery, Saul A. Teukolsky, William T. Vetterling; New York: Cambridge University Press, 1992 (2nd ed., p. 277)), the following comments are made:

    "If you want to generate a random integer between 1 and 10, you should always do it by using high-order bits, as in

        j = 1 + (int) (10.0 * (rand() / (RAND_MAX + 1.0)));

    and never by anything resembling

        j = 1 + (rand() % 10);

    (which uses lower-order bits)."
*/

#endif //RF_UTIL_H

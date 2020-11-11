/* halt.c
 *	Simple program to test whether running a user program works.
 *	
 *	Just do a "syscall" that shuts down the OS.
 *
 * 	NOTE: for some reason, user programs with global data structures 
 *	sometimes haven't worked in the Nachos environment.  So be careful
 *	out there!  One option is to allocate data structures as 
 * 	automatics within a procedure, but if you do this, you have to
 *	be careful to allocate a big enough stack to hold the automatics!
 */

#include "syscall.h"

int
main()
{
	int a = 0,b=1,c = 2;
	int d = 4;
	d = a + b;
	d = d * c;
	for(a = 0;a < 10;a++){
		d = d + a;
		b++;
		c++;
		b = a + b;
		d = d * c;
	}
	a = a +  1;
	a = b * c;
	c = a;
	a++;
	a++;
	for(a = 0;a < 10;a++){
		d = d + a;
		b++;
		c++;
		b = a + b;
		d = d * c;
	}
	d = 4;
	a = a + 1;
    Halt();
    /* not reached */
}

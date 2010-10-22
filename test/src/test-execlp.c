#include <stdio.h>
#include <unistd.h>

int main ( int argc, char** argv )
{
	execlp( "execlp-test", "execlp-test" );

	/* Execution should never reach here */
	return 1;
}

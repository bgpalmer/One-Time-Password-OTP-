#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <time.h>

int main( int argc, char ** argv ) {
	if( argc < 2 ) { fprintf( stderr, "usage: %s [length] { > output }\n", argv[ 0 ]); exit( 0 ); }

	srand( time( NULL ) );
	int i, targetFd;
	int cnt = atoi( argv[ 1 ] );
	for ( i = 0; i < argc; i++ ) {
		if( strcmp( argv[ i ], ">" ) == 0 ) {
			targetFd = open( argv[ i + 1 ], O_WRONLY | O_CREAT | O_TRUNC, 0644 );
			if ( targetFd < 0 ) { perror( "Cannot open for output" ); exit( 1 ); }
			dup2( targetFd, 1 );
		}
	}
	char * key = ( char * ) calloc( sizeof( char ), cnt );
	int r;
	for (i = 0; i < cnt; i++) {
		r = 65 + rand() % 27;
		if ( r == 26 + 65 ) { key[ i ] = 32; }
		else { key[ i ] = r; }
	} 
	printf("%s\n", key);

	return 0;
}
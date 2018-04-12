/****************
 * Brian Palmer
 * palmebri@oregonstate.edu
 * otp_dec_d.c
**/

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUFSIZE 200000

/* Returns decrypted message */
void my_decrypt(char * msg, char * key) {
	int i;
	for( i = 0; i < strlen( msg ); i++ ) {

		(key[ i ] == 32) ? (key[ i ] = 26) : (key[ i ] -= 65);
		(msg[ i ] == 32) ? (msg[ i ] = 26) : (msg[ i ] -= 65);
		
		msg[ i ] -= key[ i ];

		if (msg[ i ] < 0) {
			msg[ i ] *= -1;
			msg[ i ] = 27 - msg[ i ];
		}

		msg[ i ] %= 27;
		
		(msg[ i ] == 26) ? (msg[ i ] = 32) : (msg[ i ] += 65);

	}
}

char 		buffer[ BUFSIZE ];
char		message[ BUFSIZE ];

pid_t pid_list[ 5 ] = { 0 };
int pid_c = 0;

void error(const char *msg) { fprintf(stderr, "Error: %s\n", msg); } // Error function used for reporting issues

int main( int argc, char ** argv ) {

	if (argc < 2) { fprintf(stderr, "usage: %s [port]\n", argv[ 0 ]); exit( 0 ); }

	int 		listener, connection;
	ssize_t 	r, total;
	int port = 	atoi( argv[ 1 ] );
	struct 		sockaddr_in server, client; 
	socklen_t 	sizeOfClientInfo;
	char 		* key = NULL;
	char 		* msg = NULL;
	char 		* header = "DEC_D@@";
	char 		partmsg[ 1024 ];
	
	memset((char *) &server, '\0', sizeof( server ));
	server.sin_family = AF_INET;
	server.sin_port = htons( port );
	server.sin_addr.s_addr = INADDR_ANY;

	listener = socket(AF_INET, SOCK_STREAM, 0);
	if (listener < 0) { error( "creating socket" ); exit( 1 ); }
	if( bind( listener, (struct sockaddr * ) &server, sizeof( server )) ) { error( "binding socket" ); exit( 1 ); }
	printf("SERVER: Listening on port %d...\n", port);
	while( 1 ) {

		sizeOfClientInfo = sizeof( client );

		if( listen( listener, 5 )) { error( "opening listening channel" ); exit( 1 ); }
		connection = accept( listener, (struct sockaddr *) &client, &sizeOfClientInfo );
		if (connection < 0) { error( "on accept" ); }

		printf("SERVER: Connected Client at port %d\n", ntohs( client.sin_port ));

		pid_t child;
		if (pid_c < 5 &&(child = fork()) == -1) { error( "forking. Likely too many connections, try again later!" ); }
		else if( child == 0 ) {

			close( listener );

			/* Determine client */

			memset( buffer , '\0', sizeof( buffer ));
			while( strstr( buffer, "@@") == NULL ) {
				memset( partmsg , '\0', sizeof( partmsg ));
				r = recv( connection, partmsg, sizeof( partmsg ) - 1, 0);
				strcat(buffer, partmsg);
				if (r == -1) { error("SERVER: receiving data (1)"); exit( 0 ); }
				if (r == 0) { error("SERVER: rejected client"); exit( 0 ); }
			}
			int terminal = strstr( buffer, "@@" ) - buffer;
			buffer[ terminal ] = '\0';

			/* If client is okay - send message back */
			if( strcmp( "DEC", buffer ) != 0 ) {
				exit( 0 );
			}

			memset( buffer, '\0', sizeof( buffer ));
			strcpy( buffer, header );
			total = 0;
			while( total < strlen( buffer )) {
				total += r = send( connection, buffer + total, sizeof( partmsg ), 0 );
				if( r == 0 ) { break; }
				if( r < 0 ) { error( "writing to socket" ); break; }
				//total += r;
			}

			/* Read data */

			memset( buffer , '\0', sizeof( buffer ));
			while( strstr( buffer, "@@") == NULL ) {
				memset( partmsg , '\0', sizeof( partmsg ));
				r = recv( connection, partmsg, sizeof( partmsg ) - 1, 0);
				strcat(buffer, partmsg);
				if (r == -1) { error("SERVER: receiving data (1)"); exit( 0 ); }
				if (r == 0) { error("SERVER: rejected client"); exit( 0 ); }
			}
			terminal = strstr( buffer, "@@" ) - buffer;
			buffer[ terminal ] = '\0';

			/* Get the key and message */

			key = strtok(buffer, "|");
			msg = buffer + strlen(key) + 1;

			/* Decrypt the message */

			my_decrypt(msg, key);

			memset( message, '\0', sizeof( message ));
			strcpy( message, msg );
			strcat( message, "@@"); 

			/* Send encrypted message back */

			total = 0;
			while( total < strlen( message )) {
				total += r = send( connection, message + total, sizeof( partmsg ), 0 );
				if( r == 0 ) { break; }
				if( r < 0 ) { error( "writing to socket" ); break; }
			}
			close( connection );

			exit( 0 );
		}
		else {
			pid_list[ pid_c++ ] = child;
			close( connection );
		}
		int i;
		int check_exit_status = 0;
		for (i = 0; i < pid_c; i++ ) {
			if( waitpid( pid_list[ i ], &check_exit_status, WNOHANG ) != 0 ) {
				int j;
				for (j = i; j < pid_c - 1; j++)
					pid_list[ j ] = pid_list[ j + 1 ];
				pid_c--;
			}
		}
	}
	close( listener );
	return 0;

}
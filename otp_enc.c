#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <netdb.h>

#define BUFSIZE 200000

void error(const char *msg) { fprintf(stderr, "Error: %s\n", msg); } // Error function used for reporting issues

char 	buffer[ BUFSIZE ];
char 	msgbuffer[ BUFSIZE ];
char 	keybuffer[ BUFSIZE ];
char 	errormsg[ 256 ];

int main ( int argc, char ** argv ) {
	if (argc < 4) { fprintf(stderr, "usage: %s [plaintext] [key] [port]\n", argv[ 0 ]); exit( 0 ); }
	int 	i, targetFd, socketFd, keyFd, textFd;
	struct 	sockaddr_in server, client; 
	struct 	hostent *serverHostInfo;
	size_t 	address_size;
	char * 	key = NULL;
	int 	key_len;
	char 	partmsg[ 2048 ];
	char 	* header = "ENC@@";
	int 	charsWritten, charsRead;
	ssize_t r, total;

	memset( errormsg, '\0', sizeof( errormsg ));

	if( (keyFd = open( argv[ 2 ], O_RDONLY )) < 0) { error("cannot open key for input"); exit( 1 ); }
	if( (textFd = open( argv[ 1 ], O_RDONLY )) < 0) { error("cannot open textfile for input"); exit( 1 ); }


	memset( keybuffer, '\0', sizeof( keybuffer ));
	ssize_t keylen = read( keyFd, keybuffer, sizeof( keybuffer ));

	memset( msgbuffer, '\0', sizeof( msgbuffer )); 
	ssize_t msglen = read( textFd, msgbuffer, sizeof( msgbuffer ));
	close(keyFd); close(textFd);

	if ( keylen < msglen ) {
		error("key is too short"); exit( 1 );
	}

	/* If the key is too short, exit 1 */

	i = 0;
	while( keybuffer[ i ] != '\n' ) {
		if( keybuffer[ i ] != 32 && (keybuffer[ i ] < 65 || keybuffer[ i ] > 90)) {
			fprintf( stderr, "%s error: input contains bad characters\n", argv[ 0 ] ); exit( 1 );
		}
		i++;
	}
	i = 0;
	while (msgbuffer[ i ] != '\n') {
		if( msgbuffer[ i ] != 32 && (msgbuffer[ i ] < 65 || msgbuffer[ i ] > 90)) {
			fprintf( stderr, "%s error: input contains bad characters\n", argv[ 0 ] ); exit( 1 );
		}
		i++;
	}

	/* Get host entity */

	memset(buffer, '\0', BUFSIZE );
	serverHostInfo = gethostbyname("localhost");
	if (serverHostInfo == NULL) { error( "no such host exists" ); exit( 1 ); }
	
	/* Set up server sockaddr struct */
	
	memset((char *) &server, '\0', sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons( atoi(argv[ 3 ]) );
	memcpy( (char *) &server.sin_addr.s_addr, (char *) serverHostInfo->h_addr, serverHostInfo->h_length );
	
	/* Create socket */

	socketFd = socket( AF_INET, SOCK_STREAM, 0 );
	if( socketFd < 0 ) { error("opening socket"); exit( 1 ); }
	if( connect( socketFd, ( struct sockaddr * ) &server, sizeof( server )) < 0 ) { 
		fprintf(stderr, "Error: could not contact otp_enc_d on port %d\n", server.sin_port);
		exit( 2 );
	}
	
	/* To get started - Send header */

	memset(buffer, '\0', sizeof( buffer ));
	strcpy( buffer, header );
	total = 0;
	while( total < strlen( buffer )) {
		total += r = send( socketFd, buffer + total, sizeof( partmsg ), 0 );
		if( r == 0 ) { break; }
		if( r < 0 ) { error( "writing to socket" ); break; }
	}

	/* Response from server */

	memset( buffer , '\0', sizeof(buffer));
	while( strstr( buffer, "@@") == NULL ) {
		memset( partmsg , '\0', sizeof( partmsg ));
		charsRead = recv( socketFd, partmsg, sizeof( partmsg ) - 1, 0);
		strcat(buffer, partmsg);
		if (charsRead == -1) { error("CLIENT: receiving data (1)"); exit( 0 ); }
		if (charsRead == 0) { error("CLIENT: rejected by server (1)"); exit( 0 ); }
	}
	int terminal = strstr(buffer, "@@") - buffer;
	buffer[ terminal ] = '\0';

	/* Extra Precaution - Exit if message from server is not what we expected */

	if( strcmp (buffer, "ENC_D") != 0 ) {
		error( "Unexpected header"); exit( 0 );
	}

	/* Process was not rejected by server, continue sending message */

	/* Add the Key and Msg file to buffer with correct formating - '|' b/t Key and Msg and '@@' terminating buffer */

	memset( buffer, '\0', BUFSIZE );
	strcpy( buffer, keybuffer );
	buffer[ strcspn( buffer, "\n") ] = '|';
	strcat( buffer, msgbuffer );
	buffer[ strcspn( buffer, "\n") ] = '\0';
	strcat( buffer, "@@" );

	/* Fire message to be encrypted */

	total = 0;
	while( total < strlen( buffer )) {
		total += r = send( socketFd, buffer + total, sizeof( partmsg ), 0 );
		if( r == 0 ) { break; }
		if( r < 0 ) { error( "writing to socket" ); break; }
	}

	/* Receive Encrypted Message */

	memset( buffer , '\0', sizeof(buffer));
	while( strstr( buffer, "@@") == NULL ) {
		memset( partmsg , '\0', sizeof( partmsg ));
		charsRead = recv( socketFd, partmsg, sizeof( partmsg ) - 1, 0);
		printf("charsRead = %d\n", charsRead);
		strcat(buffer, partmsg);
		if (charsRead == -1) { error("CLIENT: receiving data (2)"); exit( 0 ); }
		if (charsRead == 0) { error("CLIENT: rejected by server(2)"); exit( 0 ); }
	}
	terminal = strstr(buffer, "@@") - buffer;
	buffer[ terminal ] = '\0';
	printf("%s\n", buffer);

	close( socketFd );
	return 0;
}

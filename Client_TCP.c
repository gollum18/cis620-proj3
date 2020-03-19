// Internet domain, connection-oriented CLIENT
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <wait.h>

#define PORT    7777                
#define MAX     1024               

static char buf[MAX];              

int
main( int argc, char *argv[] ) {
  int             orig_sock,           // Original socket in client
                  len;                 // Misc. counter
  struct sockaddr_in
                  serv_adr;            // Internet addr of server
  struct hostent  *host;               // The host (server) info
  if ( argc != 2 ) {                   // Check cmd line for host name
    printf("usage: %s server \n", argv[0]);
    return 1;
  }
  host = gethostbyname(argv[1]);       // Obtain host (server) info
  if (host == (struct hostent *) NULL ) {
    perror("gethostbyname ");
    return 2;
  }
  serv_adr.sin_family = AF_INET;                 // Set address type
  memcpy(&serv_adr.sin_addr, host->h_addr, host->h_length);
  serv_adr.sin_port   = ntohs( PORT );           
                                       
  if ((orig_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("generate error");
    return 3;
  }                                    // CONNECT
  if (connect( orig_sock,(struct sockaddr *)&serv_adr,
               sizeof(serv_adr)) < 0) {
    perror("connect error");
    return 4;
  }
  do {                                 // Process
    write(1,"> ", 2);
    if ((len=read(0, buf, MAX)) > 0) {
      send(orig_sock, buf, len, 0);
      if ((len=recv(orig_sock, buf, len,0)) > 0 )
        write(1, buf, len);
    }
  } while( buf[0] != '.' );            // until end of input
  close(orig_sock);
  return 0;
}

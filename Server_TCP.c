// Internet domain, connection-oriented SERVER
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <wait.h>
#include <ctype.h>

#define PORT    7777                
#define MAX     1024               

static char buf[MAX];              

void signal_catcher(int the_sig){
  wait(0);                             // cleanup the zombie 
}
int
main(  ) {
  int             orig_sock,           // Original socket in server
                  new_sock;            // New socket from connect
  socklen_t       clnt_len;            // Length of client address
  struct sockaddr_in  clnt_adr, serv_adr; // client and  server addresses
  int             len, i;              // Misc counters, etc.
                                       // Catch when child terminates
  if (signal(SIGCHLD , signal_catcher) == SIG_ERR) {
    perror("SIGCHLD"); return 1;
  }
  if ((orig_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("generate error"); return 2;
  }
  serv_adr.sin_family      = AF_INET;            // Set address type
  serv_adr.sin_addr.s_addr = INADDR_ANY;  // Any interface
  serv_adr.sin_port        = ntohs(PORT);        
                                                 // BIND
  if (bind( orig_sock, (struct sockaddr *) &serv_adr,
            sizeof(serv_adr)) < 0){
    close(orig_sock);
    perror("bind error"); return 3;
  }
  if (listen(orig_sock, 5) < 0 ) {               // LISTEN
    close (orig_sock);
    perror("listen error"); return 4;
  }
  do {
    clnt_len = sizeof(clnt_adr);                 // ACCEPT a connect
    if ((new_sock = accept( orig_sock, (struct sockaddr *) &clnt_adr,
                            &clnt_len)) < 0) {
      close(orig_sock);
      perror("accept error"); return 5;
    }
    if ( fork( ) == 0 ) {                        // Generate a CHILD
      close(orig_sock);
      while ( (len=recv(new_sock, buf, MAX, 0)) > 0 ){
        for (i=0; i < len; ++i)                  // Change the case
          buf[i] = toupper(buf[i]);
        send(new_sock, buf, len, 0);               // Write back to socket
        if ( buf[0] == '.' ) break;              // Are we done yet?
      }
      close(new_sock);                           // In CHILD process
      return 0;
    } else
      close(new_sock);                           // In PARENT process
  } while( 1 );                               // FOREVER
  return 0;
}

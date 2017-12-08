#include "blather.h"
//main and signal handlers
/*Implement the server which manages the interactions between clients in this
file making use of the service functions in server.c to get the job done.
The server should run indefinitely without interaction
from an interactive user. To stop it, send signals.
The server should handle SIGTERM and SIGINT by shutting down gracefully:
 exit the main computation, call server_shutdown() and return 0.

// create fifo at start up for joining
REPEAT:
  check all sources
  handle a join request if on is ready
  for each client{
    if the client is ready handle data from it
  }
}*/

int signalled = 0;
void handle_signals(int sig_num){
  signalled = 1;
  return;
}

int main(int argc, char *argv[]) {
  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stdin, NULL, _IONBF, 0);
  struct sigaction my_sa = {};   // portable signal handling setup with sigaction()
  my_sa.sa_handler = handle_signals;   // run function handle_signals
  sigaction(SIGTERM, &my_sa, NULL);    // register SIGCONT with given action
  sigaction(SIGINT,  &my_sa, NULL);    // register SIGCONT with given action

  if (argc!=2){
    printf("usage: %s <name>\n",argv[0]);
    exit(1);
  }
  server_t server;
  server_start(&server, argv[1], DEFAULT_PERMS);
//  printf("The server is fired up. Waiting for people");

  while(!signalled){
    server_check_sources(&server);

    if(server_join_ready(&server)==1){
      server_handle_join(&server);
    }
    for (int i=0; i<server.n_clients; i++){
       if(server_client_ready(&server, i)==1){
         server_handle_client(&server, i);
       }
    }
  }
  server_shutdown(&server);
  return 0;
}

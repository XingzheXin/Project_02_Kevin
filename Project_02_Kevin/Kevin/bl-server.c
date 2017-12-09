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
int alarmed = 0;
server_t server;

void handle_signals(int sig_num){
  if(sig_num == SIGINT || sig_num == SIGTERM) {
    printf("INT/TERM signal received. setting flag...\n");
    signalled = 1;
    server_shutdown(&server);
    exit(1);
    return;
  }
  else if(sig_num == SIGALRM) {
    printf("Alarm just went off\n");
    // alarmed = 1;
    server_remove_disconnected(&server, DISCONNECT_SECS);
    alarm(10);
  }
  return;
}

int main(int argc, char *argv[]) {

  // check inputs
  if (argc < 2){
    printf("usage: %s <name>\n",argv[0]);
    exit(1);
  }

  // setvbuf(stdout, NULL, _IONBF, 0);
  struct sigaction my_sa = {};   // portable signal handling setup with sigaction()
  my_sa.sa_handler = handle_signals;   // run function handle_signals
  sigemptyset(&my_sa.sa_mask);
  my_sa.sa_flags = SA_RESTART;
  sigaction(SIGTERM, &my_sa, NULL);    // register SIGCONT with given action
  sigaction(SIGINT,  &my_sa, NULL);    // register SIGCONT with given action
  sigaction(SIGALRM, &my_sa, NULL);

  server_start(&server, argv[1], DEFAULT_PERMS);

  //alarm(10);
  while(!signalled){
    // printf("001\n");
    // if(alarmed) {
    //   printf("I am bing alarmed\n");
    //   alarmed = 0;
    //   server_remove_disconnected(&server, DISCONNECT_SECS);
    // }
    server_check_sources(&server);
    if(server_join_ready(&server)) {
      server_handle_join(&server);
    }
    // printf("002\n");
    for (int i=0; i<server.n_clients; i++) {
       if(server_client_ready(&server, i)) {
         server_handle_client(&server, i);
       }
    }
    //server_remove_disconnected(&server, DISCONNECT_SECS);
    // printf("signal = %d\n", signalled);
  }

  server_shutdown(&server);
  return 0;
}

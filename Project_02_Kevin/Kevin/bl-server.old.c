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
pthread_t ping_thread;

void sigint_handler(int signum) {
  signalled = 1;
  //pthread_cancel(ping_thread);
  server_shutdown(&server);
  exit(1);
  return NULL;
}

void sigterm_handler(int signum) {
  signalled = 1;
  return;
}

void sigalrm_handler(int signum) {
  signal(SIGALRM, sigalrm_handler);
  alarmed = 1;
  alarm(1);
  return NULL;
}

void *threadproc(void *arg)
{
    while(!signalled)
    {
        sleep(1);
        server_remove_disconnected(&server, DISCONNECT_SECS);
    }
    return 0;
}

// void handle_signals(int sig_num){
//   if(sig_num == SIGINT || sig_num == SIGTERM) {
//     signalled = 1;
//     server_shutdown(&server);
//     exit(1);
//     return;
//   }
//   else if(sig_num == SIGALRM) {
//     printf("Alarm just went off\n");
//     alarmed = 1;
//     // server_remove_disconnected(&server, DISCONNECT_SECS);
//     alarm(ALARM_INTERVAL);
//   }
//   return;
// }


int main(int argc, char *argv[]) {

  // check inputs
  if (argc < 2){
    printf("usage: %s <name>\n",argv[0]);
    exit(1);
  }
  pthread_t ping_thread;
  memset(&ping_thread, 0, sizeof(pthread_t));
  setvbuf(stdout, NULL, _IONBF, 0);
  // struct sigaction my_sa = {};   // portable signal handling setup with sigaction()
  // memset(&my_sa, 0, sizeof(sigaction));
  // my_sa.sa_handler = handle_signals;   // run function handle_signals
  // sigemptyset(&my_sa.sa_mask);
  // my_sa.sa_flags = SA_RESTART;
  // sigaction(SIGTERM, &my_sa, NULL);    // register SIGCONT with given action
  // sigaction(SIGINT,  &my_sa, NULL);    // register SIGCONT with given action
  // sigaction(SIGALRM, &my_sa, NULL);

  signal(SIGINT, sigint_handler);
  signal(SIGALRM, sigalrm_handler);
  signal(SIGTERM, sigterm_handler);
  server_start(&server, argv[1], DEFAULT_PERMS);

  pthread_create(&ping_thread, NULL, &threadproc, NULL);
  // alarm(1);
  while(1){
    // if(alarmed) {
    //   alarmed = 0;
    //   server_remove_disconnected(&server, DISCONNECT_SECS);
    // }
    //server_tick(&server);
    server_check_sources(&server);
    if(server_join_ready(&server)) {
      server_handle_join(&server);
    }
    server_write_who(&server);
    // printf("002\n");
    for (int i=0; i<server.n_clients; i++) {
       if(server_client_ready(&server, i)) {
         server_handle_client(&server, i);
       }
    }
    //
    // if(alarmed) {
    //   printf("I am being alarmed\n");
    //   alarmed = 0;
    //   //alarm(0);
    //   server_remove_disconnected(&server, DISCONNECT_SECS);
    // }
    //server_remove_disconnected(&server, DISCONNECT_SECS);
    // printf("signal = %d\n", signalled);
  }
  // pthread_join(threadproc, NULL);
  server_shutdown(&server);
  return 0;
}

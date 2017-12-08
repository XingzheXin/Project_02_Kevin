#include "blather.h"
#include "stdio.h"
#include "string.h"

simpio_t simpio_actual;
simpio_t *simpio = &simpio_actual;

client_t client_actual;
client_t *client = &client_actual;

pthread_t user_thread;          // thread managing user input
pthread_t background_thread;

void *user_worker(void *arg){
  while(!simpio->end_of_input){
    simpio_reset(simpio);
    iprintf(simpio, "");                                          // print prompt
    while(!simpio->line_ready && !simpio->end_of_input) {          // read until line is complete
      simpio_get_char(simpio);
    }
    if(simpio->line_ready){
      // iprintf(simpio, "%2d You entered: %s\n",count,simpio->buf);
      // count++;
      iprintf(simpio, "");
      mesg_t msg;
      msg.kind = BL_MESG;
      strcpy(msg.body, simpio->buf);
      write(client_actual.to_server_fd, &msg, sizeof(mesg_t));
    }
  }
  pthread_cancel(background_thread); // kill the background thread
  return NULL;
}

void *background_worker(void *arg){
  while(1) {
    sleep(1);
    mesg_t msg;
    read(client_actual.to_client_fd, &msg, sizeof(mesg_t));
    switch(msg.kind) {
      case BL_MESG:
        iprintf(simpio, "[%s] : %s\n", msg.name, msg.body);
      case BL_JOINED:
        iprintf(simpio, "-- %s JOINED --\n", msg.name);
      case BL_DEPARTED:
        iprintf(simpio, "-- %s DEPARTED --\n", msg.name);
      case BL_SHUTDOWN:
        iprintf(simpio, "!!! server is shutting down !!!");
    }
  }
  return NULL;
}

int main(int argc, char *argv[]) {

	if(argc < 3) {
		printf("usage: %s <server_name> <user_name>\n", argv[0]);
		exit(1);
	}
  pid_t pid = getpid();
  printf("%d\n", pid);

	// construct a join_t structure ready to be sent to server fifo
	join_t join;
	char join_fname[MAXPATH];

  strcpy(client_actual.name, argv[2]);
  sprintf(join_fname, "%s.fifo", argv[1]);
	// Sets to_client_actual_fname to the following format
	// pid.client.fifo
  sprintf(client_actual.to_client_fname, "%d_client.fifo", pid);

	// Sets to_server_fname to the following format:
	// pid.server.fifo
  sprintf(client_actual.to_server_fname, "%d_server.fifo", pid);

	// Sets the fields of the outgoing join_t structure
	strcpy(join.name, client_actual.name);
	strcpy(join.to_client_fname, client_actual.to_client_fname);
	strcpy(join.to_server_fname, client_actual.to_server_fname);

  // remove existing fifos
  remove(client_actual.to_client_fname);
  remove(client_actual.to_server_fname);

  // make new fifos
  mkfifo(client_actual.to_client_fname, DEFAULT_PERMS);
  mkfifo(client_actual.to_server_fname, DEFAULT_PERMS);

  // Open the two fifos that are created
  client_actual.to_client_fd = open(client_actual.to_client_fname, O_RDWR);
  client_actual.to_server_fd = open(client_actual.to_server_fname, O_RDWR);
  printf("Here_06\n");
	int join_fd = open(join_fname, O_RDWR);
  printf("join_fname: %s\n", join_fname);
  printf("join.name: %s\n", join.name);
  printf("join.cl: %s\n", join.to_client_fname);
  printf("join.sv: %s\n", join.to_server_fname);

  printf("Writing join_t struct to server....\n");
	// Write the join_t structure to <server_name>.fifo
	int check = write(join_fd, &join, sizeof(join_t));
  if(check == -1) {
    perror("ERRORNO indicates: ");
    exit(1);
  }

	// ***************************************************
  char prompt[MAXNAME];
  snprintf(prompt, MAXNAME, "%s>> ","fgnd"); // create a prompt string
  simpio_set_prompt(simpio, prompt);         // set the prompt
  simpio_reset(simpio);                      // initialize io
  simpio_noncanonical_terminal_mode();       // set the terminal into a compatible mode

  pthread_create(&user_thread,   NULL, user_worker,   NULL);     // start user thread to read input
  pthread_create(&background_thread, NULL, background_worker, NULL);
  pthread_join(user_thread, NULL);
  pthread_join(background_thread, NULL);

  simpio_reset_terminal_mode();
  printf("\n");                 // newline just to make returning to the terminal prettier

	// ***************************************************
	return 0;
}

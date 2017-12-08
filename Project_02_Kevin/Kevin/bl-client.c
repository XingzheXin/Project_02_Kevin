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
  //int count = 1;
  while(!simpio->end_of_input){
    simpio_reset(simpio);
    iprintf(simpio, "");                                          // print prompt
    while(!simpio->line_ready && !simpio->end_of_input) {          // read until line is complete
      simpio_get_char(simpio);
    }
    if(simpio->line_ready){
      //iprintf(simpio, "%2d You entered: %s\n",count,simpio->buf);
      //count++;
    }
  }

  pthread_cancel(background_thread); // kill the background thread
  return NULL;
}

void *background_worker(void *arg){
  char *text[3] = {
    "Background text #1",
    "Background text #2",
    "Background text #3",
  };
  for(int i=0; ; i++){
    sleep(3);
    iprintf(simpio, "BKGND: %s\n",text[i % 3]);
  }
  return NULL;
}


// Signal Handler here
// Do we even need a signal handler?
// If we use signal handlers we might need to expand
// join_t a little bit to add a "pid" field such that
// the server knows who to send alert to?
// Or we can just not use signals and let all the clients do "busy wait"
// i.e. they all hang at "read"

int signalled = 0;
void handle_signals(int sig_num) {
	// yet to implement
}


int main(int argc, char *argv[]) {

	if(argc < 3) {
		printf("usage: %s <server_name> <user_name>\n", argv[0]);
		exit(1);
	}

  client_t client;
	// construct a join_t structure ready to be sent to server fifo
	join_t client_info;

	char server_fname[MAXPATH];
	char to_client_fname[MAXPATH];
	char to_server_fname[MAXPATH];

  strcpy(client.name, argv[2]);
	// Sets to_client_fname to the following format
	// <server_name>_to_<user_name>.fifo
	strcpy(client.to_client_fname, argv[1]);
	strcat(client.to_client_fname, "_to_");
	strcat(client.to_client_fname, argv[2]);
	strcat(client.to_client_fname, ".fifo");

	// Sets to_server_fname to the following format:
	// <user_name>_to_<server_name.fifo
	strcpy(client.to_server_fname, argv[2]);
	strcat(client.to_server_fname, "_to_");
	strcat(client.to_server_fname, argv[1]);
	strcat(client.to_server_fname, ".fifo");

	// Sets the fields of the outgoing join_t structure
	strcpy(client_info.name, client.name);
	strcpy(client_info.to_client_fname, client.to_client_fname);
	strcpy(client_info.to_server_fname, client.to_server_fname);

	// Open <server_name>.fifo
	strcpy(server_fname, argv[1]);
	strcat(server_fname, ".fifo");

  // remove existing fifos
  remove(to_client_fname);
  remove(to_server_fname);

  // make new fifos
  mkfifo(to_client_fname, DEFAULT_PERMS);
  mkfifo(to_server_fname, DEFAULT_PERMS);

  // Open the two fifos that are created
  client.to_client_fd = open(to_client_fname, O_RDWR);
  client.to_server_fd = open(to_server_fname, O_RDWR);

	// Maybe should include some error checking here
	// What if the fifo does not exist
	int join_fd = open(server_fname, O_RDWR);

  printf("Writing join_t struct to server....");
  fflush(STDOUT_FILENO);
	// Write the join_t structure to <server_name>.fifo
	int check = write(join_fd, &client_info, sizeof(join_t));
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

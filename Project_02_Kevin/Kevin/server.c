#include "blather.h"

client_t *server_get_client(server_t *server, int idx) {
  // Error Checking
  if (idx >= MAXCLIENTS) {
    printf("Requested client out of array bounds. Exiting...");
    exit(1);
  }

  return &server->client[idx];
}

void server_start(server_t *server, char *server_name, int perms) {
  // create a string with ".fifo" appended to server_name
  char fifo_name[MAXPATH+5];
  strcpy(fifo_name, server_name);
  strcat(fifo_name, ".fifo");
  strcpy(server->server_name, server_name);

  // remove old fifo
  remove(fifo_name);

  // create new fifo
  if (mkfifo(fifo_name, perms)==-1){
    perror("couldn't make fifo");
  };

  // initialize join_fd
  if(server->join_fd = open(fifo_name, O_RDWR)==-1){
    perror("Failed to open join fifo");
  }

  // initialize server name
  if(strlen(server_name) >= MAXPATH) {
    printf("Server name too long, unable to initialize");
    exit(1);
  }

  printf("The server is fired up. Waiting for people\n");
  server->n_clients = 0;

  return;
}

void server_shutdown(server_t *server) {
  char fifo_name[MAXPATH+5];
	strcpy(fifo_name, server->server_name);
	strcat(fifo_name, ".fifo");

 	//close the join fifo
	close(server->join_fd);

	// remove the join fifo so no further clients can join
	remove(fifo_name);

	// remove all existing clients
	for(int i = 0 ; i < server->n_clients; i++) {
		server_remove_client(server, i);
	}

  mesg_t notice;
  notice.kind = BL_SHUTDOWN;
  server_broadcast(server, &notice);// Send a BL_SHUTDOWN message to all
}

//******************************************************************************
int server_add_client(server_t *server, join_t *join) {
	int n = server->n_clients;
	// check if server has any space left
	if(server->n_clients >= MAXCLIENTS)	{
		printf("Failed to add client, no space left on server");
		return -1;
	}

	// add one to the client counter
	server->n_clients += 1;

	// set client attributes according to join
	strcpy(server->client[n-1].to_client_fname, join->to_client_fname);
	strcpy(server->client[n-1].to_server_fname, join->to_server_fname);
	strcpy(server->client[n-1].to_server_fname, join->name);

  server->client[n-1].to_server_fd = open(server->client[n-1].to_server_fname, O_RDWR);
  server->client[n-1].to_client_fd = open(server->client[n-1].to_client_fname, O_RDWR);
  server->client[n-1].data_ready=0;
  printf("%d\n", server->n_clients);
  printf("%s has joined\n", server->client[n-1].name);
	return 0;
}

int server_remove_client(server_t *server, int idx) {
	// error checking
	if(idx < 0 || idx > MAXCLIENTS) {
		printf("Cannot aquire client at index %d, array index out of bounds.", idx);
		exit(1);
	}


	// close relative fifos
	close(server->client[idx].to_client_fd);
	close(server->client[idx].to_server_fd);

	// remove fifos
	remove(server->client[idx].to_client_fname);
	remove(server->client[idx].to_server_fname);

	// shift the rest of the clients down by one index
	// first, check if it's the end of client array
	if(idx < server->n_clients-1) {
		for(int i = idx; i < server->n_clients; i++) {
			server->client[idx] = server->client[idx+1];
		}
	}
	// decrease client counter in server
	server->n_clients -= 1;
}

int server_broadcast(server_t *server, mesg_t *mesg) {
  for (int i=0; i< server->n_clients; i++){
    write(server->client[i].to_client_fd, &mesg, sizeof(mesg_t));
  }
  return 0;
}

void server_check_sources(server_t *server){
  int maxfd;
  int nfds;
  fd_set readset;
  maxfd = server->client[0].to_server_fd;
  for (int i=0; i<server->n_clients; i++){
    if (server->client[i].to_server_fd > maxfd){
      maxfd = server->client[i].to_server_fd;
    }
    FD_SET(server->client[i].to_server_fd, &readset);
  }
  FD_SET(server->join_fd, &readset);
  nfds = select(maxfd+1, &readset, NULL, NULL, NULL);
  if(nfds<0){
    perror("select() failed");
    exit(1);
  }
  //server->join_ready = 0;
  for(int i=0; i<server->n_clients; i++){
      if (FD_ISSET(server->client[i].to_server_fd, &readset)!=0){
          server->client[i].data_ready = 1;  //1 for ready
      }
  }
  if((server->n_clients<MAXCLIENTS) && (FD_ISSET(server->join_fd, &readset)!=0)) {
    server->join_ready = 1;
  } else{
    server->join_ready = 0;
    }
  return;
// Checks all sources of data for the server to determine if any are
// ready for reading. Sets the servers join_ready flag and the
// data_ready flags of each of client if data is ready for them.
// Makes use of the select() system call to efficiently determine
// which sources are ready.
}

int server_join_ready(server_t *server){

  return server->join_ready;// Return the join_ready flag from the server which indicates whether
// a call to server_handle_join() is safe.
}

int server_handle_join(server_t *server){
  join_t client_info;
  read(server->join_fd, &client_info, sizeof(join_t));
  server_add_client(server, &client_info);
  server->join_ready = 0;
  return server->join_ready;
}

int server_client_ready(server_t *server, int idx){
  // Return the data_ready field of the given client which indicates
  return server->client[idx].data_ready;// whether the client has data ready to be read from it.
}

int server_handle_client(server_t *server, int idx){
  if(server_client_ready(server, idx)) {// Process a message from the specified client. This function should
      mesg_t mesg;
      int n = read(server->client[idx].to_server_fd, &mesg, sizeof(mesg_t)); // only be called if server_client_ready() returns true.
      if(mesg.kind == 10){   // Departure and Message types should be broadcast to all other clients.
        server_broadcast(server, &mesg);
        return 0;
      }
      else {//no name no body
          strcpy(mesg.name, server->client[idx].name);
          server_broadcast(server, &mesg);
      }
     // server time_sec.
  }
  return 0;
  // ADVANCED: Update the last_contact_time of the client to the current
  // Ping responses should only change the last_contact_time below. Behavior
  // for other message types is not specified. Clear the client's data_ready flag so it has value 0.
}

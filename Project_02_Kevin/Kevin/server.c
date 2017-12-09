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
  char fifo_name[MAXPATH];
  sprintf(fifo_name, "%s.fifo", server_name);
  // remove old fifo
  remove(fifo_name);

  // create new fifo
  if (mkfifo(fifo_name, perms)==-1){
    perror("couldn't make fifo");
  };

  // initialize join_fd
  server->join_fd = open(fifo_name, O_RDWR);
  if(server->join_fd == -1){
    perror("Failed to open join fifo");
  }

  // initialize server name
  if(strlen(server_name) >= MAXPATH) {
    printf("Server name too long, unable to initialize");
    exit(1);
  }

  server->n_clients = 0;

  printf("The server is fired up. Waiting for people\n");


  return;
}

void server_shutdown(server_t *server) {
  char fifo_name[MAXPATH];
	sprintf(fifo_name, "%s.fifo", server->server_name);

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

  return;
}

//******************************************************************************
int server_add_client(server_t *server, join_t *join) {
  int timestamp = (int)time(NULL);
	int n = server->n_clients;
	// check if server has any space left
	if(server->n_clients >= MAXCLIENTS)	{
		printf("Failed to add client, no space left on server");
		return -1;
	}

	// set client attributes according to join
	strcpy(server->client[n].to_client_fname, join->to_client_fname);
	strcpy(server->client[n].to_server_fname, join->to_server_fname);
	strcpy(server->client[n].name, join->name);

  server->client[n].to_server_fd = open(server->client[n].to_server_fname, O_RDWR);
  server->client[n].to_client_fd = open(server->client[n].to_client_fname, O_RDWR);
  server->client[n].data_ready=0;
  server->n_clients += 1;

  mesg_t msg;
  strcpy(msg.name, join->name);
  msg.kind = BL_JOINED;
  server_broadcast(server, &msg);
	return 0;
}

int server_remove_client(server_t *server, int idx) {
	// error checking
	if(idx < 0 || idx >= MAXCLIENTS) {
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

  return 0;
}

int server_broadcast(server_t *server, mesg_t *mesg) {
  printf("Broadcasting...\n");
  for (int i=0; i< server->n_clients; i++){
    write(server->client[i].to_client_fd, mesg, sizeof(mesg_t));
  }
  printf("Finished Broadcasting\n");
  return 0;
}

void server_check_sources(server_t *server){
  printf("server->join_fd = %d\n", server->join_fd);
  // return if there are no exisiting clients.
  int maxfd = server->join_fd;
  fd_set readset;
  FD_ZERO(&readset);
  // Find the maximum file descriptor
  for (int i=0; i<server->n_clients; i++){
    if (server->client[i].to_server_fd > maxfd){
      maxfd = server->client[i].to_server_fd;
    }
    // Add all to_server fds to the readset
    FD_SET(server->client[i].to_server_fd, &readset);
  }
  //
  // if(maxfd < server->join_fd)  {
  //   maxfd = server->join_fd;
  // }

  // Add the join_fd to readset
  FD_SET(server->join_fd, &readset);
  //printf("Here_02fwfwf\n");
  //printf("maxfd+1 = %d", maxfd+1);
  // struct timeval tv = {};
  // tv.tv_sec = 10;
  // tv.tv_usec = 0;
  // Now go to sleep until something is ready for reading
  int nfds = select(maxfd+1, &readset, NULL, NULL, NULL);

  //printf("Here_03\n");
  // Wakes up because something is ready
  // Iterates through every single to_server fifo and look for a message.
  for(int i=0; i<server->n_clients; i++){
      if (FD_ISSET(server->client[i].to_server_fd, &readset)){
          server->client[i].data_ready = 1;  //1 for ready
      }
  }
  //printf("Here_03\n");
  // This sets the join_ready flag for the server
  // If there is a join ready, join_ready will be a non-zero value
  // If there is none, FD_ISSET will set join_ready flag to zero.
  server->join_ready = FD_ISSET(server->join_fd, &readset);
  //printf("here__23: %d\n",server->join_ready);
  //printf("Here_04\n");
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
  join_t join;
  int nread = read(server->join_fd, &join, sizeof(join_t));
  if(nread == -1) {
    perror("ERRORNO Indicates: \n");
  }

  server_add_client(server, &join);
  server->join_ready = 0;
  return server->join_ready;
}

int server_client_ready(server_t *server, int idx){
  // Return the data_ready field of the given client which indicates
  return server->client[idx].data_ready; // whether the client has data ready to be read from it.
}

int server_handle_client(server_t *server, int idx){
  printf("Here in server_handle_client...\n");
  if(server_client_ready(server, idx)) {
      printf("just to make sure\n");
      server->client[idx].data_ready = 0;
      mesg_t mesg;
      int n = read(server->client[idx].to_server_fd, &mesg, sizeof(mesg_t)); // only be called if server_client_ready() returns true.
      printf("%s just sent mesg.kind = %d\n", server->client[idx].name, mesg.kind);
      switch(mesg.kind) {
        case BL_MESG:
        case BL_DEPARTED:
          server_broadcast(server, &mesg);
          break;
        case BL_PING:
          server->client[idx].last_contact_time = server->time_sec;
          printf("Client Number %d, Last contacted time: %d.\n", idx, server->client[idx].last_contact_time);
          break;
      }

  }
  return 0;
  // ADVANCED: Update the last_contact_time of the client to the current
  // Ping responses should only change the last_contact_time below. Behavior
  // for other message types is not specified. Clear the client's data_ready flag so it has value 0.
}

void server_tick(server_t *server) {
  server-> time_sec = (int)time(NULL);
  return NULL;
}

void server_ping_clients(server_t *server) {
  mesg_t msg;
  msg.kind = BL_PING;
  server_broadcast(server, &msg);
  return NULL;
}

void server_remove_disconnected(server_t *server, int disconnect_secs) {
  printf("Pinging all the existing clients...\n");
  server_tick(server);
  server_ping_clients(server);
  for(int i = 0; i < server->n_clients; i++) {
    printf("The current server time : %d\n", server->time_sec);
    printf("Client %d's last contact time : %d\n", i, server->client[i].last_contact_time);
    if(server->time_sec - server->client[i].last_contact_time >= disconnect_secs) {
      mesg_t msg;
      msg.kind = BL_DISCONNECTED;
      strcpy(msg.name, server->client[i].name);
      server_remove_client(server, i);
      server_broadcast(server, &msg);
      printf("%s has disconnect from server\n", msg.name);
    }
  }
  printf("Finished pinging all the existing clients...\n");
  return NULL;
}

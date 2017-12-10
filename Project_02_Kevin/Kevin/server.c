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
  strcpy(server->server_name, server_name);
  server->n_clients = 0;
  server->time_sec = (int)time(NULL);
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

  //****************************************
  //Make the log file
  char log_fname[MAXPATH];
  sprintf(log_fname, "%s.log", server_name);
  server->log_fd = open(log_fname, O_RDWR | O_CREAT);
  printf("server_start(): end\n");
  return;
}

void server_shutdown(server_t *server) {
  char fifo_name[MAXPATH];
  char log_fname[MAXPATH];

	sprintf(fifo_name, "%s.fifo", server->server_name);
  sprintf(log_fname, "%s.log", server->server_name);
 	//close the join fifo
	close(server->join_fd);
  close(server->log_fd);
	// remove the join fifo so no further clients can join
	remove(fifo_name);
  mesg_t notice;
  notice.kind = BL_SHUTDOWN;
  server_broadcast(server, &notice);// Send a BL_SHUTDOWN message to all

	// remove all existing clients
	for(int i = 0 ; i < server->n_clients; i++) {
		server_remove_client(server, i);
	}

  printf("Shutting down server...\n");
  return NULL;
}

//******************************************************************************
int server_add_client(server_t *server, join_t *join) {
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
  server->client[n].last_contact_time = server->time_sec;
  server->n_clients += 1;

  mesg_t msg;
  strcpy(msg.name, join->name);
  msg.kind = BL_JOINED;
  server_broadcast(server, &msg);

  printf("server_add_client(): %s %s %s\n", join->name, join->to_client_fname, join->to_server_fname);
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
			server->client[i] = server->client[i+1];
		}
	}
	// decrease client counter in server
	server->n_clients -= 1;

  return 0;
}

int server_broadcast(server_t *server, mesg_t *mesg) {
  for (int i=0; i< server->n_clients; i++){
    write(server->client[i].to_client_fd, mesg, sizeof(mesg_t));
  }
  if(mesg->kind!=BL_PING){
    server_log_message(server, mesg);
  }
  if(mesg->kind == BL_MESG)
    printf("server_broadcast(): %d from %s - %s\n", mesg->kind, mesg->name, mesg->body);
  else if (mesg->kind == BL_DEPARTED)
    printf("server: depated client %s\n", mesg->name);
  return 0;
}

void server_log_message(server_t *server, mesg_t *mesg){
    write(server->log_fd, mesg, sizeof(mesg_t));
    printf("Tracking completed\n");
    return;
}

void server_write_who(server_t *server){
    who_t logged_in;
    logged_in.n_clients = server->n_clients;
    for (int i=0; i<logged_in.n_clients; i++){
      strcpy(logged_in.names[i], server->client[i].name);
    }

    pwrite(server->log_fd, &logged_in, sizeof(who_t), 0);
    return;
}

void server_check_sources(server_t *server){
  int maxfd = server->join_fd;
  fd_set readset;
  FD_ZERO(&readset);
  for (int i=0; i<server->n_clients; i++){
    if (server->client[i].to_server_fd > maxfd){
      maxfd = server->client[i].to_server_fd;
    }
    // Add all to_server fds to the readset
    FD_SET(server->client[i].to_server_fd, &readset);
  }

  FD_SET(server->join_fd, &readset);

  int nfds = select(maxfd+1, &readset, NULL, NULL, NULL);

  for(int i=0; i<server->n_clients; i++){
      if (FD_ISSET(server->client[i].to_server_fd, &readset)){
          server->client[i].data_ready = 1;
      }
  }
  server->join_ready = FD_ISSET(server->join_fd, &readset);
  return;
}

int server_join_ready(server_t *server){
  return server->join_ready;
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

  printf("server_process_join()\n");
}

int server_client_ready(server_t *server, int idx){
  // Return the data_ready field of the given client which indicates
  return server->client[idx].data_ready; // whether the client has data ready to be read from it.
}

int server_handle_client(server_t *server, int idx){
  server->client[idx].last_contact_time = server->time_sec;
  if(server_client_ready(server, idx)) {
      server->client[idx].data_ready = 0;
      mesg_t mesg;
      int n = read(server->client[idx].to_server_fd, &mesg, sizeof(mesg_t)); // only be called if server_client_ready() returns true.
      //printf("%s just sent mesg.kind = %d\n", server->client[idx].name, mesg.kind);
      switch(mesg.kind) {
        case BL_MESG:
          printf("mesg received from client %d %s : %s\n", idx, mesg.name, mesg.body);
          server_broadcast(server, &mesg);
          break;
        case BL_DISCONNECTED:
          printf("%s has disconnected from server\n", mesg.name);
          server_broadcast(server, &mesg);
          server_remove_client(server, idx);
          break;
        case BL_DEPARTED:
          printf("%s has depated from chat\n", mesg.name);
          server_broadcast(server, &mesg);
          server_remove_client(server, idx);
          break;
        case BL_PING:
          server->client[idx].last_contact_time = server->time_sec;
          //printf("Client Number %d, Last contacted time: %d.\n", idx, server->client[idx].last_contact_time);
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
  //printf("Current server time is: %d\n", server->time_sec);
  return NULL;
}

void server_ping_clients(server_t *server) {
  mesg_t msg;
  msg.kind = BL_PING;
  server_broadcast(server, &msg);
  return NULL;
}

void server_remove_disconnected(server_t *server, int disconnect_secs) {
  //printf("Pinging all the existing clients...\n");
  server_tick(server);
  server_ping_clients(server);
  //sleep(3);
  for(int i = 0; i < server->n_clients; i++) {
    // printf("The current server time : %d\n", server->time_sec);
    // printf("Client %d's last contact time : %d\n", i, server->client[i].last_contact_time);
    if(server->time_sec - server->client[i].last_contact_time >= disconnect_secs) {
      mesg_t msg;
      msg.kind = BL_DISCONNECTED;
      strcpy(msg.name, server->client[i].name);
      server_broadcast(server, &msg);
      server_remove_client(server, i);
      printf("%s has disconnect from server\n", msg.name);
    }
  }
  //printf("Finished pinging all the existing clients...\n");
  return NULL;
}

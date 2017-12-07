#include "blather.h"
#include "string.h"
#include "stdio.h"

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
	mkfifo(fifo_name, S_IRUSR | S_IWUSR);

	// initialize join_fd	
	server->join_fd = open(fifo_name, O_RDWR);
	
	// initialize server name
	if(strlen(server_name) >= MAXPATH) {
		printf("Server name too long, unable to initialize");
		exit(1);
	}
	
	// set n_clients
	server->n_clients = 0;
	
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
}

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
	switch(mesg->kind) {
		case BL_MESG:
			for(int i = 0; i < server->n_clients; i++) {
				write(server->client[i].to_client_fd, mesg->name, MAXNAME);
				write(server->client[i].to_client_fd, mesg->body, MAXLINE);
			}
	}
	return 0;	
}

void server_check_sources(server_t *server) {
		
}

int server_join_ready(server_t *server) {

}

int server_handle_join(server_t *server) {

}

int server_client_ready(server_t *server, int idx) {

}

int server_handle_client(server_t *server, int idx) {

}

void server_tick(server_t *server) {

}

void server_ping_clients(server_t *server) {

}

void server_remove_disconnected(server_t *server, int disconnect_secs) {

}

void server_write_who(server_t *server) {

}

void server_log_message(server_t *server, mesg_t *mesg) {

}


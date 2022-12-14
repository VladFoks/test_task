#include <stdio.h>
#include <ev.h>
#include <netinet/in.h>
#include <stdlib.h> 
#define BUFFER_SIZE 1024

int total_clients = 0;  
int addr_len = 0;
int port_no = 0;
void accept_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);
void read_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);

int main(int argc, char **argv){
	if (argc > 2) {
		puts("Incorrect number of parameters");
		return 1;
    }
	else if (argc == 2) {
		port_no = atoi(argv[1]);
	}
	else {
		port_no = 3333;
	}
	printf("%d port nomer.\n", port_no);
	struct ev_loop *loop = ev_default_loop(0);
	int sd;
	struct sockaddr_in addr;
	addr_len = sizeof(addr);
	struct ev_io w_accept;
// Create server socket
	if( (sd = socket(PF_INET, SOCK_STREAM, 0)) < 0 ){
		perror("socket error");
		return -1;
	}
	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port_no);
	addr.sin_addr.s_addr = INADDR_ANY;
// Bind socket to address
	if (bind(sd, (struct sockaddr*) &addr, sizeof(addr)) != 0){
		perror("bind error");
	}
// Start listing on the socket
	if (listen(sd, 2) < 0){
		perror("listen error");
		return -1;
	}
// Initialize and start a watcher to accepts client requests
	ev_io_init(&w_accept, accept_cb, sd, EV_READ);
	ev_io_start(loop, &w_accept);
// Start infinite loop
		ev_loop(loop, 0);
	return 0;
}
//======================================================================
/* Accept client requests */
void accept_cb(struct ev_loop *loop, struct ev_io *watcher, int revents){
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);
	int client_sd;
	struct ev_io *w_client = (struct ev_io*) malloc (sizeof(struct ev_io));	
	if(EV_ERROR & revents){
		perror("got invalid event");
		return;
	}
// Accept client request
	client_sd = accept(watcher->fd, (struct sockaddr *)&client_addr, &client_len);
	if (client_sd < 0){
		perror("accept error");
		return;
	}
	total_clients ++ ;
	printf("Successfully connected with client.\n");
	printf("%d client(s) connected.\n", total_clients);
// Initialize and start watcher to read client requests
	ev_io_init(w_client, read_cb, client_sd, EV_READ);
	ev_io_start(loop, w_client);
}
//======================================================================
/* Read client message */
void read_cb(struct ev_loop *loop, struct ev_io *watcher, int revents){
	char buffer[BUFFER_SIZE];
	ssize_t read;
	if(EV_ERROR & revents){
		perror("got invalid event");
		return;
	}
// Receive message from client socket
	read = recv(watcher->fd, buffer, BUFFER_SIZE, 0);
	if(read < 0){
		perror("read error");
		return;
	}
	if(read == 0){
// Stop and free watchet if client socket is closing
	ev_io_stop(loop,watcher);
	free(watcher);
	perror("peer might closing");
	total_clients --; 
	printf("%d client(s) connected.\n", total_clients);
	return;
	}
	else{
		printf("message:%s\n",buffer);
	}
// Send message back to the client
	send(watcher->fd, buffer, read, 0);
	memset(&buffer,0,sizeof(buffer));
}

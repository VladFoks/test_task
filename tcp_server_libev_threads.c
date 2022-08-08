#include <stdio.h>
#include <ev.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdlib.h> 

#define BUFFER_SIZE 1024
struct ev_loop;
struct ev_io w_accept;
struct ev_io *watcher;
int total_clients = 0;  
int addr_len = 0;
int port_no = 0;
void accept_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);
void read_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);
int sd;
pthread_mutex_t lock;
pthread_t thread;

//======================================================================
/* Reverse buffer*/    
void revstr_rec(char *sstr, char *dstr, int len) { 
    if((! *sstr) || (! len) )
        return;
    revstr_rec(sstr + 1, dstr, len - 1);
    dstr[len - 1] = *sstr;
    return;
}
//======================================================================
/* Create thread*/
void *loop2thread(){
    struct ev_loop* loop2= ev_default_loop(0);
    loop2 = ev_loop_new(0);
    printf("Inside loop 2\n"); 
    ev_io_init(&w_accept, read_cb, sd, EV_READ);
    ev_io_start(loop2, &w_accept);
//	printf("Accepts client requests from loop 2\n");
//launch 2 observers
	ev_loop(loop2, 0);
    return 0;
}

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
// Create pthread	
	pthread_mutex_init(&lock, 0); 

    pthread_create(&thread, 0, loop2thread, 0); 
	printf("Thread created\n"); 
	
// Initialize and start a watcher to accepts client requests
	ev_io_init(&w_accept, accept_cb, sd, EV_READ);
	ev_io_start(loop, &w_accept);
//	printf("Accepts client requests\n");
	ev_loop(loop, 0);
	ev_io_stop(loop,watcher);
	free(watcher);
	pthread_join(thread, 0);
	pthread_mutex_destroy(&lock);
	printf("The end\n"); 
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
	if (client_sd < 0)	{
		perror("accept error");
		return;
	}
	total_clients ++ ;// Increment total_clients count
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
	pthread_mutex_lock(&lock);     
	printf("block and reverse buffer \n");
	char *sstr = NULL;
    char *dstr = NULL;
    sstr = malloc(1024);
    if(! sstr)  {
        printf("no memory . . .\n");
        return;
    }
    strcpy(sstr, buffer);
    printf("buffer: %s\n", sstr);
    dstr = malloc(1024);
    if(! dstr)  {
        printf("no memory . . .\n");
        return;
    }
    revstr_rec(sstr, dstr, strlen(sstr));  
    strcpy(buffer, dstr);
    free(sstr);
    free(dstr);  
    pthread_mutex_unlock(&lock);   
	if(read == 0){
// Stop and free watchet if client socket is closing
	ev_io_stop(loop,watcher);
	free(watcher);
	perror("peer might closing");
	total_clients --; // Decrement total_clients count
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

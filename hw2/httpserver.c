#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "libhttp.h"

/*
 * Global configuration variables.
 * You need to use these in your implementation of handle_files_request and
 * handle_proxy_request. Their values are set up in main() using the
 * command line arguments (already implemented for you).
 */
int server_port;
char *server_files_directory;
char *server_proxy_hostname;
int server_proxy_port;

char*conc(char*s1,char*s2){
  size_t len1,len2;
  char*s3;
  len1=strlen(s1);
  len2=strlen(s2);
  if(NULL==(s3=malloc(len1+len2+1))) return NULL;
  memcpy(s3,s1,len1);
  memcpy(s3+len1,s2,len2);
  s3[len1+len2]='\0';
  return s3;
}

int end_request(char*request,int n){
  return (0==strncmp(request+n-4,"\r\n\r\n",4))||(0==strncmp(request+n-9,"</html>\r\n",9));
}

/*
 * Reads an HTTP request from stream (fd), and writes an HTTP response
 * containing:
 *
 *   1) If user requested an existing file, respond with the file
 *   2) If user requested a directory and index.html exists in the directory,
 *      send the index.html file.
 *   3) If user requested a directory and index.html doesn't exist, send a list
 *      of files in the directory with links to each.
 *   4) Send a 404 Not Found response.
 */
void handle_files_request(int fd) {
  struct http_request*request=http_request_parse(fd);
  char*fullpath,*fullfullpath;
  struct stat st;
  
  if(0==strcmp("GET",request->method)){
    fullpath=conc(server_files_directory,request->path);
    if(0==stat(fullpath,&st)){ // file/dir exists
      if(S_ISREG(st.st_mode)){ // is file
	http_send_file(fd,fullpath);
      }else if(S_ISDIR(st.st_mode)){ // is dir
	fullfullpath=conc(fullpath,"/index.html");
	if(0==stat(fullfullpath,&st)){ // has an index.html
	  http_send_file(fd,fullfullpath);
	}else{ // hasnt. list files
	  DIR*d=opendir(fullpath);
	  struct dirent*di;
	  http_start_response(fd,200);
	  http_send_header(fd,"Content-type","text/html");
	  http_end_headers(fd);
	  while(NULL!=(di=readdir(d)))
	    http_send_anchor(fd,di->d_name);
	}
	free(fullfullpath);
      }
    }else{ // doesnt exist. error 404
      http_start_response(fd,404);
      http_send_header(fd,"Content-type","text/html");
      http_end_headers(fd);
      http_send_string(fd,"<center><p>ERROR 404</p></center>");
    }
    free(fullpath);
  }
}

pthread_mutex_t mutex;

void*start_act(void*arg){
  int*c=(int*)arg,n;
  char buffer[1024];
  while(0<(n=read(c[0],buffer,sizeof(buffer)))){
    pthread_mutex_lock(&mutex);
    write(c[1],buffer,n);
    pthread_mutex_unlock(&mutex);
  }
  pthread_exit(NULL);
}

/*
 * Opens a connection to the proxy target (hostname=server_proxy_hostname and
 * port=server_proxy_port) and relays traffic to/from the stream fd and the
 * proxy target. HTTP requests from the client (fd) should be sent to the
 * proxy target, and HTTP responses from the proxy target should be sent to
 * the client (fd).
 *
 *   +--------+     +------------+     +--------------+
 *   | client | <-> | httpserver | <-> | proxy target |
 *   +--------+     +------------+     +--------------+
 */
void handle_proxy_request(int fd) {
  // connect to proxy
  struct hostent*h=gethostbyname(server_proxy_hostname);
  int pfd=socket(PF_INET,SOCK_STREAM,0);
  struct sockaddr_in sin;
  struct in_addr **addr_list;
  memset(&sin,0,sizeof(sin));
  sin.sin_family=AF_INET;
  sin.sin_port=htons(server_proxy_port);
  addr_list=(struct in_addr**)h->h_addr_list;
  inet_pton(AF_INET,inet_ntoa(*addr_list[0]),&sin.sin_addr);
  if(0>connect(pfd,(struct sockaddr*)&sin,sizeof(sin))) return;
  // set up threads
  pthread_t tcli,tpro;
  pthread_attr_t attr;
  int ab[2]={fd,pfd},ba[2]={pfd,fd};
  pthread_mutex_init(&mutex,NULL);
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_JOINABLE);
  pthread_create(&tcli,&attr,start_act,(void*)&ab);
  pthread_create(&tpro,&attr,start_act,(void*)&ba);
  pthread_join(tcli,NULL);
  pthread_join(tpro,NULL);
  pthread_attr_destroy(&attr);
  pthread_mutex_destroy(&mutex);
  close(pfd);
  pthread_exit(NULL);
}

/*
 * Opens a TCP stream socket on all interfaces with port number PORTNO. Saves
 * the fd number of the server socket in *socket_number. For each accepted
 * connection, calls request_handler with the accepted fd number.
 */
void serve_forever(int *socket_number, void (*request_handler)(int)) {

  struct sockaddr_in server_address, client_address;
  size_t client_address_length = sizeof(client_address);
  int client_socket_number;
  pid_t pid;

  *socket_number = socket(PF_INET, SOCK_STREAM, 0);
  if (*socket_number == -1) {
    perror("Failed to create a new socket");
    exit(errno);
  }

  int socket_option = 1;
  if (setsockopt(*socket_number, SOL_SOCKET, SO_REUSEADDR, &socket_option,
        sizeof(socket_option)) == -1) {
    perror("Failed to set socket options");
    exit(errno);
  }

  memset(&server_address, 0, sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = INADDR_ANY;
  server_address.sin_port = htons(server_port);

  if (bind(*socket_number, (struct sockaddr *) &server_address,
        sizeof(server_address)) == -1) {
    perror("Failed to bind on socket");
    exit(errno);
  }

  if (listen(*socket_number, 1024) == -1) {
    perror("Failed to listen on socket");
    exit(errno);
  }

  printf("Listening on port %d...\n", server_port);

  while (1) {

    client_socket_number = accept(*socket_number,
        (struct sockaddr *) &client_address,
        (socklen_t *) &client_address_length);
    if (client_socket_number < 0) {
      perror("Error accepting socket");
      continue;
    }

    printf("Accepted connection from %s on port %d\n",
        inet_ntoa(client_address.sin_addr),
        client_address.sin_port);

    pid = fork();
    if (pid > 0) {
      close(client_socket_number);
    } else if (pid == 0) {
      // Un-register signal handler (only parent should have it)
      signal(SIGINT, SIG_DFL);
      close(*socket_number);
      request_handler(client_socket_number);
      close(client_socket_number);
      exit(EXIT_SUCCESS);
    } else {
      perror("Failed to fork child");
      exit(errno);
    }
  }

  close(*socket_number);

}

int server_fd;
void signal_callback_handler(int signum) {
  printf("Caught signal %d: %s\n", signum, strsignal(signum));
  printf("Closing socket %d\n", server_fd);
  if (close(server_fd) < 0) perror("Failed to close server_fd (ignoring)\n");
  exit(0);
}

char *USAGE =
  "Usage: ./httpserver --files www_directory/ --port 8000\n"
  "       ./httpserver --proxy inst.eecs.berkeley.edu:80 --port 8000\n";

void exit_with_usage() {
  fprintf(stderr, "%s", USAGE);
  exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
  signal(SIGINT, signal_callback_handler);

  /* Default settings */
  server_port = 8000;
  void (*request_handler)(int) = NULL;

  int i;
  for (i = 1; i < argc; i++) {
    if (strcmp("--files", argv[i]) == 0) {
      request_handler = handle_files_request;
      free(server_files_directory);
      server_files_directory = argv[++i];
      if (!server_files_directory) {
        fprintf(stderr, "Expected argument after --files\n");
        exit_with_usage();
      }
    } else if (strcmp("--proxy", argv[i]) == 0) {
      request_handler = handle_proxy_request;

      char *proxy_target = argv[++i];
      if (!proxy_target) {
        fprintf(stderr, "Expected argument after --proxy\n");
        exit_with_usage();
      }

      char *colon_pointer = strchr(proxy_target, ':');
      if (colon_pointer != NULL) {
        *colon_pointer = '\0';
        server_proxy_hostname = proxy_target;
        server_proxy_port = atoi(colon_pointer + 1);
      } else {
        server_proxy_hostname = proxy_target;
        server_proxy_port = 80;
      }
    } else if (strcmp("--port", argv[i]) == 0) {
      char *server_port_string = argv[++i];
      if (!server_port_string) {
        fprintf(stderr, "Expected argument after --port\n");
        exit_with_usage();
      }
      server_port = atoi(server_port_string);
    } else if (strcmp("--help", argv[i]) == 0) {
      exit_with_usage();
    } else {
      fprintf(stderr, "Unrecognized option: %s\n", argv[i]);
      exit_with_usage();
    }
  }

  if (server_files_directory == NULL && server_proxy_hostname == NULL) {
    fprintf(stderr, "Please specify either \"--files [DIRECTORY]\" or \n"
                    "                      \"--proxy [HOSTNAME:PORT]\"\n");
    exit_with_usage();
  }

  serve_forever(&server_fd, request_handler);

  return EXIT_SUCCESS;
}

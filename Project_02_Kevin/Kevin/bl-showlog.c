#include "blather.h"

int main(int argc, char const *argv[]) {
  if (argc < 2){
    printf("usage: %s <name>\n",argv[0]);
    exit(1);
  }

  int log_fd = open(argv[1], O_RDWR);

  who_t who;
  mesg_t msg;
  int nread = read(log_fd, &who, sizeof(who_t));
  while(nread != 0) {
    read(log_fd, &who, sizeof(who_t));
    printf("%d CLIENTS\n", who.n_clients);
  }
  //printf("%d CLIENTS\n", who.n_clients);
  for(int i = 0; i < who.n_clients; i++){
    printf("%d: %s\n", i, who.names[i]);
  }
  return 0;
}

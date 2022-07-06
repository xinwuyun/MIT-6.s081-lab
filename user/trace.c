#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  if(argc < 2){
    printf("Too few arguments.\n");
    exit(1);
  }
  int mask = atoi(argv[1]);
  if(trace(mask) !=0 ){
    printf("trace failed...\n");
    exit(1);
  }else{
    exec(argv[2], argv+2);
    printf("exec failed...\n");
    exit(1);
  }
  exit(0);
}

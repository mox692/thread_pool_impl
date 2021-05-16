#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/*
・cliからコマンドライン引数をとる
 ・-binpath
 ・-num

*/

main() {
  time_t t = time(NULL);
  const file_path = "./key_data.dat";
  int id = 43;
  key_t key = ftok(file_path, id);
  if (id == -1) {
    return EXIT_FAILURE;
  }
  int shared_segment_size = 0x6400;
  int segment_id = shmget(key, 0, 0);

  if (segment_id == -1) {
    return EXIT_FAILURE;
  }

  srand(t);

  char *shared_mem = shmat(segment_id, 0, 0);
  printf("shared memory attached at address %p\n", shared_mem);

  printf("now: %s\n", shared_mem);

  // TODO: structでやりとりするようにする
  sprintf(shared_mem, "-binpath %s%d -num %d", "./test", rand() % 10,
          rand() % 10);
  printf("after: %s\n", shared_mem);
  shmdt(shared_mem);

  return EXIT_SUCCESS;
}
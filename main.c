#include "errHandle.h"
#include <errno.h>
#include <pthread.h>
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

* cliツールを想定
* Input
    * バイナリへのpath
* Option
    * いくつthreadを立てるか(max_thread)
    * timeout(timeout)
* Example
    * bin
* TODO
    * 完全に別processからtask queueにtaskを追加できるようにする
      * submit taskのタイミングを任意にする
    * taskの種別事にexecを分ける(runtaskの間にmiddleware的なものを挟む)

*/

/*
・program開始時にpidを表示するように変更
・user定義sigを定義、そのあとハンドラを定義
・
*/
typedef struct Task {
  char *taskName;
  char *binpath;
  int num;
} Task;

int max_thread = 4;
int timeout_sec = 3600;
char *bin_path[2]; // TODO: よくわかってない

pthread_mutex_t mutex;
pthread_cond_t cond;

char *shared_mem;
int segment_id;

int original_pid;

Task TaskQueue[256];
int taskCount = 0;
int totalDoneTaskCount = 0;

/*
・loopを常に回し、taskQueueを監視している
・taskQueueにもしtaskがあったら、lockをとりつつどれかのthreadがキャッチする。catchできたthreadがrunTask(ここだと実行バイナリ)を実行
*/

int ctoi(char c) {
  if (c >= '0' && c <= '9') {
    return c - '0';
  } else {
    return -1;
  }
  return EXIT_SUCCESS;
}

int executeTask(Task *task) {
  usleep(50000);
  // execlが呼び出し元に制御を戻さない仕様らしいので、forkしてchild
  // processにexecさせる
  int pid = fork();
  if (pid == 0) {
    if (execl(bin_path, NULL) != 0) {
      return -1;
    }
    exit(0);
  } else {
    wait(NULL);
  }
  pthread_mutex_lock(&mutex);
  totalDoneTaskCount++;
  printf("NO.%d task end.\n", totalDoneTaskCount);
  pthread_mutex_unlock(&mutex);
  return EXIT_SUCCESS;
}

void *startThread() {
  while (1) {
    Task task;
    pthread_mutex_lock(&mutex);
    while (taskCount == 0) {
      pthread_cond_wait(&cond, &mutex);
    }
    task = TaskQueue[0];
    for (int i = 0; i < taskCount - 1; i++) {
      TaskQueue[i] = TaskQueue[i + 1];
    }
    taskCount--;

    pthread_mutex_unlock(&mutex);
    if (executeTask(&task) != 0) {
      return -2;
    }
  }
}

int submitTask() {
  // hardCode
  Task newTask = {.binpath = bin_path, .taskName = "mytask"};
  TaskQueue[taskCount] = newTask;
  taskCount++;
}

void sigUsrHandler(int signo) {
  //  printf("get signal %d\n", signo);
  submitTask();
}

int finish_mem_share() {
  printf("pid: %d\n", getpid());
  if (shmdt(shared_mem) == -1) {
    ERROR("shmdt fail.");
    return EXIT_FAILURE;
  }
  if (shmctl(segment_id, IPC_RMID, 0) == -1) {
    printf("segment_id; %d\n", segment_id);
    ERROR("shmctl fail.");
    return EXIT_FAILURE;
  }
}

// thread系のdest
// sharememの開放
void finish() {
  printf("\nGet SIGINT, program will exit...\n");
  pthread_mutex_destroy(&mutex);
  pthread_cond_destroy(&cond);

  if (finish_mem_share() != 0) {
    ERROR("finish_mem_share fail.");
    exit(EXIT_FAILURE);
  }
  exit(EXIT_SUCCESS);
}

void sigIntHandler(int signo) {
  if (getpid() == original_pid) {
    finish();
  }
  return;
}

int isDelimiter(char p, char delim) { return p == delim; }

int split(char *dst[], char *src, char delim) {
  int count = 0;

  for (;;) {
    while (isDelimiter(*src, delim)) {
      src++;
    }

    if (*src == '\0')
      break;

    dst[count++] = src;

    while (*src && !isDelimiter(*src, delim)) {
      src++;
    }

    if (*src == '\0')
      break;

    *src++ = '\0';
  }
  return count;
}

int start_mem_share() {
  // sigkillに対するハンドラを書きたい
  const file_path = "./key_data.dat";
  int id = 43;
  key_t key = ftok(file_path, id);
  if (id == -1) {
    ERROR("ftok fail.");
    return EXIT_FAILURE;
  }
  int shared_segment_size = 0x6400;
  segment_id = shmget(key, shared_segment_size,
                      IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
  if (segment_id == -1) {
    ERROR("shmget fail.");
    return EXIT_FAILURE;
  }

  printf("key: %d\n", key);
  printf("segment_id: %d\n", segment_id);

  shared_mem = shmat(segment_id, 0, 0);
  if (shared_mem == -1) {
    ERROR("shmat fail");
  }
  printf("shared memory attached at address %p\n", shared_mem);

  // write!!
  sprintf(shared_mem, "-binpath test -num 5", "./atest");

  // sharememにはbinpathとtaskの個数が渡される想定.
  int pid = fork();
  if (pid == 0) {
    char taskInfo[6400];
    char *taskInfoAddr = &taskInfo;
    char tmpStr[100];
    char *dist[100];
    printf("Attach Success, waiting additional task...\n");
    for (;;) {
      sleep(2);
      // printf("taskInfoAddr:%p, shared_mem: %p\n", taskInfoAddr, shared_mem);
      if (strcmp(taskInfoAddr, shared_mem) != 0) {
        // tmp配列に文字を書き込み
        // taskInfo書き換え
        printf("Write detected.\n");
        printf("Before: %s, After: %s\n", taskInfoAddr, shared_mem);
        strcpy(taskInfoAddr, shared_mem);
        // ここから下の処理の仕方は変えていきたい
        for (int i = 0; i < 100; i++) {
          tmpStr[i] = taskInfoAddr[i];
        }
        int count = split(dist, tmpStr, ' ');
        Task newTask;
        Task *newTaskPtr = &newTask;
        printf("[@@@@@@@@@@@@@@@@count  = %d \n", count);
        for (int i = 0; i < count; i++) {
          printf("%s ", dist[i]);
        }
        for (int i = 0; i < count; i++) {
          if (strcmp(dist[i], "-binpath") == 0) {
            printf("called!!!");
            newTask.binpath = dist[i + 1];
          }
          if (strcmp(dist[i], "-num") == 0) {
            newTask.num = atoi(dist[i + 1]);
          }
        }
        printf("newtask.binpath = %s, newtask.num = %d\n", newTask.binpath,
               newTask.num);
      }
    }
  } else {
    return EXIT_SUCCESS;
  }
}

int run() {
  pthread_t th[max_thread];

  // debug text.
  for (int i = 0; i < max_thread; i++) {
    if (pthread_create(&th[i], NULL, &startThread, NULL) != 0) {
      return -2;
    }
  }

  // TODO: hardcode
  int runTaskCount = 10;
  for (int i = 0; i < runTaskCount; i++) {
    pthread_mutex_lock(&mutex);
    submitTask();
    pthread_mutex_unlock(&mutex);
  }

  // sig handler
  printf("initial task all done.\n");
  printf("if you want to add task, please send signal to this process(pid = "
         "%d).\n",
         getpid());

  pthread_mutex_destroy(&mutex);
  pthread_cond_destroy(&cond);
  for (int i = 0; i < max_thread; i++) {
    if (pthread_join(th[i], NULL) != 0) {
      return -3;
    }
  }

  return EXIT_SUCCESS;
}

int initialPrint() {
  printf("******* program start **********\n");
  printf("pid: %d\n", getpid());
  printf("program start in 2s.\n");
  sleep(2);
  return EXIT_SUCCESS;
}

int parseArgs(int argc, char *argv[]) {
  if (argc < 2) {
    ERROR("Args are not enough, got %d. at least you must provide bin path.");
    return EXIT_FAILURE;
  }
  for (int i = 0; i < argc; i++) {
    if (strcmp(argv[i], "-binpath") == 0) {
      strcpy(bin_path, argv[i + 1]);
    } else if (strcmp(argv[i], "-timeout_sec") == 0) {
      timeout_sec = ctoi(*(argv[i + 1]));
      if (timeout_sec < 0) {
        return EXIT_FAILURE;
      }
    } else if (strcmp(argv[i], "-max_thread") == 0) {
      max_thread = ctoi(*(argv[i + 1]));
      if (max_thread < 0) {
        return EXIT_FAILURE;
      }
    }
  }
  printf("path: %s, timeout: %d,thread: %d \n", bin_path, timeout_sec,
         max_thread);
  return EXIT_SUCCESS;
}

void set_original_pid() { original_pid = getpid(); }

// sigハンドラの作成
// thread系のinit
int init() {

  // !forkはこれ以降に行うようにする
  set_original_pid();

  pthread_mutex_init(&mutex, NULL);
  pthread_cond_init(&cond, NULL);
  // SIGの対応開始
  signal(SIGUSR1, sigUsrHandler);
  signal(SIGINT, sigIntHandler);

  // memshareの開始
  if (start_mem_share() != 0) {
    return EXIT_FAILURE;
  }
  initialPrint();
  return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {

  if (parseArgs(argc, argv) != 0) {
    return -2;
  }

  if (init() != 0) {
    return -1;
  }

  if (run() != 0) {
    return -3;
  }

  if (getpid() == original_pid) {
    finish();
  }
  return EXIT_SUCCESS;
}
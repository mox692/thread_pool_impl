#include "errHandle.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
} Task;

int max_thread = 4;
int timeout_sec = 3600;
char *bin_path[2]; // TODO: よくわかってない

pthread_mutex_t mutex;
pthread_cond_t cond;

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
  return 0;
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
  return 0;
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

void sigHandler(int signo) {
  //  printf("get signal %d\n", signo);
  pthread_mutex_lock(&mutex);
  submitTask();
  pthread_mutex_unlock(&mutex);
}

int run() {
  pthread_t th[max_thread];

  pthread_mutex_init(&mutex, NULL);
  pthread_cond_init(&cond, NULL);
  // debug text.
  for (int i = 0; i < max_thread; i++) {
    if (pthread_create(&th[i], NULL, &startThread, NULL) != 0) {
      return -2;
    }
  }

  // TODO: hardcode
  int runTaskCount = 100;
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
  signal(SIGUSR1, sigHandler);

  pthread_mutex_destroy(&mutex);
  pthread_cond_destroy(&cond);
  for (int i = 0; i < max_thread; i++) {
    if (pthread_join(th[i], NULL) != 0) {
      return -3;
    }
  }
  return 0;
}

int initialPrint() {
  printf("******* program start **********\n");
  printf("pid: %d\n", getpid());
  printf("program start in 2s.\n");
  sleep(2);
  return 0;
}

int parseArgs(int argc, char *argv[]) {
  if (argc < 2) {
    ERROR("Args are not enough, got %d. at least you must provide bin path.");
    return -1;
  }

  for (int i = 0; i < argc; i++) {
    if (strcmp(argv[i], "-binpath") == 0) {
      strcpy(bin_path, argv[i + 1]);
    } else if (strcmp(argv[i], "-timeout_sec") == 0) {
      timeout_sec = ctoi(*(argv[i + 1]));
      if (timeout_sec < 0) {
        return -2;
      }
    } else if (strcmp(argv[i], "-max_thread") == 0) {
      max_thread = ctoi(*(argv[i + 1]));
      if (max_thread < 0) {
        return -2;
      }
    }
  }
  printf("path: %s, timeout: %d,thread: %d \n", bin_path, timeout_sec,
         max_thread);
  return 0;
}

int main(int argc, char *argv[]) {

  if (parseArgs(argc, argv) != 0) {
    return -2;
  }

  if (initialPrint() != 0) {
    ERROR("Fail to initialPrint.");
  }

  if (run() != 0) {
    return -3;
  }
  return 0;
}
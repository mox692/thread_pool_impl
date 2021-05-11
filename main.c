#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

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

typedef struct Task {
    char* taskName;
    char* binpath;
} Task;


int max_thread = 4;
int timeout_sec = 3600;
char* bin_path = "./sayHello";

pthread_mutex_t mutex;
pthread_cond_t cond;

Task TaskQueue[256];
int taskCount = 0;


/*
・loopを常に回し、taskQueueを監視している
・taskQueueにもしtaskがあったら、lockをとりつつどれかのthreadがキャッチする。catchできたthreadがrunTask(ここだと実行バイナリ)を実行
*/

int executeTask(Task* task) {
     usleep(50000);
    if(execl(bin_path, NULL) != 0) {
        return -1;
    }
    pthread_mutex_lock(&mutex);
    pthread_mutex_unlock(&mutex);
    printf("task done!!");
}

void* startThread() {
    while(1) {
        Task task;
        pthread_mutex_lock(&mutex);
            while(taskCount == 0) {
                pthread_cond_wait(&cond, &mutex);
            }
            task = TaskQueue[0];
            for (int i = 0; i < taskCount - 1; i++) {
                TaskQueue[i] = TaskQueue[i + 1];
            }
            taskCount--;

        pthread_mutex_unlock(&mutex);
        if(executeTask(&task) != 0) {
            return -1;
        }
    }
}

int submitTask() {
    // hardCode
    Task newTask = {
        .binpath = bin_path,
        .taskName = "mytask"
    }; 
    TaskQueue[taskCount] = newTask;
    taskCount++;
}

int run() {
    pthread_t th[max_thread];

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
    // debug text.
    printf("次の設定でthread poolを作成します,%d ,%s:, %d\n", timeout_sec, bin_path, max_thread);
    for(int i = 0; i < max_thread; i++) {
        if(pthread_create(&th[i], NULL, &startThread, NULL) != 0) {
            return -2;
        }
    }

    // TODO: hardcode
    int runTaskCount = 100;
    for(int i = 0; i < runTaskCount; i++) {
        pthread_mutex_lock(&mutex);
        submitTask();
        pthread_mutex_unlock(&mutex);
    }

    for(int i = 0; i < max_thread; i++) {
       if(pthread_join(th[i], NULL) != 0) {
           return -3;
       }
    }

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
    return 0;
}

int parseArgs(int argc, char* argv[]) {
    if(argc < 2) {
        fprintf(stderr, "err: Args are not enough, got %d. at least you must provide bin path.", argc);
        return -1;
    }

    // TODO:
    // for(int i = 0; i < argc; i++) {
    //          printf("%s\n",argv[i]);
    //     if(*argv[i]=='-binpath') {
    //         strcpy(bin_path, argv[i + 1]);
    //     } else if (*argv[i]=='-timeout') {
    //         strcpy(timeout_sec, argv[i + 1]);
    //     } else if (*argv[i]=='-max_thread') {
    //         strcpy(max_thread, argv[i + 1]);
    //     } 
    // }
    int status = run();
    return status;
}

int main(int argc, char* argv[]) {
    exit(parseArgs(argc, argv));
}
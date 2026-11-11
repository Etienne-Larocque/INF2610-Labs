#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <stdatomic.h>
#include <time.h>
#include <errno.h>

// -------- configuration --------
#define BUFFER_SIZE 5
#define N_CAMERAS 3
#define N_SCREENS 2
// Rappel de l'énoncé : le total caméras + écrans doit être > 4.


// ------------------- Simulation de caméra/écran (fournies, ne pas modifier) -------------------
static atomic_int video_id = 0;
char *createVideo() {
    char *buf = malloc(64);
    if (!buf) { perror("malloc"); exit(1); }
    sprintf(buf, "Video-%d", atomic_fetch_add(video_id, 1));
    return buf;
}
void broadcastVideo(const char *video) {
    printf("[SCREEN] Diffusion: %s\n", video);
}

// ------------------- Signal : simulation de panne (fourni) -------------------
// alarm() est déclenché avec un délai aléatoire dans main() : quand il expire,
// ce handler est appelé. Ne fais PAS de travail lourd dans un handler de
// signal (voir le cours) : contente-toi de lever un drapeau.
volatile sig_atomic_t breakdown_flag = 0;

void handle_breakdown(int sig) {
    (void)sig;
    breakdown_flag = 1;
}

/* TODO (Section 1) : Mise en place du système de vidéosurveillance
 * ------------------------------------------------------------------
 */
static pthread_mutex_t monitor = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t wait_for_space = PTHREAD_COND_INITIALIZER;
pthread_cond_t wait_for_video = PTHREAD_COND_INITIALIZER;
static char* buffer[BUFFER_SIZE];
static int available_spots = BUFFER_SIZE;

static void put_video(char* video) {
    pthread_mutex_lock(&monitor);
    while (available_spots <= 0) {
        pthread_cond_wait(&wait_for_space, &monitor);
    }
    buffer[BUFFER_SIZE-available_spots] = video;
    available_spots--;
    pthread_cond_signal(&wait_for_video);
    pthread_mutex_unlock(&monitor);
}


static char* get_video() {
    pthread_mutex_lock(&monitor);
    while (available_spots >= BUFFER_SIZE) {
        pthread_cond_wait(&wait_for_video, &monitor);
    }
    char* video = buffer[0];
    for (int i = 0; i < (BUFFER_SIZE-1); i++) {
        buffer[i] = buffer[i + 1];
    }
    buffer[BUFFER_SIZE - 1] = NULL;
    available_spots++;
    pthread_cond_signal(&wait_for_space);
    pthread_mutex_unlock(&monitor);
    return video;
}

void* camera(void* arg) {
    while (!breakdown_flag) {
        char *video = createVideo();
        put_video(video);
    }
    return NULL;
}

void* screen(void* arg) {
    char* video;
    while (!breakdown_flag) {
        video = get_video();
        broadcastVideo(video);
        free(video);
    }
    return NULL;
}

 /* TODO (Section 2) : Intégrité des données lors d'une panne
 * ------------------------------------------------------------------
 */
typedef struct {
    pthread_t* camtid, scrtid;
} sec_args_t;

void* secondaire(void* arg) {
    sec_args_t* args = (sec_args_t*)arg;
    pthread_t camtid[N_CAMERAS] = args->camtid;
    pthread_t scrtid[N_SCREENS] = args->scrttid;

    while (!breakdown_flag);

    for (int i = 0; i < N_CAMERAS; i++)  pthread_join(camtid[i], NULL);
    for (int i = 0; i < N_SCREENS; i++)  pthread_join(scrtid[i], NULL);

    // collects data, returns data

    return NULL
}

int main(int argc, char *argv[]) {
    /*if (argc != 4) {
        printf("Usage: %s <log1> <log2> <N>\n", argv[0]);
        return 1;
    }*/

    if (N_CAMERAS + N_SCREENS <= 4) {
        printf("Should have more than four total cameras or screens");
        return 1;
    }

    (void)argc;
    (void)argv;
    srand(time(NULL));




    struct sigaction sa;
    sa.sa_handler = handle_breakdown;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGALRM, &sa, NULL);

    pthread_t camtid[N_CAMERAS], scrtid[N_SCREENS], sectid;
    for (int i = 0; i < N_CAMERAS; i++)  pthread_create(&camtid[i], NULL, camera, NULL);
    for (int i = 0; i < N_SCREENS; i++)  pthread_create(&scrtid[i], NULL, screen, NULL);

    pthread_create(&sectid, NULL, secondaire, NULL);

    alarm(rand() % 11 + 5);

    while (true) {
        pthread_join(sectid, NULL);
        // catch data to restart
        
        // wait for any keyboard input
        
        // restarts

        pthread_create(&sectid, NULL, secondaire, NULL);
    }

    return 0;
}

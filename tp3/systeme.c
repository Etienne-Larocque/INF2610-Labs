#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/mman.h>
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
#define CAMERA_DELAY_US 400000
#define SCREEN_DELAY_US 1000000  // durée d'une diffusion

// ------------------- Simulation de caméra/écran (fournies, ne pas modifier) -------------------
static atomic_int video_id = 0;
char *createVideo() {
    char *buf = malloc(64);
    if (!buf) { perror("malloc"); exit(1); }
    sprintf(buf, "Video-%d", atomic_fetch_add(&video_id, 1));
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

typedef struct {
    char videos[BUFFER_SIZE][64];
    int count;
    char playing[N_SCREENS][64];
    int next_id;
} backup_t;

static backup_t *backup;
static const char *role = "PRINCIPAL";

// Moniteur du serveur
static pthread_mutex_t monitor = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t space_available = PTHREAD_COND_INITIALIZER;
static pthread_cond_t video_available = PTHREAD_COND_INITIALIZER;
static char *buffer[BUFFER_SIZE];
static int bufin = 0, bufout = 0, count = 0;
static char *playing[N_SCREENS];  // vidéo en diffusion par chaque écran
static volatile sig_atomic_t stop = 0;

// Appelée sous le mutex : la sauvegarde est toujours cohérente avec le buffer
static void update_backup(void) {
    for (int i = 0; i < count; i++)
        strcpy(backup->videos[i], buffer[(bufout + i) % BUFFER_SIZE]);
    backup->count = count;
    for (int s = 0; s < N_SCREENS; s++)
        strcpy(backup->playing[s], playing[s] ? playing[s] : "");
}

static int put_video(char *video) {
    pthread_mutex_lock(&monitor);

    while (count == BUFFER_SIZE && !stop)
        pthread_cond_wait(&space_available, &monitor);
    if (stop) {
        pthread_mutex_unlock(&monitor);
        return 0;
    }

    buffer[bufin] = video;
    bufin = (bufin + 1) % BUFFER_SIZE;
    count++;
    update_backup();
    printf("[%s] Reçu: %s (%d/%d)\n", role, video, count, BUFFER_SIZE);
    pthread_cond_signal(&video_available);
    pthread_mutex_unlock(&monitor);
    return 1;
}

// L'écran i prend une vidéo. Elle passe du buffer à playing[i] au lieu de
// disparaître : elle reste dans la sauvegarde tant que la diffusion n'est pas finie.
static int get_video(int i) {
    pthread_mutex_lock(&monitor);
    if (playing[i]) {  // diffusion interrompue par la panne : on la recommence
        pthread_mutex_unlock(&monitor);
        return 1;
    }

    while (count == 0 && !stop)
        pthread_cond_wait(&video_available, &monitor);
    if (stop) {
        pthread_mutex_unlock(&monitor);
        return 0;
    }

    playing[i] = buffer[bufout];
    buffer[bufout] = NULL;
    bufout = (bufout + 1) % BUFFER_SIZE;
    count--;
    update_backup();
    pthread_cond_signal(&space_available);
    pthread_mutex_unlock(&monitor);
    return 1;
}

static void finish_broadcast(int i) {
    pthread_mutex_lock(&monitor);
    free(playing[i]);
    playing[i] = NULL;
    update_backup();
    pthread_mutex_unlock(&monitor);
}

void *camera(void *arg) {
    (void)arg;
    while (1) {
        usleep(CAMERA_DELAY_US);
        char *video = createVideo();
        if (!put_video(video)) {
            free(video);  // jamais reçue par le serveur
            break;
        }
    }
    return NULL;
}

void *screen(void *arg) {
    int i = (int)(intptr_t)arg;
    while (get_video(i)) {
        broadcastVideo(playing[i]);
        usleep(SCREEN_DELAY_US);
        if (stop) {  // panne pendant la diffusion : playing[i] est gardée
            printf("[SCREEN] Interrompue: %s\n", playing[i]);
            break;
        }
        finish_broadcast(i);
    }
    return NULL;
}

static pthread_t camtid[N_CAMERAS], scrtid[N_SCREENS];

static void start_threads(void) {
    stop = 0;
    for (int i = 0; i < N_CAMERAS; i++) pthread_create(&camtid[i], NULL, camera, NULL);
    for (int i = 0; i < N_SCREENS; i++) pthread_create(&scrtid[i], NULL, screen, (void *)(intptr_t)i);
}

static void stop_threads(void) {
    // broadcast pour réveiller ceux qui dorment dans un wait, sinon ils ne verront jamais stop
    pthread_mutex_lock(&monitor);
    stop = 1;
    pthread_cond_broadcast(&space_available);
    pthread_cond_broadcast(&video_available);
    pthread_mutex_unlock(&monitor);

    for (int i = 0; i < N_CAMERAS; i++) pthread_join(camtid[i], NULL);
    for (int i = 0; i < N_SCREENS; i++) pthread_join(scrtid[i], NULL);
}

// Recopie la sauvegarde dans la mémoire du serveur (aucun thread ne roule)
static void load_from_backup(void) {
    for (int i = 0; i < backup->count; i++) {
        buffer[bufin] = strdup(backup->videos[i]);
        bufin = (bufin + 1) % BUFFER_SIZE;
        count++;
    }
    for (int s = 0; s < N_SCREENS; s++)
        playing[s] = backup->playing[s][0] ? strdup(backup->playing[s]) : NULL;
    video_id = backup->next_id;
}

// Utilisé lorsque le serveur tombe en panne
static void clear_server_memory(void) {
    backup->next_id = video_id;
    while (count > 0) {
        free(buffer[bufout]);
        buffer[bufout] = NULL;
        bufout = (bufout + 1) % BUFFER_SIZE;
        count--;
    }
    for (int s = 0; s < N_SCREENS; s++) {
        free(playing[s]);
        playing[s] = NULL;
    }
}

static void wait_for_signal(int fd) {
    char c;
    if (read(fd, &c, 1) <= 0) exit(1);  // l'autre serveur est fermé
}

// Serveur secondaire (processus fils)
static void run_secondary_server(int rfd, int wfd) {
    role = "SECONDAIRE";
    while (1) {
        wait_for_signal(rfd);  // PANNE
        load_from_backup();
        printf("[SECONDAIRE] Copie terminée (%d vidéos en attente), prend le relais\n", count);
        write(wfd, "O", 1);
        start_threads();

        wait_for_signal(rfd);  // RESET
        stop_threads();
        clear_server_memory();
        printf("[SECONDAIRE] Données renvoyées, retour en attente\n");
        write(wfd, "F", 1);
    }
}

/* TODO (Section 2) : Intégrité des données lors d'une panne
 * ------------------------------------------------------------------
 * Une vidéo prise par un écran reste dans playing[i] (et donc dans la
 * sauvegarde) jusqu'à la fin de sa diffusion. Si la panne arrive pendant la
 * diffusion, le serveur qui prend le relais la recharge et l'écran la
 * recommence depuis le début.
 */

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    srand(time(NULL));

    struct sigaction sa;
    sa.sa_handler = handle_breakdown;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGALRM, &sa, NULL);

    backup = mmap(NULL, sizeof(backup_t), PROT_READ | PROT_WRITE,
                MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (backup == MAP_FAILED) { perror("mmap"); return 1; }

    int to_sec[2], to_pri[2];
    pipe(to_sec);
    pipe(to_pri);

    fflush(stdout);
    if (fork() == 0) {
        close(to_sec[1]);
        close(to_pri[0]);
        run_secondary_server(to_sec[0], to_pri[1]);
    }
    close(to_sec[0]);
    close(to_pri[1]);

    sigset_t alarm_mask, old_mask;
    sigemptyset(&alarm_mask);
    sigaddset(&alarm_mask, SIGALRM);

    while (1) {
        load_from_backup();
        // Les threads héritent du masque qui bloque le SIG
        sigprocmask(SIG_BLOCK, &alarm_mask, &old_mask);
        start_threads();
        sigprocmask(SIG_SETMASK, &old_mask, NULL);

        // Simulation d'une panne du serveur principal après un délai aléatoire.
        alarm(rand() % 11 + 5);
        while (!breakdown_flag)
            pause();

        printf("\n[PRINCIPAL] *** PANNE ***\n");
        stop_threads();
        clear_server_memory();
        write(to_sec[1], "P", 1);

        wait_for_signal(to_pri[0]);  // confirmation du secondaire
        printf("[PRINCIPAL] Confirmation reçue, Entrée pour redémarrer\n");
        getchar();

        write(to_sec[1], "R", 1);
        wait_for_signal(to_pri[0]);  // le secondaire a remis les données dans la sauvegarde
        breakdown_flag = 0;
        printf("[PRINCIPAL] Redémarrage\n");
    }
}

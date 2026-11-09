#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/wait.h>
#include "types.h"
#include "communication.h"
#include "ascenseur.h"
#include "utilisateur.h"
#include "gestionnaire.h"

int main(void)
{
    int file_id = creer_file_messages();
    if (file_id < 0)
    {
        perror("msgget");
        return EXIT_FAILURE;
    }

    srand((unsigned int)time(NULL));

    pid_t processus[NB_ASCENSEURS];

    for (int i = 0; i < NB_ASCENSEURS; i++)
    {
        processus[i] = fork();
        if (processus[i] < 0)
        {
            perror("fork");
            supprimer_file_messages(file_id);
            return EXIT_FAILURE;
        }
        if (processus[i] == 0)
        {
            processus_ascenseur(file_id, i, 0);
        }
    }

    pthread_t threads[NB_UTILISATEURS];

    for (int i = 0; i < NB_UTILISATEURS; i++)
    {
        ParametreUtilisateur *parametre = malloc(sizeof(ParametreUtilisateur));
        if (!parametre)
        {
            perror("malloc");
            return EXIT_FAILURE;
        }
        parametre->id = i + 1;
        parametre->file_id = file_id;

        if (pthread_create(&threads[i], NULL, generer_utilisateur, parametre) != 0)
        {
            perror("pthread_create");
            free(parametre);
            return EXIT_FAILURE;
        }
        usleep(300000);
    }

    for (int i = 0; i < NB_UTILISATEURS; i++)
    {
        pthread_join(threads[i], NULL);
    }

    gerer_demandes(file_id, NB_UTILISATEURS);

    for (int i = 0; i < NB_ASCENSEURS; i++)
    {
        Demande stop = {-1, -1, -1};
        envoyer_commande(file_id, i, stop);
    }

    for (int i = 0; i < NB_ASCENSEURS; i++)
    {
        waitpid(processus[i], NULL, 0);
    }

    supprimer_file_messages(file_id);
    printf("\nSimulation terminee.\n");
    return EXIT_SUCCESS;
}

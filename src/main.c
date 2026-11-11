/* Programme principal : ressources IPC, processus ascenseurs et threads du scenario. */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <getopt.h>
#include <string.h>
#include "types.h"
#include "communication.h"
#include "synchronisation.h"
#include "ascenseur.h"
#include "utilisateur.h"
#include "gestionnaire.h"
#include "alertes.h"
#include "scenario.h"
#include "immeuble.h"

static volatile sig_atomic_t arret_demande = 0;
static EtatPartage *partage_signal = NULL;

static void afficher_aide(const char *nom)
{
    printf("Usage: %s [-n utilisateurs] [-d delai] [-p panne%%] [-r reparation] [-c capacite] [-i] [-s graine] [-v]\n", nom);
}

static void afficher_niveaux_interactifs(void)
{
    puts("\nEtages disponibles :");
    for (int i = 0; i <= NB_ETAGES; i++)
        printf("  %2d - %s\n", i, nom_etage(i));
    puts("Priorites : 0 = normale, 1 = acces prioritaire, 2 = urgente");
}

static void attendre_etat_ascenseur(EtatPartage *partage, int numero, int panne)
{
    const struct timespec cent_millisecondes = {.tv_sec = 0, .tv_nsec = 100000000L};
    const struct timespec cent_cinquante_millisecondes = {.tv_sec = 0, .tv_nsec = 150000000L};
    for (int tentative = 0; tentative < 100; tentative++)
    {
        int etat_panne;
        verrouiller_etat(partage);
        etat_panne = partage->ascenseurs[numero].hors_service;
        deverrouiller_etat(partage);
        if (etat_panne == panne)
        {
            nanosleep(&cent_cinquante_millisecondes, NULL);
            return;
        }
        nanosleep(&cent_millisecondes, NULL);
    }
}

static void gerer_signal(int numero_signal)
{
    (void)numero_signal;
    arret_demande = 1;
    /* sem_post est utilise pour reveiller le gestionnaire bloque sur le semaphore. */
    if (partage_signal != NULL)
        sem_post(&partage_signal->semaphore_evenements);
}

int main(int argc, char **argv)
{
    Configuration configuration = {.utilisateurs = NB_UTILISATEURS, .delai_creation = 1,
                                   .panne_probabilite = 30, .duree_reparation = 5,
                                   .capacite = MAX_COMMANDES_ASCENSEUR, .interactif = 0,
                                   .verbeux = 0,
                                   .graine = (unsigned int)time(NULL)};
    int option;
    while ((option = getopt(argc, argv, "n:d:p:r:c:is:hv")) != -1)
    {
        switch (option)
        {
        case 'n': configuration.utilisateurs = atoi(optarg); break;
        case 'd': configuration.delai_creation = atoi(optarg); break;
        case 'p': configuration.panne_probabilite = atoi(optarg); break;
        case 'r': configuration.duree_reparation = atoi(optarg); break;
        case 'c': configuration.capacite = atoi(optarg); break;
        case 'i': configuration.interactif = 1; break;
        case 's': configuration.graine = (unsigned int)strtoul(optarg, NULL, 10); break;
        case 'v': configuration.verbeux = 1; break;
        case 'h': afficher_aide(argv[0]); return EXIT_SUCCESS;
        default: afficher_aide(argv[0]); return EXIT_FAILURE;
        }
    }
    if (configuration.utilisateurs < 1 || configuration.utilisateurs > CAPACITE_FILE ||
        configuration.delai_creation < 0 || configuration.panne_probabilite < 0 ||
        configuration.panne_probabilite > 100 || configuration.duree_reparation < 0 ||
        configuration.capacite < 1 || configuration.capacite > CAPACITE_ATTENTE_ASCENSEUR)
    {
        fprintf(stderr, "Options invalides.\n");
        return EXIT_FAILURE;
    }
    int id_file, identifiant_shm = -1;
    int tube[2];
    int nombre_enfants = 0;
    pid_t enfants[NB_ASCENSEURS] = {-1, -1};
    pthread_t thread_manager, thread_alertes_id, thread_scenario;
    pthread_t utilisateurs[CAPACITE_FILE];
    int manager_lance = 0, alertes_lance = 0, scenario_lance = 0;
    int nombre_utilisateurs_lances = 0;
    int resultat_final = EXIT_SUCCESS;
    char chemin_statistiques[256];
    Demande demandes_interactives[CAPACITE_FILE];

    setvbuf(stdout, NULL, _IOLBF, 0);
    signal(SIGINT, gerer_signal);
    signal(SIGTERM, gerer_signal);
    srand(configuration.graine);

    if (mkdir("resultats", 0775) < 0 && errno != EEXIST)
    {
        perror("creation du dossier resultats");
        return EXIT_FAILURE;
    }
    snprintf(chemin_statistiques, sizeof(chemin_statistiques),
             "resultats/statistiques_%ld_%ld.csv", (long)time(NULL), (long)getpid());

    id_file = creer_file_messages();
    if (id_file < 0)
    {
        perror("msgget");
        return EXIT_FAILURE;
    }

    EtatPartage *partage = creer_etat_partage(&identifiant_shm);
    if (partage == NULL)
    {
        perror("creation memoire partagee");
        supprimer_file_messages(id_file);
        return EXIT_FAILURE;
    }
    partage_signal = partage;
    partage->affichage_detaille = configuration.verbeux;
    partage->mode_interactif = configuration.interactif;
    for (int i = 0; i < NB_ASCENSEURS; i++)
        partage->ascenseurs[i].capacite = configuration.capacite;

    if (pipe(tube) < 0)
    {
        perror("pipe alertes");
        detruire_etat_partage(identifiant_shm, partage);
        supprimer_file_messages(id_file);
        return EXIT_FAILURE;
    }

    printf("Simulation de frequentation : %d usagers, 2 ascenseurs, 11 niveaux.\n",
           configuration.utilisateurs);
    printf("Capacite : %d, probabilite de panne : %d%%.\n\n",
           configuration.capacite, configuration.panne_probabilite);

    /* Le tube est cree avant fork : les processus fils en heritent l'extremite ecriture. */
    for (int i = 0; i < NB_ASCENSEURS; i++)
    {
        enfants[i] = fork();
        if (enfants[i] < 0)
        {
            perror("fork");
            arret_demande = 1;
            break;
        }
        if (enfants[i] == 0)
        {
            close(tube[0]);
            processus_ascenseur(id_file, i, partage, tube[1]);
            close(tube[1]);
            _exit(EXIT_SUCCESS);
        }
        nombre_enfants++;
    }

    if (nombre_enfants != NB_ASCENSEURS)
    {
        resultat_final = EXIT_FAILURE;
        arret_demande = 1;
    }

    StatistiquesThreads statistiques;
    statistiques.demandes_generees = 0;
    statistiques.demandes_terminees = 0;
    pthread_mutex_init(&statistiques.mutex_aleatoire, NULL);
    pthread_mutex_init(&statistiques.mutex_compteurs, NULL);

    ParametreAlertes param_alertes = {.tube_lecture = tube[0], .partage = partage};
    if (!arret_demande)
    {
        int rc = pthread_create(&thread_alertes_id, NULL, thread_alertes, &param_alertes);
        if (rc == 0)
            alertes_lance = 1;
        else
        {
            fprintf(stderr, "Creation thread alertes impossible (%d).\n", rc);
            arret_demande = 1;
        }
    }

    ParametreGestionnaire param_manager = {
        .id_file = id_file,
        .partage = partage,
        .nombre_attendu = configuration.utilisateurs,
        .arret = &arret_demande,
        .statistiques = &statistiques,
        .resultat = 0,
        .configuration = &configuration,
        .chemin_statistiques = chemin_statistiques};
    if (!arret_demande)
    {
        int rc = pthread_create(&thread_manager, NULL, thread_gestionnaire, &param_manager);
        if (rc == 0)
            manager_lance = 1;
        else
        {
            fprintf(stderr, "Creation thread gestionnaire impossible (%d).\n", rc);
            arret_demande = 1;
        }
    }

    ParametreScenario param_scenario = {.id_file = id_file, .partage = partage,
                                        .configuration = &configuration};
    if (!arret_demande && !configuration.interactif)
    {
        int rc = pthread_create(&thread_scenario, NULL,
                                scenario_panne_reparation, &param_scenario);
        if (rc == 0)
            scenario_lance = 1;
        else
        {
            fprintf(stderr, "Creation thread scenario impossible (%d).\n", rc);
            arret_demande = 1;
        }
    }

    /* Les appels arrivent progressivement pour representer un flux d'usagers. */
    if (!arret_demande && !configuration.interactif)
    {
        for (int i = 0; i < configuration.utilisateurs && !arret_demande; i++)
        {
            ParametreUtilisateur *parametre = malloc(sizeof(*parametre));
            if (parametre == NULL)
            {
                perror("malloc utilisateur");
                arret_demande = 1;
                break;
            }
            parametre->id = i + 1;
            parametre->id_file = id_file;
            parametre->partage = partage;
            parametre->statistiques = &statistiques;
            parametre->configuration = &configuration;
            parametre->a_demande_preparee = configuration.interactif;
            if (configuration.interactif)
                parametre->demande_preparee = demandes_interactives[i];
            int rc = pthread_create(&utilisateurs[i], NULL,
                                    generer_utilisateur, parametre);
            if (rc != 0)
            {
                fprintf(stderr, "Creation thread usager impossible (%d).\n", rc);
                free(parametre);
                arret_demande = 1;
                break;
            }
            nombre_utilisateurs_lances++;
            if (i + 1 < configuration.utilisateurs && configuration.delai_creation > 0)
                sleep((unsigned int)configuration.delai_creation);
        }
    }

    if (!arret_demande && configuration.interactif)
    {
        char commande[32];
        while (!arret_demande && nombre_utilisateurs_lances < configuration.utilisateurs)
        {
            puts("\n=== CONSOLE ASCENSEURS ===");
            int tous_en_panne = 1;
            verrouiller_etat(partage);
            for (int ascenseur = 0; ascenseur < NB_ASCENSEURS; ascenseur++)
                if (!partage->ascenseurs[ascenseur].hors_service)
                    tous_en_panne = 0;
            deverrouiller_etat(partage);
            if (tous_en_panne)
                puts("etat | reparer | aide | quitter");
            else
                puts("appel | etat | panne | reparer | aide | quitter");
            printf("> ");
            fflush(stdout);
            if (scanf("%31s", commande) != 1)
                break;
            if (strcmp(commande, "aide") == 0)
            {
                puts("appel   : creer une demande utilisateur");
                puts("etat    : afficher la position et la charge des ascenseurs");
                puts("panne   : mettre un ascenseur hors service");
                puts("reparer : remettre un ascenseur en service");
                puts("quitter : interrompre la simulation");
            }
            else if (strcmp(commande, "appel") == 0)
            {
                if (tous_en_panne)
                {
                    puts("[INFO] Appel impossible : les deux ascenseurs sont en panne.");
                    puts("Reparez au moins un ascenseur avant de lancer un appel.");
                    continue;
                }
                int depart, arrivee, priorite;
                afficher_niveaux_interactifs();
                printf("Etage de depart (numero) : ");
                int saisie_depart = scanf("%d", &depart);
                printf("Etage de destination (numero) : ");
                int saisie_arrivee = scanf("%d", &arrivee);
                printf("Priorite (0 normale, 1 prioritaire, 2 urgente) : ");
                int saisie_priorite = scanf("%d", &priorite);
                if (saisie_depart != 1 || saisie_arrivee != 1 || saisie_priorite != 1 ||
                    depart < 0 || depart > NB_ETAGES || arrivee < 0 || arrivee > NB_ETAGES ||
                    depart == arrivee || priorite < 0 || priorite > 2)
                {
                    puts("Demande invalide.");
                    continue;
                }
                int i = nombre_utilisateurs_lances;
                int tous_en_panne = 1;
                verrouiller_etat(partage);
                for (int ascenseur = 0; ascenseur < NB_ASCENSEURS; ascenseur++)
                    if (!partage->ascenseurs[ascenseur].hors_service)
                        tous_en_panne = 0;
                deverrouiller_etat(partage);
                if (tous_en_panne)
                    puts("[INFO] Les deux ascenseurs sont en panne : la demande reste en attente.");
                demandes_interactives[i] = (Demande){.id = i + 1, .etage_depart = depart,
                    .etage_arrivee = arrivee, .priorite = priorite,
                    .tentatives = 0, .heure_creation = time(NULL)};
                ParametreUtilisateur *parametre = malloc(sizeof(*parametre));
                if (parametre == NULL)
                {
                    perror("malloc utilisateur");
                    arret_demande = 1;
                    break;
                }
                parametre->id = i + 1;
                parametre->id_file = id_file;
                parametre->partage = partage;
                parametre->statistiques = &statistiques;
                parametre->configuration = &configuration;
                parametre->a_demande_preparee = 1;
                parametre->demande_preparee = demandes_interactives[i];
                if (pthread_create(&utilisateurs[i], NULL, generer_utilisateur, parametre) != 0)
                {
                    free(parametre);
                    puts("Impossible de creer le thread utilisateur.");
                    arret_demande = 1;
                    break;
                }
                nombre_utilisateurs_lances++;
                puts("[CONSOLE] Demande envoyee. Le trajet continue en arriere-plan.");
            }
            else if (strcmp(commande, "etat") == 0)
            {
                verrouiller_etat(partage);
                for (int i = 0; i < NB_ASCENSEURS; i++)
                    printf("Ascenseur %d : niveau %d (%s), %s, capacite %d/%d\n",
                           i + 1, partage->ascenseurs[i].etage,
                           nom_etage(partage->ascenseurs[i].etage),
                           partage->ascenseurs[i].hors_service ? "en panne" :
                           (partage->ascenseurs[i].occupe ? "en mouvement" : "disponible"),
                           partage->ascenseurs[i].occupe ? 1 : 0,
                           partage->ascenseurs[i].capacite);
                deverrouiller_etat(partage);
            }
            else if (strcmp(commande, "panne") == 0 || strcmp(commande, "reparer") == 0)
            {
                int numero;
                printf("Numero de l'ascenseur (1 ou 2) : ");
                if (scanf("%d", &numero) != 1 || numero < 1 || numero > NB_ASCENSEURS)
                {
                    puts("Numero invalide.");
                    continue;
                }
                int deja_en_panne;
                verrouiller_etat(partage);
                deja_en_panne = partage->ascenseurs[numero - 1].hors_service;
                deverrouiller_etat(partage);
                if (strcmp(commande, "panne") == 0 && deja_en_panne)
                {
                    puts("[INFO] Cet ascenseur est deja en panne.");
                    continue;
                }
                if (strcmp(commande, "reparer") == 0 && !deja_en_panne)
                {
                    puts("[INFO] Cet ascenseur est deja en service.");
                    continue;
                }
                Demande aucune = {.id = -1};
                int action = strcmp(commande, "panne") == 0 ? COMMANDE_PANNE : COMMANDE_REPARER;
                envoyer_commande(id_file, numero - 1, action, &aucune);
                attendre_etat_ascenseur(partage, numero - 1, action == COMMANDE_PANNE);
                puts(action == COMMANDE_PANNE ? "Ordre de panne envoye." : "Ordre de reparation envoye.");
            }
            else if (strcmp(commande, "quitter") == 0)
            {
                arret_demande = 1;
                sem_post(&partage->semaphore_evenements);
            }
            else
                puts("Commande inconnue. Tapez aide.");
        }
    }

    /* Le gestionnaire termine les appels et envoie les notifications finales
       avant que le processus principal attende les threads usagers. */
    if (scenario_lance)
        pthread_join(thread_scenario, NULL);
    if (manager_lance)
        pthread_join(thread_manager, NULL);

    if (arret_demande)
    {
        /* En cas d'interruption, liberer les usagers encore en attente d'un retour. */
        for (int i = 1; i <= nombre_utilisateurs_lances; i++)
        {
            Demande demande_arret = {.id = i};
            if (envoyer_notification(id_file, &demande_arret,
                                     NOTIFICATION_ARRET, -1) < 0)
                perror("notification arret usager");
        }
    }
    for (int i = 0; i < nombre_utilisateurs_lances; i++)
        pthread_join(utilisateurs[i], NULL);

    if (param_manager.resultat != configuration.utilisateurs)
        resultat_final = EXIT_FAILURE;

    /* Chaque ascenseur recoit un ordre d'arret, puis le pere attend ses fils. */
    for (int i = 0; i < nombre_enfants; i++)
    {
        if (envoyer_commande(id_file, i, COMMANDE_ARRET, NULL) < 0)
        {
            perror("envoi arret ascenseur");
            kill(enfants[i], SIGTERM);
        }
    }
    for (int i = 0; i < nombre_enfants; i++)
    {
        int statut;
        if (waitpid(enfants[i], &statut, 0) < 0 ||
            !WIFEXITED(statut) || WEXITSTATUS(statut) != 0)
            resultat_final = EXIT_FAILURE;
    }

    /* Le pere envoie un dernier enregistrement pour liberer le thread d'alertes. */
    if (alertes_lance)
    {
        envoyer_alerte(tube[1], partage, ALERTE_FIN_THREAD, -1, -1, -1);
        pthread_join(thread_alertes_id, NULL);
    }

    close(tube[0]);
    close(tube[1]);
    pthread_mutex_lock(&statistiques.mutex_compteurs);
    printf("Demandes generees : %d / %d.\n",
           statistiques.demandes_generees, configuration.utilisateurs);
    pthread_mutex_unlock(&statistiques.mutex_compteurs);
    pthread_mutex_destroy(&statistiques.mutex_aleatoire);
    pthread_mutex_destroy(&statistiques.mutex_compteurs);
    supprimer_file_messages(id_file);
    detruire_etat_partage(identifiant_shm, partage);
    partage_signal = NULL;
    puts("Simulation terminee : processus et ressources IPC liberes.");
    return arret_demande ? EXIT_FAILURE : resultat_final;
}

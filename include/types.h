#ifndef TYPES_H
#define TYPES_H

#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <time.h>

#define NB_ETAGES 10 /* Niveaux 0 a 10 inclus. */
#define NB_ASCENSEURS 2
#define NB_UTILISATEURS 12
#define CAPACITE_FILE 128
#define CAPACITE_ATTENTE_ASCENSEUR 3
#define MAX_COMMANDES_ASCENSEUR 2 /* En cours + une commande en attente. */

typedef struct
{
    int utilisateurs;
    int delai_creation;
    int panne_probabilite;
    int duree_reparation;
    int capacite;
    int interactif;
    int verbeux;
    unsigned int graine;
} Configuration;

#define TYPE_DEMANDE 1L
#define TYPE_COMMANDE_BASE 100L
#define TYPE_FIN_BASE 200L
#define TYPE_NOTIFICATION_BASE 300L

typedef enum
{
    DIRECTION_DESCENTE = -1,
    DIRECTION_IMMOBILE = 0,
    DIRECTION_MONTEE = 1
} Direction;

typedef enum
{
    PRIORITE_NORMALE = 0,
    PRIORITE_ACCES = 1,
    PRIORITE_URGENTE = 2
} Priorite;

typedef enum
{
    COMMANDE_ATTRIBUER = 1,
    COMMANDE_PANNE = 2,
    COMMANDE_REPARER = 3,
    COMMANDE_ARRET = 4
} TypeCommande;

typedef enum
{
    FIN_TRAJET = 0,
    FIN_PANNE = 1,
    FIN_REPARATION = 2
} StatutFin;

typedef enum
{
    NOTIFICATION_ENREGISTREE = 1,
    NOTIFICATION_AFFECTEE = 2,
    NOTIFICATION_REAFFECTEE = 3,
    NOTIFICATION_TERMINEE = 4,
    NOTIFICATION_ARRET = 5
} TypeNotification;

typedef enum
{
    ALERTE_PANNE = 1,
    ALERTE_REPARATION = 2,
    ALERTE_FIN_THREAD = 99
} TypeAlerte;

typedef struct
{
    int id;
    int etage_depart;
    int etage_arrivee;
    int priorite;
    int tentatives;
    time_t heure_creation;
} Demande;

/* Pour System V, le premier champ d'un message doit etre un long type. */
typedef struct
{
    long type;
    Demande demande;
} MessageDemande;

typedef struct
{
    long type;
    int action;
    Demande demande;
} MessageCommande;

typedef struct
{
    long type;
    Demande demande;
    int numero_ascenseur;
    int statut;
    time_t heure_prise_en_charge;
    time_t heure_fin;
} MessageFin;

/* Notification destinee au thread representant l'usager demandeur.
 * Le type est propre a l'identifiant de l'usager (300 + id). */
typedef struct
{
    long type;
    int demande_id;
    int statut;
    int numero_ascenseur;
} MessageNotification;

typedef struct
{
    int etage;
    int direction;
    int destination;
    int occupe;
    int hors_service;
    unsigned long trajets_termines;
    int capacite;
} EtatAscenseur;

/* Zone partagee entre le processus principal et les deux processus ascenseurs. */
typedef struct
{
    sem_t semaphore_etat;       /* Semaphore binaire : protection de l'etat partage. */
    sem_t semaphore_evenements; /* Reveille le gestionnaire apres une demande/fin. */
    sem_t semaphore_alertes;    /* Synchronise les alertes lues dans le tube. */
    int affichage_detaille;
    int mode_interactif;
    EtatAscenseur ascenseurs[NB_ASCENSEURS];
} EtatPartage;

/* Donnees partagees uniquement entre les threads du processus principal. */
typedef struct
{
    pthread_mutex_t mutex_aleatoire;
    pthread_mutex_t mutex_compteurs;
    int demandes_generees;
    int demandes_terminees;
} StatistiquesThreads;

typedef struct
{
    int type;
    int numero_ascenseur;
    int etage;
    int demande_id;
} Alerte;

typedef struct
{
    int id;
    int id_file;
    EtatPartage *partage;
    StatistiquesThreads *statistiques;
    Configuration *configuration;
    Demande demande_preparee;
    int a_demande_preparee;
} ParametreUtilisateur;

typedef struct
{
    int id_file;
    EtatPartage *partage;
    int nombre_attendu;
    volatile sig_atomic_t *arret;
    StatistiquesThreads *statistiques;
    int resultat;
    Configuration *configuration;
    const char *chemin_statistiques;
} ParametreGestionnaire;

typedef struct
{
    int id_file;
    EtatPartage *partage;
    int tube_ecriture;
    int numero_ascenseur;
    Configuration *configuration;
} ParametreAscenseur;

typedef struct
{
    int id_file;
    EtatPartage *partage;
    Configuration *configuration;
} ParametreScenario;

typedef struct
{
    int tube_lecture;
    EtatPartage *partage;
} ParametreAlertes;

#endif

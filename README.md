# Gestion d'ascenseurs

Première version fonctionnelle d'une simulation en C de deux ascenseurs dans un immeuble de 11 niveaux (0 à 10).

## Fonctionnalités de cette version

- Deux ascenseurs exécutés dans des processus distincts (`fork`).
- Utilisateurs simulés par des threads POSIX (`pthread`).
- Demandes générées aléatoirement (étage de départ et destination).
- Communication par file de messages System V.
- Attribution des demandes aux ascenseurs disponibles.
- Déplacement étage par étage et affichage dans le terminal.

Cette version constitue une base minimale : elle n'intègre pas encore l'optimisation avancée, les priorités, les pannes ni les statistiques.

## Compilation

Sous Linux :

```bash
make
```

## Exécution

```bash
make run
```

## Nettoyage

```bash
make clean
```

## Dépendances

- GCC
- Make
- pthreads
- IPC System V (files de messages)

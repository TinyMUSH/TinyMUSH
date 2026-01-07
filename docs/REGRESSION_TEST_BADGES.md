# Badges des Tests de Régression

Ce document explique comment les badges pour les tests de régression sont générés et affichés.

## Vue d'ensemble

Le workflow GitHub Actions [regression-test-ubuntu-latest.yml](../.github/workflows/regression-test-ubuntu-latest.yml) exécute automatiquement tous les fichiers de test de régression (`.conf`) dans le dossier `scripts/`. Un badge de résumé est généré et stocké dans la branche `badges` du dépôt.

## Badge de résumé

### Affichage

Un badge de résumé global affiche le nombre total de tests qui passent:

```markdown
![Regression Tests](https://raw.githubusercontent.com/TinyMUSH/TinyMUSH/badges/badges/regression-summary.svg)
```

Ce badge est affiché dans le [README.md](../README.md) principal du projet.

### Format

Le badge affiche:
- **"X passed"** (vert) si tous les tests passent
- **"X/Y passed"** (jaune) si certains tests échouent (X tests passés sur Y total)

### Mise à jour

Le badge est automatiquement mis à jour après chaque exécution du workflow de régression, qui se déclenche après chaque build réussi sur Ubuntu.

## Statistiques détaillées

Un fichier JSON contenant les statistiques détaillées est également généré et disponible à:

```
https://raw.githubusercontent.com/TinyMUSH/TinyMUSH/badges/badges/regression-stats.json
```

### Format du fichier JSON

```json
{
  "total": 43,
  "passed": 41,
  "failed": 2,
  "timestamp": "2026-01-07T15:30:00Z"
}
```

Champs:
- `total`: Nombre total de tests exécutés
- `passed`: Nombre de tests réussis
- `failed`: Nombre de tests échoués
- `timestamp`: Date et heure (UTC) de la dernière exécution

## Fonctionnement technique

Le workflow se compose de trois jobs principaux:

### 1. list-tests
Découvre automatiquement tous les fichiers `.conf` dans le dossier `scripts/`.

### 2. regression-test
- Exécute chaque test en parallèle dans une matrice de jobs
- Utilise `continue-on-error: true` pour ne pas arrêter le workflow en cas d'échec
- Sauvegarde le résultat de chaque test (success/failure) dans un fichier texte
- Upload les résultats comme artefacts

### 3. update-summary
- Télécharge tous les résultats des tests
- Compte le nombre de tests réussis et échoués
- Génère le badge de résumé avec shields.io
- Crée le fichier JSON de statistiques
- Commite les fichiers dans la branche `badges`

## Structure de la branche badges

```
badges/
├── README.md
├── regression-summary.svg    # Badge de résumé
└── regression-stats.json     # Statistiques détaillées
```

## Création initiale de la branche badges

Si la branche `badges` n'existe pas encore, créez-la:

```bash
git checkout --orphan badges
git rm -rf .
mkdir badges
echo "# Regression Test Badges" > README.md
git add README.md
git commit -m "Initialize badges branch"
git push origin badges
git checkout master
```

## Intégration dans le README

Le badge est déjà intégré dans le README principal:

```markdown
|Build status|
|------------|
|[![Build (Ubuntu)](https://github.com/TinyMUSH/TinyMUSH/actions/workflows/build-test-ubuntu-latest.yml/badge.svg)](https://github.com/TinyMUSH/TinyMUSH/actions/workflows/build-test-ubuntu-latest.yml)|
|[![Build (macOS)](https://github.com/TinyMUSH/TinyMUSH/actions/workflows/build-test-macos-latest.yml/badge.svg)](https://github.com/TinyMUSH/TinyMUSH/actions/workflows/build-test-macos-latest.yml)|
|[![Regression Test (Ubuntu)](https://github.com/TinyMUSH/TinyMUSH/actions/workflows/regression-test-ubuntu-latest.yml/badge.svg)](https://github.com/TinyMUSH/TinyMUSH/actions/workflows/regression-test-ubuntu-latest.yml)|
|![Regression Tests](https://raw.githubusercontent.com/TinyMUSH/TinyMUSH/badges/badges/regression-summary.svg)|
|[![Doxygen](https://github.com/TinyMUSH/TinyMUSH/actions/workflows/update-doxygen.yml/badge.svg)](https://github.com/TinyMUSH/TinyMUSH/actions/workflows/update-doxygen.yml)|
|[![CodeQL](https://github.com/TinyMUSH/TinyMUSH/actions/workflows/update-codeql.yml/badge.svg)](https://github.com/TinyMUSH/TinyMUSH/actions/workflows/update-codeql.yml)|
```

## Badges individuels par test (optionnel)

Si vous souhaitez afficher un badge pour chaque test individuel, vous pouvez utiliser le script `generate_badge_table.py`:

```bash
python3 scripts/generate_badge_table.py
```

Ce script génère une table markdown complète avec tous les tests organisés par catégorie.

Pour implémenter les badges individuels, vous devrez étendre le workflow pour générer un badge SVG pour chaque test (non implémenté actuellement, mais décrit dans une version précédente de ce document).

## Dépannage

### Le badge n'apparaît pas

1. Vérifiez que la branche `badges` existe:
   ```bash
   git ls-remote --heads origin badges
   ```

2. Vérifiez que le workflow a été exécuté au moins une fois:
   - Allez sur Actions → Regression Test (Ubuntu)
   - Vérifiez qu'une exécution s'est terminée

3. Vérifiez que le fichier badge existe dans la branche `badges`:
   ```bash
   git checkout badges
   ls -la badges/
   git checkout master
   ```

### Le badge affiche une erreur 404

L'URL du badge doit pointer vers la branche `badges`:
```
https://raw.githubusercontent.com/TinyMUSH/TinyMUSH/badges/badges/regression-summary.svg
                                                      ^^^^^^
                                                      nom de la branche
```

### Les statistiques ne se mettent pas à jour

Vérifiez les permissions du workflow:
- Le workflow doit avoir `contents: write` pour pouvoir pousser vers la branche `badges`
- Vérifiez dans Settings → Actions → General → Workflow permissions

## Améliorations futures

Possibles améliorations:
- Ajouter un badge pour chaque test individuel
- Générer un rapport HTML détaillé
- Envoyer des notifications en cas d'échec
- Créer un historique des tendances de tests
- Afficher le temps d'exécution de chaque test

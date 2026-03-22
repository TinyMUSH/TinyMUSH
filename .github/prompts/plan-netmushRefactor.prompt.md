## Plan: Refactorisation incrementale de src/netmush (analyse complete)

Ce plan est base sur une analyse de tous les sources declares dans NETMUSH_SOURCES.

- Total sources C analyses: 92
- Sources > 1000 lignes: 31
- Plus gros modules individuels: funstring.c (4035), game.c (3915), ansi.c (3818), predicates.c (3689), netcommon.c (3298), look.c (3159), funmisc.c (2555), funmath.c (2357), db_flatfile.c (2334), object.c (2110), eval.c (2081)
- Familles les plus volumineuses (somme par prefixe): db (7494), funvars (5755), funlist (4991), funobj (4566), command (4060), funstring (4035), bsd (2231), cque (2825)

Objectif: modulariser sans changement de comportement observable, en priorisant la reduction des gros fichiers et la baisse du couplage progressif.

## Etat d avancement (2026-03-22)

- Phase 1 entamee: `funstring.c` a deja ete decoupe (modules `funstring_check.c`, `funstring_search.c`, `funstring_format.c`) et le coordinateur principal est passe sous 1000 lignes.
- Phase 2 entamee: extraction de helpers objets vers `object_utils.c`; `object.c` est passe sous 1000 lignes.
- Phase 2 en cours: extraction de rendu depuis `look.c` vers `look_content.c`; `look.c` est reduit mais reste >1000 lignes.
- `eval.c` est deja partiellement module (`eval_parse.c`, `eval_regs.c`, `eval_tcache.c`, `eval_gender.c`), mais `eval.c` reste >1000 lignes.

Prochaine etape naturelle:
1. Continuer le decoupage de `look.c` (presentation/desc/dispatch) jusqu a passer sous 1000 lignes.
2. Enchainer sur `funmath.c` puis `eval.c` avec le meme cycle court (extraire -> compiler -> verifier).

Contraintes:
1. Refactor strictement structurel: deplacement de code, extraction de helpers, renommage minimal, ajustement d API interne si necessaire.
2. Maintenir les points d entree publics existants autant que possible.
3. Mettre a jour CMakeLists.txt a chaque ajout de source.
4. Garder prototypes.h comme point central des declarations publiques.
5. Toute etape est bloquante tant que la compilation n est pas propre.

## Phases mises a jour

### Phase 0 - Baseline et garde-fous (obligatoire)
1. Geler une baseline: taille des fichiers, nombre de fonctions publiques, nombre de helpers statiques par fichier cible.
2. Definir les familles de modules prioritaires par risque et volume.
3. Valider les commandes de verification (build + demarrage minimal).

### Phase 1 - Cibles a fort ROI et risque modere
Ordre recommande:
1. funstring.c (4035)
2. funmath.c (2357)
3. funmisc.c (2555)
4. funlist.c (1186) en coherence avec ses sous-modules deja presents (funlist_edit, funlist_sort, funlist_sets, funlist_tables, funlist_advanced)

Strategie:
- Extraire par sous-domaines stables (validation, formatage, recherche, operations ANSI, operations numeriques, etc.).
- Conserver un coordinateur mince dans le fichier principal.
- Deplacer les helpers statiques avec leur sous-domaine.

### Phase 2 - Couche objets et etat
Ordre recommande:
1. object.c (2110)
2. predicates.c (3689)
3. set.c (2020)
4. create.c (1782)
5. move.c (1135)
6. look.c (3159)

Strategie:
- Decoupage par responsabilite metier (resolution, permissions, mutation, presentation).
- Stabiliser les APIs utilitaires utilisees transversalement.

### Phase 3 - Stockage et attributs
Ordre recommande:
1. db_flatfile.c (2334)
2. db_attributes.c (1713)
3. db_objects.c (1657)
4. walkdb.c (1703)

Strategie:
- Separer lecture/ecriture, serialisation, indexation, migration.
- Encapsuler les details de stockage derriere des interfaces internes claires.

### Phase 4 - Runtime, commandes et reseau
Ordre recommande:
1. game.c (3915)
2. netcommon.c (3298)
3. command_admin.c (1996)
4. command_core.c (1386)
5. ansi.c (3818)

Strategie:
- Isoler startup/shutdown, dump/persistence, notification/runtime, utilitaires reseau.
- Eviter les dependances circulaires vers eval et predicates.

### Phase 5 - Noyau d evaluation (dernier)
Ordre recommande:
1. eval.c (2081)

Strategie:
- Cartographie explicite des dependances avant extraction.
- Decoupage cible: parsing/scan, evaluation coeur, substitutions, registres/trace.
- Preservation stricte des points d entree publics et des effets de bord existants.

### Phase 6 - Harmonisation transversale
1. Normaliser les noms de fichiers selon le pattern module_component.c.
2. Verifier la coherence prototypes.h / externs.h.
3. Repasser sur les coordinateurs pour eviter la re-monolithisation.

## Verification (a chaque extraction)
1. Build: compiler et corriger immediatement toute erreur ou warning regressif.
2. Lien: verifier l absence de symboles dupliques ou manquants.
3. Taille: verifier que le fichier source d origine diminue et que les nouveaux fichiers restent lisibles.
4. Runtime minimal: demarrer le serveur depuis game/ et verifier le demarrage sans regression evidente.
5. Validation ciblee selon phase:
- Phases 1-2: fonctions de base MUSH et commandes critiques
- Phase 3: lecture/ecriture attributs et persistence
- Phase 4: startup/shutdown, dump, notifications
- Phase 5: evaluation d expressions, substitutions, registres

## Cibles prioritaires (etat actuel)

Top modules > 1000 lignes a traiter en priorite:
1. src/netmush/funstring.c
2. src/netmush/game.c
3. src/netmush/ansi.c
4. src/netmush/predicates.c
5. src/netmush/netcommon.c
6. src/netmush/look.c
7. src/netmush/funmisc.c
8. src/netmush/funmath.c
9. src/netmush/db_flatfile.c
10. src/netmush/object.c
11. src/netmush/eval.c
12. src/netmush/set.c
13. src/netmush/command_admin.c
14. src/netmush/create.c
15. src/netmush/db_attributes.c
16. src/netmush/walkdb.c
17. src/netmush/db_objects.c
18. src/netmush/funiter.c
19. src/netmush/alloc.c
20. src/netmush/log.c
21. src/netmush/funvars_registers.c
22. src/netmush/command_core.c
23. src/netmush/flags.c
24. src/netmush/speech.c
25. src/netmush/htab.c
26. src/netmush/funlist_advanced.c
27. src/netmush/nametabs.c
28. src/netmush/funlist.c
29. src/netmush/move.c
30. src/netmush/wild.c
31. src/netmush/conf_core.c

## Decisions
- Prioriser par volume ET couplage, pas par taille seule.
- Appliquer un cycle court: extraire -> compiler -> verifier -> continuer.
- Reporter toute extraction a risque eleve (eval, game) apres stabilisation des patterns sur les phases 1-4.
- Ne pas introduire de changement fonctionnel dans ce chantier.

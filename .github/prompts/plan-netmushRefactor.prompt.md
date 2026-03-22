## Plan: Refactorisation incrementale de src/netmush (analyse complete)

Ce plan est base sur une analyse de tous les sources declares dans NETMUSH_SOURCES.

- Total sources C analyses: 118
- Sources > 1000 lignes: 29
- Plus gros modules individuels: game.c (3915), ansi.c (3818), netcommon.c (3298), db_flatfile.c (2334), set.c (2020), command_admin.c (1996), create.c (1782), db_attributes.c (1713), walkdb.c (1703), funstring_format.c (1694), db_objects.c (1657)
- Familles les plus volumineuses (somme par prefixe): a recalculer en continu apres chaque lot de refactor.

Objectif: modulariser sans changement de comportement observable, en priorisant la reduction des gros fichiers et la baisse du couplage progressif.

## Etat d avancement (2026-03-22)

- Validation branches: toutes les branches locales restantes (hors master et gh-pages) sont deja incluses dans origin/master.
- Phase 1 avancee:
  - funstring.c est sous 1000 lignes (971) avec modules funstring_check.c, funstring_search.c, funstring_format.c.
  - funmath.c est passe sous 1000 lignes (811) via funmath_compare.c, funmath_arith.c, funmath_bitwise.c.
  - funmisc.c est sous 1000 lignes (906).
  - point restant notable de la famille funstring: funstring_format.c (1694).
- Phase 2 avancee:
  - object.c est sous 1000 lignes (851), avec object_utils.c (202) et object_check.c (1120).
  - predicates.c est sous 1000 lignes (642), avec predicates_eval.c (1352) et predicates_*.c.
  - look.c est passe sous 1000 lignes (936) avec look_pretty.c (485), mais look_content.c reste a 1115.
- Evaluation:
  - eval.c est deja partiellement module (eval_parse.c, eval_regs.c, eval_tcache.c, eval_gender.c), mais eval.c reste a 1174 lignes.

Prochaine etape naturelle:
1. Continuer le decoupage du sous-systeme look pour faire passer look_content.c sous 1000 lignes.
2. Traiter eval.c (1174), en cycle court extraire -> compiler -> verifier.
3. Reprioriser ensuite sur les plus gros modules runtime/stockage: game.c, ansi.c, netcommon.c, db_flatfile.c, set.c, command_admin.c.

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
1. funstring_format.c (1694)
2. funiter.c (1579)
3. funlist.c (1174)
4. funlist_advanced.c (1253)

Strategie:
- Extraire par sous-domaines stables (validation, formatage, recherche, operations ANSI, operations numeriques, etc.).
- Conserver un coordinateur mince dans le fichier principal.
- Deplacer les helpers statiques avec leur sous-domaine.

### Phase 2 - Couche objets et etat
Ordre recommande:
1. look_content.c (1115)
2. set.c (2020)
3. create.c (1782)
4. move.c (1135)
5. predicates_eval.c (1352)
6. object_check.c (1120)

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
1. eval.c (1174)

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
1. src/netmush/game.c (3915)
2. src/netmush/ansi.c (3818)
3. src/netmush/netcommon.c (3298)
4. src/netmush/db_flatfile.c (2334)
5. src/netmush/set.c (2020)
6. src/netmush/command_admin.c (1996)
7. src/netmush/create.c (1782)
8. src/netmush/db_attributes.c (1713)
9. src/netmush/walkdb.c (1703)
10. src/netmush/funstring_format.c (1694)
11. src/netmush/db_objects.c (1657)
12. src/netmush/funiter.c (1579)
13. src/netmush/alloc.c (1548)
14. src/netmush/log.c (1434)
15. src/netmush/funvars_registers.c (1399)
16. src/netmush/command_core.c (1386)
17. src/netmush/flags.c (1367)
18. src/netmush/speech.c (1355)
19. src/netmush/predicates_eval.c (1352)
20. src/netmush/htab.c (1349)
21. src/netmush/funlist_advanced.c (1253)
22. src/netmush/nametabs.c (1237)
23. src/netmush/funlist.c (1174)
24. src/netmush/eval.c (1174)
25. src/netmush/move.c (1135)
26. src/netmush/object_check.c (1120)
27. src/netmush/look_content.c (1115)
28. src/netmush/wild.c (1062)
29. src/netmush/conf_core.c (1041)

## Decisions
- Prioriser par volume ET couplage, pas par taille seule.
- Appliquer un cycle court: extraire -> compiler -> verifier -> continuer.
- Reporter toute extraction a risque eleve (eval, game) apres stabilisation des patterns sur les phases 1-4.
- Ne pas introduire de changement fonctionnel dans ce chantier.

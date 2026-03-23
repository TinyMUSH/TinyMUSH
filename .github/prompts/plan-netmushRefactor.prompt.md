## Plan: Refactorisation incrementale de src/netmush (analyse complete)

Ce plan est base sur une analyse de tous les sources declares dans NETMUSH_SOURCES.

- Total sources C analyses: 124
- Sources > 1000 lignes: 26
- Plus gros modules individuels: game.c (3915), ansi.c (3818), netcommon.c (3298), db_flatfile.c (2334), set.c (2020), command_admin.c (1996), create.c (1782), db_attributes.c (1713), walkdb.c (1703), db_objects.c (1657), alloc.c (1548)
- Familles les plus volumineuses (somme par prefixe): a recalculer en continu apres chaque lot de refactor.

Objectif: modulariser sans changement de comportement observable, en priorisant la reduction des gros fichiers et la baisse du couplage progressif.

## Etat d avancement (2026-03-22) - MIS A JOUR

- Validation branches: toutes les branches locales restantes (hors master et gh-pages) sont deja incluses dans origin/master.
- Revalidation technique complete: ./rebuild.sh OK (build + redemarrage), avec un warning preexistant dans create.c sur const char * -> char *.
- Phase 1 avancee:
  - funstring.c est sous 1000 lignes (971) avec modules funstring_check.c, funstring_search.c, funstring_format.c.
  - funmath.c est passe sous 1000 lignes (811) via funmath_compare.c, funmath_arith.c, funmath_bitwise.c.
  - funmisc.c est sous 1000 lignes (906).
  - funstring_format.c est passe sous 1000 lignes (596) via extraction de funstring_border.c (382) et funstring_align.c (760).
  - funiter.c est passe sous 1000 lignes (611) via extraction de funiter_apply.c (559) et funiter_compose.c (460).
  - funlist.c est passe sous 1000 lignes (978) via extraction de funlist_utils.c (218): autodetect_list, get_list_type, validate_list_args, dbnum.
  - funlist_advanced.c est passe sous 1000 lignes (815) via extraction de funlist_filter.c (482): fun_elements, fun_exclude.
- Phase 2 avancee:
  - object.c est sous 1000 lignes (851), avec object_utils.c (202) et object_check.c (1120).
  - predicates.c est sous 1000 lignes (642), avec predicates_eval.c (1352) et predicates_*.c.
  - look.c est passe sous 1000 lignes (936) avec look_pretty.c (485).
  - look_content.c est passe sous 1000 lignes (562) via extraction de look_sweep.c (313) et look_decomp.c (281).
- Evaluation:
  - eval.c est passe sous 1000 lignes (714) via extraction de eval_subst.c (536): _eval_expand_percent() (handler de substitution %-).
  - eval est maintenant totalement module: eval_parse.c, eval_regs.c, eval_tcache.c, eval_gender.c, eval_subst.c.

Prochaine etape naturelle:
1. Traiter set.c (2020) et create.c (1782) (Phase 2).
2. Traiter db_flatfile.c (2334), db_attributes.c (1713), db_objects.c (1657), walkdb.c (1703) (Phase 3).
3. Traiter game.c (3915), ansi.c (3818), netcommon.c (3298), command_admin.c (1996), command.c (4186) (Phase 4).
4. Traiter les modules moyens restants: alloc.c (1548), log.c (1434), funvars_registers.c (1399), command_core.c (1386), flags.c (1367), speech.c (1355), predicates_eval.c (1352), htab.c (1349), nametabs.c (1237), move.c (1135), object_check.c (1120), wild.c (1062), conf_core.c (1041).

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
1. ~~funstring_format.c (1694)~~ DONE → funstring_format.c (596) + funstring_border.c (394) + funstring_align.c (760)
2. ~~funiter.c (1579)~~ DONE → funiter.c (611) + funiter_apply.c (559) + funiter_compose.c (460)
3. ~~funlist.c (1174)~~ DONE → funlist.c (978) + funlist_utils.c (218)
4. ~~funlist_advanced.c (1253)~~ DONE → funlist_advanced.c (815) + funlist_filter.c (482)

Strategie:
- Extraire par sous-domaines stables (validation, formatage, recherche, operations ANSI, operations numeriques, etc.).
- Conserver un coordinateur mince dans le fichier principal.
- Deplacer les helpers statiques avec leur sous-domaine.

### Phase 2 - Couche objets et etat
Ordre recommande:
1. ~~look_content.c (1115)~~ DONE → look_content.c (562) + look_sweep.c (318) + look_decomp.c (281)
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
1. ~~eval.c (1174)~~ DONE → eval.c (714) + eval_subst.c (536)
   eval est maintenant totalement module: eval_parse.c, eval_regs.c, eval_tcache.c, eval_gender.c, eval_subst.c.

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
1. src/netmush/command.c (4186)
2. src/netmush/game.c (3915)
3. src/netmush/ansi.c (3818)
4. src/netmush/netcommon.c (3298)
5. src/netmush/db_flatfile.c (2334)
6. src/netmush/set.c (2020)
7. src/netmush/command_admin.c (1996)
8. src/netmush/create.c (1782)
9. src/netmush/db_attributes.c (1713)
10. src/netmush/walkdb.c (1703)
11. src/netmush/db_objects.c (1657)
12. src/netmush/alloc.c (1548)
13. src/netmush/log.c (1434)
14. src/netmush/funvars_registers.c (1399)
15. src/netmush/command_core.c (1386)
16. src/netmush/flags.c (1367)
17. src/netmush/speech.c (1355)
18. src/netmush/predicates_eval.c (1352)
19. src/netmush/htab.c (1349)
20. src/netmush/nametabs.c (1237)
21. src/netmush/move.c (1135)
22. src/netmush/object_check.c (1120)
23. src/netmush/wild.c (1062)
24. src/netmush/db_backend_gdbm.c (1056)
25. src/netmush/conf_core.c (1041)

Modules recemment reduits (< 1000 lignes):
- funstring_format.c: 1694 → 596 (+ funstring_border.c 382, funstring_align.c 760)
- look_content.c: 1115 → 562 (+ look_sweep.c 313, look_decomp.c 281)
- funiter.c: 1579 → 611 (+ funiter_apply.c 559, funiter_compose.c 460)
- funmath.c: 1333 → 811 (+ funmath_compare.c 174, funmath_arith.c 305, funmath_bitwise.c 122)
- funlist.c: 1174 → 978 (+ funlist_utils.c 218)
- funlist_advanced.c: 1253 → 815 (+ funlist_filter.c 482)
- eval.c: 1174 → 714 (+ eval_subst.c 536)

## Decisions
- Prioriser par volume ET couplage, pas par taille seule.
- Appliquer un cycle court: extraire -> compiler -> verifier -> continuer.
- Reporter toute extraction a risque eleve (eval, game) apres stabilisation des patterns sur les phases 1-4.
- Ne pas introduire de changement fonctionnel dans ce chantier.

#!/bin/bash
# Test pour reproduire la perte d'attributs lors du shutdown
# Ce script teste que les modifications d'attributs sont bien persistées

set -e

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
GAME_DIR="${ROOT_DIR}/game"
PID_FILE="${GAME_DIR}/netmush.pid"
LOG_FILE="${GAME_DIR}/logs/netmush.log"

echo "=== Test de persistance des attributs ==="
echo

# Sauvegarder la DB actuelle
if [ -f "${GAME_DIR}/db/netmush.db" ]; then
    echo "Sauvegarde de la DB actuelle..."
    cp -p "${GAME_DIR}/db/netmush.db" "${GAME_DIR}/db/netmush.db.test_backup"
fi

# Arrêter le serveur s'il tourne
if [ -f "$PID_FILE" ] && kill -0 $(cat "$PID_FILE") 2>/dev/null; then
    echo "Arrêt du serveur existant..."
    kill -QUIT $(cat "$PID_FILE")
    sleep 2
fi

# Démarrer le serveur
echo "Démarrage du serveur..."
cd "$GAME_DIR"
./netmush &
sleep 3

# Vérifier que le serveur est démarré
if [ ! -f "$PID_FILE" ]; then
    echo "ERREUR: Le serveur n'a pas démarré"
    exit 1
fi

PID=$(cat "$PID_FILE")
echo "Serveur démarré (PID: $PID)"
echo

# Créer un objet de test et ajouter plusieurs attributs
echo "Test 1: Ajout d'attributs et shutdown normal"
{
    sleep 1
    echo "connect #1 Ctrl8088"
    sleep 1
    echo "@create Test_Attr_Persistence=10"
    sleep 1
    echo "&TEST_ATTR1 #3=Valeur 1"
    sleep 1
    echo "&TEST_ATTR2 #3=Valeur 2 avec plus de texte"
    sleep 1
    echo "&TEST_ATTR3 #3=Valeur 3"
    sleep 1
    echo "think u([get(#3/TEST_ATTR1)] [get(#3/TEST_ATTR2)] [get(#3/TEST_ATTR3)])"
    sleep 1
    echo "QUIT"
    sleep 1
} | nc -q 1 localhost 6250 > /tmp/test_attr_output1.txt 2>&1 &

sleep 8
echo "Attributs définis:"
grep -E "TEST_ATTR|Valeur" /tmp/test_attr_output1.txt || echo "(pas trouvé dans la sortie)"
echo

# Shutdown normal
echo "Arrêt normal du serveur..."
kill -QUIT $PID
sleep 3

# Attendre que le dump soit terminé
while kill -0 $PID 2>/dev/null; do
    echo "Attente de la fin du dump..."
    sleep 1
done
echo "Serveur arrêté"
echo

# Redémarrer et vérifier
echo "Redémarrage du serveur..."
cd "$GAME_DIR"
./netmush &
sleep 3

PID=$(cat "$PID_FILE")
echo "Serveur redémarré (PID: $PID)"
echo

# Vérifier que les attributs sont toujours présents
echo "Vérification de la persistance des attributs..."
{
    sleep 1
    echo "connect #1 Ctrl8088"
    sleep 1
    echo "think Attr1=[get(#3/TEST_ATTR1)]"
    sleep 1
    echo "think Attr2=[get(#3/TEST_ATTR2)]"
    sleep 1
    echo "think Attr3=[get(#3/TEST_ATTR3)]"
    sleep 1
    echo "examine #3/TEST_*"
    sleep 1
    echo "QUIT"
    sleep 1
} | nc -q 1 localhost 6250 > /tmp/test_attr_output2.txt 2>&1 &

sleep 8
echo "Attributs après redémarrage:"
grep -E "Attr[123]=|TEST_ATTR" /tmp/test_attr_output2.txt || echo "ERREUR: Attributs perdus!"
echo

# Arrêter le serveur
kill -QUIT $PID 2>/dev/null || true
sleep 2

# Vérifier le résultat
echo
echo "=== Résultat du test ==="
if grep -q "Valeur 1" /tmp/test_attr_output2.txt && \
   grep -q "Valeur 2" /tmp/test_attr_output2.txt && \
   grep -q "Valeur 3" /tmp/test_attr_output2.txt; then
    echo "✓ SUCCÈS: Les attributs ont été persistés correctement"
    EXIT_CODE=0
else
    echo "✗ ÉCHEC: Les attributs n'ont pas été persistés"
    EXIT_CODE=1
fi

# Restaurer la DB
if [ -f "${GAME_DIR}/db/netmush.db.test_backup" ]; then
    echo
    read -p "Restaurer la DB de sauvegarde? (o/N) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Oo]$ ]]; then
        mv "${GAME_DIR}/db/netmush.db.test_backup" "${GAME_DIR}/db/netmush.db"
        echo "DB restaurée"
    fi
fi

exit $EXIT_CODE

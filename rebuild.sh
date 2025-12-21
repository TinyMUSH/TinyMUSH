#!/bin/bash

# Couleurs pour l'output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}=== TinyMUSH Upgrade Script ===${NC}"

# 1. Arrêt propre du serveur s'il est en cours d'exécution
if pgrep -x "netmush" > /dev/null; then
    echo -e "${YELLOW}Fermeture propre du serveur...${NC}"
    
    # Envoyer SIGTERM pour arrêt gracieux
    kill $(pgrep -x netmush) 2>/dev/null
    
    # Attendre le shutdown gracieux (max 30 secondes)
    for i in {1..30}; do
        if ! pgrep -x "netmush" > /dev/null; then
            echo -e "${GREEN}Serveur arrêté proprement${NC}"
            break
        fi
        if [ $i -eq 30 ]; then
            echo -e "${RED}Timeout - forçage de l'arrêt${NC}"
            killall -9 netmush 2>/dev/null
        else
            sleep 1
            echo -n "."
        fi
    done
    echo ""
fi

# 2. Attendre un peu avant de continuer
sleep 2

# 3. Compilation
echo -e "${YELLOW}Compilation du serveur...${NC}"
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..

if [ $? -ne 0 ]; then
    echo -e "${RED}Erreur lors de la configuration CMake${NC}"
    exit 1
fi

cmake --build . --target install-upgrade -j$(nproc)

if [ $? -ne 0 ]; then
    echo -e "${RED}Erreur lors de la compilation${NC}"
    exit 1
fi

cd ..

# 4. Redémarrage du serveur
echo -e "${YELLOW}Redémarrage du serveur...${NC}"
cd game

# Vérifier que le serveur est bien arrêté
if pgrep -x "netmush" > /dev/null; then
    echo -e "${RED}Erreur : Le serveur est toujours en cours d'exécution${NC}"
    exit 1
fi

# Lancer le serveur (fork automatique libère le terminal)
if [ -f "netmush" ]; then
    echo -e "${YELLOW}Démarrage du serveur...${NC}"
    ./netmush 2>&1
    
    # Le serveur fork et le parent quitte. Attendre que le fichier PID soit créé
    # par le processus enfant qui devient le vrai serveur
    echo -e "${YELLOW}Vérification du démarrage...${NC}"
    PID_FILE="netmush.pid"
    
    for i in {1..10}; do
        if [ -f "$PID_FILE" ]; then
            SERVER_PID=$(cat "$PID_FILE")
            echo -e "${GREEN}Serveur lancé (PID: $SERVER_PID)${NC}"
            break
        fi
        sleep 1
    done
    
    if [ -z "$SERVER_PID" ]; then
        echo -e "${RED}Erreur : Le PID du serveur n'a pas pu être déterminé${NC}"
        exit 1
    fi
    
    # Vérifier que le serveur est bien en cours d'exécution
    if kill -0 $SERVER_PID 2>/dev/null; then
        echo -e "${GREEN}=== Upgrade complété avec succès ===${NC}"
    else
        echo -e "${RED}Erreur : Le serveur s'est arrêté après le démarrage${NC}"
        exit 1
    fi
else
    echo -e "${RED}Erreur : binaire netmush non trouvé${NC}"
    exit 1
fi

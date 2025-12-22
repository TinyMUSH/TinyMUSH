#!/bin/bash

# Couleurs pour l'output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}=== TinyMUSH Upgrade Script ===${NC}"

# Répertoire racine du dépôt (emplacement de ce script)
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

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

# 3.5. Option de restauration DB à partir d'un backup local
BACKUP_FILE="${ROOT_DIR}/netmush.db.backup"
GAME_DB_DIR="${ROOT_DIR}/game/db"
GAME_DB_FILE="${GAME_DB_DIR}/netmush.db"
BACKUPS_DIR="${ROOT_DIR}/game/backups"

if [ -f "$BACKUP_FILE" ]; then
    echo -e "${YELLOW}Un backup local a été détecté: ${BACKUP_FILE}${NC}"
    echo -e "${YELLOW}Souhaitez-vous remplacer la base du jeu (${GAME_DB_FILE}) par ce backup ?${NC}"
    read -r -p "Confirmer le remplacement ? [y/N] " REPLY
    case "$REPLY" in
        [yY]|[yY][eE][sS]|[oO]|[oO][uU][iI])
            mkdir -p "$GAME_DB_DIR" "$BACKUPS_DIR"
            TS="$(date +%Y%m%d-%H%M%S)"
            if [ -f "$GAME_DB_FILE" ]; then
                SAFE_BAK="$BACKUPS_DIR/netmush.db.pre-rebuild-${TS}"
                echo -e "${YELLOW}Sauvegarde de l'ancienne DB: ${SAFE_BAK}${NC}"
                cp -p "$GAME_DB_FILE" "$SAFE_BAK"
            fi
            echo -e "${YELLOW}Restauration du backup vers ${GAME_DB_FILE}...${NC}"
            cp -f "$BACKUP_FILE" "$GAME_DB_FILE"
            chmod 600 "$GAME_DB_FILE" 2>/dev/null || true
            echo -e "${GREEN}Base de données restaurée depuis le backup.${NC}"
            ;;
        *)
            echo -e "${YELLOW}Restauration ignorée (réponse: ${REPLY:-N}).${NC}"
            ;;
    esac
fi

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

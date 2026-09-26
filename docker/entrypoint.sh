#!/bin/bash

# Copy docs if destination is empty
if [ ! -d "/app/game/docs" ] || [ -z "$(ls -A /app/game/docs)" ]; then
    echo "Copying docs-original to docs..."
    cp -r /app/game/docs-original/* /app/game/docs/
fi

# Copy configs if destination is empty
if [ ! -d "/app/game/configs" ] || [ -z "$(ls -A /app/game/configs)" ]; then
    echo "Copying configs-original to configs..."
    cp -r /app/game/configs-original/* /app/game/configs/
fi

# Run the game
/app/game/netmush --debug

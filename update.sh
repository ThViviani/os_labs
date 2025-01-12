#!/bin/bash

# Остановить выполнение при ошибке
set -e

# Проверка наличия обновлений
echo "Checking for updates..."
git fetch origin

LOCAL=$(git rev-parse HEAD)
REMOTE=$(git rev-parse origin/main)
BASE=$(git merge-base HEAD origin/main)

if [ "$LOCAL" = "$REMOTE" ]; then
    echo "No updates available. The local branch is up-to-date with the remote branch."
    exit 0
elif [ "$LOCAL" = "$BASE" ]; then
    echo "Updates are available. Pulling changes from the remote branch..."
    git pull origin main
    echo "The project has been successfully updated!"
elif [ "$REMOTE" = "$BASE" ]; then
    echo "Your local changes are ahead of the remote branch. Consider pushing them."
    exit 0
else
    echo "Local and remote branches have diverged. Please resolve conflicts manually."
    exit 1
fi

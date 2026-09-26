## TinyMUSH Docker README

### Quick Start
1. Copy the Dockerfile, compose.yaml, and entrypoint.sh into a folder to run from, e.g. ~/docker/tinymush/
2. Create subfolders: data/, data/db/, data/logs/, data/backups/, data/configs/, and data/docs/
3. Change the UID and GID values in the Dockerfile (in two places) and in compose.yaml (in one place) if you want the application not to run as UID and GID 1000.
4. Run "docker compose up -d;docker compose logs --follow" to start the server and observe the logs.
5. Run "telnet localhost 6250" to connect.
6. As the wizard user, using the @shutdown command will cause the container to exit after the application shuts down.  Be sure to run "docker compose down" before starting it up again.

### Dockerfile
The Dockerfile pulls the source from github, builds it in the first image, creates a second image, copies the game files and supporting binaries into the second, resulting in a smaller tinymush4 image.  Change the UID and GID values in the Dockerfile (in two places) if you want the application not to run as UID and GID 1000.

### compose.yaml
Change the UID and GID values if you want the application not to run as UID and GID 1000.  It expects the following volume folders and runs on the default port 6250.  
- ./data/db/
- ./data/logs/
- ./data/backups/
- ./data/configs/
- ./data/docs/

### entrypoint.sh
This script is used by the Dockerfile and copied into the container image.  It runs each time the container starts.  Each time the container starts, if either of these volume folders is empty, it will copy a set of default files stored in the container image into them:
- ./data/docs/
- ./data/configs/

This ensures a default set of docs and configs are in place at first run time.  You can customize them after that initial run.  Deleting all files in a given folder will result in a new set being copied out from the container when it starts.  This can be handy if you customize the files and break something.

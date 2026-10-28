# WSL Install
Open PowerShell as administrator.

Run ```bash wsl --install``` to enable WSL if not already installed.

Reboot your machine if prompted.

Run ```bash wsl --list --online``` to view available distributions.

Install Ubuntu 26.04 with ```bash wsl --install Ubuntu-26.04```.

Follow prompts to create a username and password.

# Docker Install
```bash
curl -fsSL https://get.docker.com -o get-docker.sh
sudo sh get-docker.sh
```

# VS Code Configurattion
1. Install WSL
2. Install Dev Container exrension

# Project setup
1. Connect to WSL (use Ctrl + Shift + P)
2. Clone project to your directory
3. Open prject in VS Code
4. Reopen in container (use Ctrl + Shift + P)
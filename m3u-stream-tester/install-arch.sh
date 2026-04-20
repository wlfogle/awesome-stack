#!/usr/bin/env bash
set -euo pipefail

# Manjaro/Arch setup helper for this repository
TIMEOUT="${TIMEOUT:-25s}"

if ! command -v pacman >/dev/null 2>&1; then
  echo "This script is intended for Manjaro/Arch (pacman). Exiting."
  exit 1
fi

sudo -v || { echo "sudo privileges are required"; exit 1; }

sudo pacman -S --needed --noconfirm base-devel git curl wget jq ripgrep fd pkgconf cmake make unzip
sudo pacman -S --needed --noconfirm python python-pip python-virtualenv nodejs npm rustup cargo

if [ -d "src-tauri" ] || [ -f "src-tauri/Cargo.toml" ] || rg -q --no-messages 'tauri' Cargo.toml 2>/dev/null; then
  sudo pacman -S --needed --noconfirm gtk3 libappindicator-gtk3 librsvg || true
  if pacman -Si webkit2gtk-4.1 >/dev/null 2>&1; then
    sudo pacman -S --needed --noconfirm webkit2gtk-4.1 || true
  else
    sudo pacman -S --needed --noconfirm webkit2gtk || true
  fi
  if ! command -v rustc >/dev/null 2>&1; then
    rustup default stable || true
  fi
fi

if [ -f "requirements.txt" ]; then
  python -m venv .venv
  source .venv/bin/activate
  pip install -U pip
  pip install -r requirements.txt
fi

if [ -f "pyproject.toml" ] && [ ! -f "requirements.txt" ]; then
  python -m venv .venv
  source .venv/bin/activate
  pip install -U pip build setuptools wheel
  pip install -U poetry pdm hatchling || true
fi

echo "[OK] Manjaro setup complete for $(basename "$PWD")."

#!/usr/bin/env bash
# Fastfetch config
sed -i 's/--config neofetch/--config mokka/g' ~/.config/fish/config.fish

# Non-default starship config
mv ~/.config/starship-mokka.toml ~/.config/starship.toml

# Micro config
sed -i 's/geany/catppuccin-mocha/g' ~/.config/micro/settings.json

# Apply fish theme
yes | fish_config theme save "Catppuccin Mocha"

# Regen bat cache for new theme
bat cache --build

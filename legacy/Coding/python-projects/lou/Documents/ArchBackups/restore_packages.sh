#!/bin/bash
# ArchForge Pro Package Restoration Script
# Generated on: Mon Jun 23 15:29:53 2025

echo "ArchForge Pro - Package Restoration"
echo "======================================"

# Update package database
echo "Updating package database..."
sudo pacman -Sy

# Install explicitly installed packages (official repos)
echo "Installing official repository packages..."
sudo pacman -S --needed --noconfirm 7zip alsa-firmware appmenu-gtk-module arj ark autorandr b43-fwcutter base base-devel bash-completion bind bluedevil bluetooth-support bridge-utils btrfs-assistant btrfs-progs bzip2 chaotic-keyring chaotic-mirrorlist cmake coreutils cryptsetup curlftpfs dialog dmidecode dmraid dolphin dolphin-plugins dosfstools downgrade dracut e2fsprogs ecryptfs-utils efibootmgr ethtool exfatprogs f2fs-tools fastfetch fatresize ffmpegthumbs file filesystem findutils firedragon-catppuccin firedragon-extension-plasma-integration firewalld freetype2 fscrypt fwupd garuda-boot-options garuda-browser-settings garuda-common-settings garuda-dracut-support garuda-fish-config garuda-hooks garuda-hotfixes garuda-icons garuda-mokka garuda-network-assistant garuda-nvidia-prime-config garuda-rani garuda-settings-manager garuda-setup-assistant garuda-system-maintenance garuda-video-linux-config garuda-wallpapers gawk gcc-libs gettext glibc gnu-netcat grep grub grub-btrfs grub-garuda gstreamer-meta gwenview gzip htop inetutils input-devices-support intel-ucode inxi iproute2 iptables-nft iputils jfsutils kate kde-gtk-config kdeconnect kdegraphics-thumbnailers kdeplasma-addons kf6-servicemenus-rootactions kimageformats kinfocenter kinit konsole kscreen ksshaskpass kvantum kwallet-pam kwayland-integration kwin kwin-effect-rounded-corners-git kwin-effects-forceblur lhasa lib32-pipewire-jack libappindicator-gtk3 libdvdcss libinput-gestures-qt licenses lightdm lightdm-gtk-greeter linux-firmware linux-zen linux-zen-headers logrotate lrzip lsb-release lvm2 lzip lzop man-db man-pages mdadm memtest86+ micro mtools nano net-tools networkmanager-support nfs-utils nilfs-utils nmap noto-fonts noto-fonts-cjk noto-fonts-emoji nss-mdns ntfs-3g octopi okular openh264 os-prober-btrfs pacman pacman-contrib partitionmanager paru pciutils perl-file-mimeinfo pipewire-jack pipewire-support plasma-applet-window-buttons plasma-browser-integration plasma-desktop plasma-firewall plasma-nm plasma-pa plasma-systemmonitor plasma-thunderbolt plasma6-applets-window-title plasma6-wallpapers-blurredwallpaper plocate power-profiles-daemon powerdevil powertop procps-ng psmisc qt6-imageformats qt6-quick3d quota-tools rate-mirrors resvg rsync sddm sddm-kcm sed shadow snapper-support snapper-tools sof-firmware spectacle sshfs sudo systemd systemd-sysvcompat tar tela-circle-icon-theme-dracula terminus-font traceroute ttf-dejavu ttf-fantasque-sans-mono ttf-fira-sans ttf-liberation ttf-opensans ugrep unace unarchiver unarj unrar unzip update-grub usbutils util-linux vi vlc warp-terminal wget which whois wireless-regdb wireless_tools wireplumber wqy-zenhei xdg-desktop-portal xdg-desktop-portal-gtk xdg-desktop-portal-kde xdg-user-dirs xdg-utils xfsprogs xorg-bdftopcf xorg-iceauth xorg-mkfontscale xorg-sessreg xorg-smproxy xorg-x11perf xorg-xbacklight xorg-xcmsdb xorg-xcursorgen xorg-xdriinfo xorg-xev xorg-xgamma xorg-xhost xorg-xinit xorg-xinput xorg-xkbevd xorg-xkbprint xorg-xkbutils xorg-xkill xorg-xlsatoms xorg-xlsclients xorg-xmodmap xorg-xpr xorg-xrefresh xorg-xsetroot xorg-xvinfo xorg-xwd xorg-xwininfo xorg-xwud xsel xz zip

# Install AUR packages (requires AUR helper like yay or paru)
echo "Installing AUR packages..."
echo "Package restoration completed!"
echo "Please verify that all packages are installed correctly."

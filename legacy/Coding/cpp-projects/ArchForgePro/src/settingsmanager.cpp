#include "settingsmanager.h"
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>
#include <QDirIterator>
#include <QRegularExpression>
#include <QDebug>

SettingsManager::SettingsManager(QObject *parent)
    : QObject(parent)
{
}

SettingsManager::~SettingsManager()
{
}

void SettingsManager::refreshSettingsList()
{
    m_settingFiles.clear();
    emit operationProgress("Scanning Arch Linux system components...", 0);
    
    // 1. Critical System Configuration Files
    scanSystemConfigs();
    emit operationProgress("System configs scanned", 10);
    
    // 2. User Dotfiles and Configurations
    scanUserConfigs();
    emit operationProgress("User configs scanned", 20);
    
    // 3. Pacman Configuration and Hooks
    scanPacmanComponents();
    emit operationProgress("Pacman components scanned", 30);
    
    // 4. Systemd Services and Units
    scanSystemdComponents();
    emit operationProgress("Systemd components scanned", 40);
    
    // 5. Network Configuration
    scanNetworkConfigs();
    emit operationProgress("Network configs scanned", 50);
    
    // 6. Boot Configuration
    scanBootConfigs();
    emit operationProgress("Boot configs scanned", 60);
    
    // 7. Desktop Environment Configs
    scanDesktopConfigs();
    emit operationProgress("Desktop configs scanned", 70);
    
    // 8. Virtual Machines and Containers
    scanVirtualMachines();
    emit operationProgress("VMs and containers scanned", 80);
    
    // 9. BTRFS Snapshots (if available)
    scanBtrfsSnapshots();
    emit operationProgress("BTRFS snapshots scanned", 90);
    
    // 10. Additional System Components
    scanAdditionalComponents();
    emit operationProgress("Additional components scanned", 95);
    
    m_lastRefreshTime = QDateTime::currentDateTime();
    emit operationProgress(QString("Scan completed - Found %1 items").arg(m_settingFiles.size()), 100);
    emit settingsListRefreshed();
}

QList<SettingFile> SettingsManager::getSettingFiles() const
{
    return m_settingFiles;
}

QList<SettingFile> SettingsManager::getSystemSettings() const
{
    QList<SettingFile> systemSettings;
    for (const auto &file : m_settingFiles) {
        if (file.isSystemConfig) {
            systemSettings.append(file);
        }
    }
    return systemSettings;
}

QList<SettingFile> SettingsManager::getUserSettings() const
{
    QList<SettingFile> userSettings;
    for (const auto &file : m_settingFiles) {
        if (file.isUserConfig) {
            userSettings.append(file);
        }
    }
    return userSettings;
}

void SettingsManager::backupSettings(const QString &location)
{
    QDir().mkpath(location);
    QString settingsArchive = location + "/settings_backup_" + 
                             QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".tar.gz";
    
    QProcess process;
    QStringList args = {"-czf", settingsArchive};
    
    // Add all setting files to the archive
    for (const auto &file : m_settingFiles) {
        args << file.path;
    }
    
    process.start("tar", args);
    process.waitForFinished();
    
    emit operationProgress("Settings backup completed", 100);
}

void SettingsManager::exportSettings(const QString &fileName)
{
    QProcess process;
    QStringList args = {"-czf", fileName};
    
    for (const auto &file : m_settingFiles) {
        args << file.path;
    }
    
    process.start("tar", args);
    process.waitForFinished();
    
    emit operationProgress("Settings exported", 100);
}

void SettingsManager::importSettings(const QString &fileName)
{
    QProcess process;
    process.start("tar", {"-xzf", fileName, "-C", "/"});
    process.waitForFinished();
    
    emit operationProgress("Settings imported", 100);
}

QStringList SettingsManager::getConfigDirectories() const
{
    QStringList dirs;
    dirs << "/etc";
    dirs << QDir::homePath() + "/.config";
    dirs << QDir::homePath() + "/.local/share";
    return dirs;
}

QList<SettingFile> SettingsManager::searchSettings(const QString &query) const
{
    QList<SettingFile> results;
    for (const auto &file : m_settingFiles) {
        if (file.name.contains(query, Qt::CaseInsensitive) ||
            file.path.contains(query, Qt::CaseInsensitive)) {
            results.append(file);
        }
    }
    return results;
}

void SettingsManager::scanDirectory(const QString &path, bool isSystem)
{
    QDir dir(path);
    if (!dir.exists()) return;
    
    QFileInfoList entries = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    
    for (const QFileInfo &entry : entries) {
        if (entry.isFile()) {
            // Only include configuration files
            QString suffix = entry.suffix().toLower();
            QStringList configSuffixes = {"conf", "cfg", "ini", "rc", "config", "xml", "json", "yaml", "yml"};
            
            if (configSuffixes.contains(suffix) || entry.fileName().startsWith(".")) {
                m_settingFiles.append(createSettingFile(entry.absoluteFilePath(), isSystem));
            }
        } else if (entry.isDir() && entry.fileName() != "." && entry.fileName() != "..") {
            // Recursively scan subdirectories (limited depth)
            if (!entry.absoluteFilePath().contains("/.git/") && 
                !entry.absoluteFilePath().contains("/.cache/")) {
                scanDirectory(entry.absoluteFilePath(), isSystem);
            }
        }
    }
}

SettingFile SettingsManager::createSettingFile(const QString &filePath, bool isSystem) const
{
    QFileInfo info(filePath);
    SettingFile file;
    file.path = filePath;
    file.name = info.fileName();
    file.size = info.size();
    file.modified = info.lastModified();
    file.isSystemConfig = isSystem;
    file.isUserConfig = !isSystem;
    return file;
}

void SettingsManager::scanSystemConfigs()
{
    // Critical system configuration files
    QStringList systemConfigs = {
        "/etc/fstab",                    // File system table
        "/etc/hosts",                    // Host file
        "/etc/hostname",                 // System hostname
        "/etc/locale.conf",              // System locale
        "/etc/vconsole.conf",            // Virtual console config
        "/etc/mkinitcpio.conf",          // Initial ramdisk config
        "/etc/modprobe.d",               // Kernel module config
        "/etc/modules-load.d",           // Module loading config
        "/etc/sysctl.d",                 // Kernel parameters
        "/etc/udev/rules.d",             // Device rules
        "/etc/X11",                      // X11 configuration
        "/etc/environment",              // Environment variables
        "/etc/profile",                  // System profile
        "/etc/bash.bashrc",              // System bash config
        "/etc/sudoers",                  // Sudo configuration
        "/etc/passwd",                   // User accounts
        "/etc/group",                    // Group information
        "/etc/shadow",                   // Password hashes
        "/etc/gshadow",                  // Group passwords
        "/etc/motd",                     // Message of the day
        "/etc/issue",                    // Login banner
        "/etc/fonts",                    // Font configuration
        "/etc/gtk-2.0",                  // GTK2 system config
        "/etc/gtk-3.0",                  // GTK3 system config
        "/etc/lightdm",                  // Display manager config
        "/etc/gdm",                      // GNOME display manager
        "/etc/sddm",                     // SDDM display manager
        "/etc/xdg"                       // XDG base directories
    };
    
    for (const QString &path : systemConfigs) {
        addIfExists(path, true);
    }
}

void SettingsManager::scanUserConfigs()
{
    QString homeDir = QDir::homePath();
    
    // User configuration directories
    QStringList userConfigDirs = {
        homeDir + "/.config",
        homeDir + "/.local/share",
        homeDir + "/.local/bin",
        homeDir + "/.themes",
        homeDir + "/.icons",
        homeDir + "/.fonts",
        homeDir + "/.gnupg",
        homeDir + "/.ssh"
    };
    
    for (const QString &dir : userConfigDirs) {
        if (QDir(dir).exists()) {
            scanDirectory(dir, false);
        }
    }
    
    // Common dotfiles
    QStringList dotfiles = {
        ".bashrc", ".bash_profile", ".bash_history",
        ".zshrc", ".zsh_history", ".oh-my-zsh",
        ".vimrc", ".vim", ".nvim",
        ".gitconfig", ".gitignore_global",
        ".tmux.conf", ".tmux",
        ".xinitrc", ".xprofile", ".Xresources", ".Xdefaults",
        ".profile", ".pam_environment",
        ".inputrc", ".dircolors",
        ".gtkrc-2.0", ".gtkrc",
        ".mozilla", ".thunderbird",
        ".chromium", ".google-chrome",
        ".kde4", ".kde",
        ".dmrc", ".face", ".face.icon",
        ".selected_editor", ".sudo_as_admin_successful"
    };
    
    for (const QString &dotfile : dotfiles) {
        QString path = homeDir + "/" + dotfile;
        addIfExists(path, false);
    }
}

void SettingsManager::scanPacmanComponents()
{
    // Pacman configuration and databases
    QStringList pacmanPaths = {
        "/etc/pacman.conf",              // Main pacman config
        "/etc/pacman.d",                 // Pacman configuration directory
        "/etc/makepkg.conf",             // Package building config
        "/usr/share/libalpm/hooks",      // System hooks
        "/etc/pacman.d/hooks",           // Custom hooks
        "/var/lib/pacman/local",         // Package database
        "/etc/xdg/reflector",            // Mirror list updater
        "/etc/systemd/system/reflector.timer", // Reflector timer
        "/etc/systemd/system/reflector.service" // Reflector service
    };
    
    for (const QString &path : pacmanPaths) {
        addIfExists(path, true);
    }
    
    // AUR helpers configuration
    QString homeDir = QDir::homePath();
    QStringList aurConfigs = {
        homeDir + "/.config/yay",
        homeDir + "/.config/paru",
        homeDir + "/.config/pikaur",
        homeDir + "/.makepkg.conf"
    };
    
    for (const QString &path : aurConfigs) {
        addIfExists(path, false);
    }
}

void SettingsManager::scanSystemdComponents()
{
    // Systemd configuration
    QStringList systemdPaths = {
        "/etc/systemd/system",           // Custom system units
        "/etc/systemd/user",             // Custom user units
        "/etc/systemd/system.conf",      // System manager config
        "/etc/systemd/user.conf",        // User manager config
        "/etc/systemd/logind.conf",      // Login manager config
        "/etc/systemd/journald.conf",    // Journal config
        "/etc/systemd/resolved.conf",    // DNS resolver config
        "/etc/systemd/timesyncd.conf",   // Time synchronization
        "/etc/systemd/networkd.conf",    // Network manager config
        "/etc/tmpfiles.d",               // Temporary files config
        "/etc/sysusers.d"                // System users config
    };
    
    for (const QString &path : systemdPaths) {
        addIfExists(path, true);
    }
    
    // User systemd services
    QString homeDir = QDir::homePath();
    QString userSystemd = homeDir + "/.config/systemd/user";
    addIfExists(userSystemd, false);
}

void SettingsManager::scanNetworkConfigs()
{
    // Network configuration files
    QStringList networkPaths = {
        "/etc/systemd/network",          // Systemd network config
        "/etc/NetworkManager",           // NetworkManager config
        "/etc/netctl",                   // Netctl profiles
        "/etc/wpa_supplicant",           // WiFi configuration
        "/etc/dhcpcd.conf",              // DHCP client config
        "/etc/resolv.conf",              // DNS configuration
        "/etc/nsswitch.conf",            // Name service switch
        "/etc/hosts.deny",               // TCP wrappers deny
        "/etc/hosts.allow",              // TCP wrappers allow
        "/etc/iptables",                 // Firewall rules
        "/etc/ufw",                      // Uncomplicated firewall
        "/etc/fail2ban"                  // Intrusion prevention
    };
    
    for (const QString &path : networkPaths) {
        addIfExists(path, true);
    }
}

void SettingsManager::scanBootConfigs()
{
    // Boot configuration
    QStringList bootPaths = {
        "/boot/loader",                  // systemd-boot config
        "/etc/default/grub",             // GRUB configuration
        "/boot/grub",                    // GRUB directory
        "/etc/grub.d",                   // GRUB scripts
        "/boot/syslinux",                // Syslinux config
        "/boot/refind_linux.conf",       // rEFInd config
        "/boot/EFI"                      // EFI system partition
    };
    
    for (const QString &path : bootPaths) {
        addIfExists(path, true);
    }
}

void SettingsManager::scanDesktopConfigs()
{
    QString homeDir = QDir::homePath();
    
    // Desktop environment configurations
    QStringList desktopPaths = {
        // KDE/Plasma
        homeDir + "/.config/kde.org",
        homeDir + "/.config/plasma*",
        homeDir + "/.config/kwin*",
        homeDir + "/.config/kglobalshortcuts*",
        homeDir + "/.kde",
        homeDir + "/.kde4",
        
        // GNOME
        homeDir + "/.config/dconf",
        homeDir + "/.local/share/gnome-shell",
        homeDir + "/.config/gtk-*",
        
        // XFCE
        homeDir + "/.config/xfce4",
        homeDir + "/.config/Thunar",
        
        // i3/Sway
        homeDir + "/.config/i3",
        homeDir + "/.config/sway",
        homeDir + "/.config/waybar",
        homeDir + "/.config/rofi",
        homeDir + "/.config/dunst",
        
        // Other WMs
        homeDir + "/.config/awesome",
        homeDir + "/.config/bspwm",
        homeDir + "/.config/openbox",
        homeDir + "/.config/herbstluftwm",
        
        // Applications
        homeDir + "/.config/alacritty",
        homeDir + "/.config/kitty",
        homeDir + "/.config/terminator",
        homeDir + "/.config/Code",
        homeDir + "/.config/discord",
        homeDir + "/.config/spotify"
    };
    
    for (const QString &path : desktopPaths) {
        // Handle wildcards
        if (path.contains("*")) {
            QDir dir(QFileInfo(path).absolutePath());
            QString pattern = QFileInfo(path).fileName();
            QStringList matches = dir.entryList(QStringList() << pattern, QDir::Dirs);
            for (const QString &match : matches) {
                addIfExists(dir.absoluteFilePath(match), false);
            }
        } else {
            addIfExists(path, false);
        }
    }
}

void SettingsManager::scanVirtualMachines()
{
    QString homeDir = QDir::homePath();
    
    // Virtual machine configurations and images
    QStringList vmPaths = {
        // VirtualBox
        homeDir + "/.config/VirtualBox",
        homeDir + "/VirtualBox VMs",
        
        // VMware
        homeDir + "/.vmware",
        homeDir + "/vmware",
        
        // QEMU/KVM
        homeDir + "/.config/libvirt",
        "/etc/libvirt",
        
        // Docker
        homeDir + "/.docker",
        "/etc/docker",
        
        // Podman
        homeDir + "/.config/containers",
        "/etc/containers",
        
        // LXC/LXD
        "/var/lib/lxc",
        "/etc/lxc",
        homeDir + "/.config/lxc"
    };
    
    for (const QString &path : vmPaths) {
        addIfExists(path, path.startsWith("/etc") || path.startsWith("/var"));
    }
}

void SettingsManager::scanBtrfsSnapshots()
{
    // Check if BTRFS is used
    QProcess fsCheck;
    fsCheck.start("findmnt", {"-t", "btrfs", "-o", "TARGET"});
    fsCheck.waitForFinished();
    
    if (fsCheck.exitCode() == 0) {
        QString output = fsCheck.readAllStandardOutput();
        QStringList btrfsMounts = output.split("\n", Qt::SkipEmptyParts);
        
        for (const QString &mount : btrfsMounts) {
            if (mount == "TARGET") continue; // Skip header
            
            // Look for snapshot directories
            QStringList snapshotPaths = {
                mount + "/.snapshots",
                mount + "/@snapshots",
                mount + "/snapshots"
            };
            
            for (const QString &snapPath : snapshotPaths) {
                if (QDir(snapPath).exists()) {
                    addDirectoryInfo(snapPath, "BTRFS Snapshots", true);
                }
            }
        }
    }
}

void SettingsManager::scanAdditionalComponents()
{
    QString homeDir = QDir::homePath();
    
    // Additional important components
    QStringList additionalPaths = {
        // Cron jobs
        "/etc/crontab",
        "/etc/cron.d",
        "/var/spool/cron",
        homeDir + "/.crontab",
        
        // Log configuration
        "/etc/logrotate.conf",
        "/etc/logrotate.d",
        "/etc/rsyslog.conf",
        "/etc/rsyslog.d",
        
        // Security
        "/etc/security",
        "/etc/pam.d",
        "/etc/apparmor.d",
        "/etc/selinux",
        
        // System monitoring
        "/etc/munin",
        "/etc/nagios",
        "/etc/zabbix",
        
        // Development tools
        homeDir + "/.cargo",
        homeDir + "/.rustup",
        homeDir + "/.npm",
        homeDir + "/.config/pip",
        homeDir + "/.pyenv",
        homeDir + "/.rbenv",
        homeDir + "/.nvm",
        
        // Game configurations
        homeDir + "/.steam",
        homeDir + "/.local/share/Steam",
        homeDir + "/.wine",
        homeDir + "/.lutris",
        
        // Certificates
        "/etc/ssl",
        "/etc/ca-certificates",
        
        // Time zone
        "/etc/localtime",
        "/etc/timezone"
    };
    
    for (const QString &path : additionalPaths) {
        addIfExists(path, !path.startsWith(homeDir));
    }
}

void SettingsManager::addIfExists(const QString &path, bool isSystem)
{
    if (QFileInfo::exists(path)) {
        if (QFileInfo(path).isDir()) {
            scanDirectory(path, isSystem);
        } else {
            m_settingFiles.append(createSettingFile(path, isSystem));
        }
    }
}

void SettingsManager::addDirectoryInfo(const QString &path, const QString &description, bool isSystem)
{
    if (QDir(path).exists()) {
        SettingFile file;
        file.path = path;
        file.name = QFileInfo(path).fileName() + " (" + description + ")";
        file.size = 0; // Directory size calculation would be expensive
        file.modified = QFileInfo(path).lastModified();
        file.isSystemConfig = isSystem;
        file.isUserConfig = !isSystem;
        m_settingFiles.append(file);
    }
}

#include "packageinfo.h"
#include <QJsonObject>

QString PackageInfo::methodString() const {
    switch (method) {
        case InstallMethod::PACMAN: return "Pacman";
        case InstallMethod::YAY: return "YAY";
        case InstallMethod::PARU: return "Paru";
        case InstallMethod::PIKAUR: return "Pikaur";
        case InstallMethod::TRIZEN: return "Trizen";
        case InstallMethod::AURMAN: return "Aurman";
        case InstallMethod::AURA: return "Aura";
        case InstallMethod::PAKKU: return "Pakku";
        case InstallMethod::PIP: return "PIP";
        case InstallMethod::PIPX: return "PIPX";
        case InstallMethod::CONDA: return "Conda";
        case InstallMethod::MAMBA: return "Mamba";
        case InstallMethod::FLATPAK: return "Flatpak";
        case InstallMethod::SNAP: return "Snap";
        case InstallMethod::APPIMAGE: return "AppImage";
        case InstallMethod::GIT: return "Git";
        case InstallMethod::LOCAL: return "Local";
        case InstallMethod::WEB_DOWNLOAD: return "Web Download";
        case InstallMethod::BINARY_RELEASE: return "Binary Release";
        case InstallMethod::SOURCE_BUILD: return "Source Build";
        case InstallMethod::DOCKER: return "Docker";
        case InstallMethod::NIX: return "Nix";
        case InstallMethod::HOMEBREW: return "Homebrew";
        default: return "Unknown";
    }
}

QString PackageInfo::categoryString() const {
    switch (category) {
        case PackageCategory::DEVELOPMENT: return "Development";
        case PackageCategory::SYSTEM: return "System";
        case PackageCategory::MULTIMEDIA: return "Multimedia";
        case PackageCategory::GAMES: return "Games";
        case PackageCategory::INTERNET: return "Internet";
        case PackageCategory::OFFICE: return "Office";
        case PackageCategory::GRAPHICS: return "Graphics";
        case PackageCategory::EDUCATION: return "Education";
        case PackageCategory::SCIENCE: return "Science";
        case PackageCategory::UTILITIES: return "Utilities";
        case PackageCategory::SECURITY: return "Security";
        case PackageCategory::TERMINAL: return "Terminal";
        case PackageCategory::OTHER: return "Other";
        default: return "Other";
    }
}

QJsonObject PackageInfo::toJson() const {
    QJsonObject json;
    json["name"] = name;
    json["method"] = static_cast<int>(method);
    json["source"] = source;
    json["description"] = description;
    json["version"] = version;
    json["size"] = size;
    json["category"] = static_cast<int>(category);
    json["popularity"] = popularity;
    json["lastUpdated"] = lastUpdated;
    json["maintainer"] = maintainer;
    json["license"] = license;
    json["url"] = url;
    json["homepage"] = homepage;
    json["installed"] = installed;
    json["securityScore"] = securityScore;
    json["compatibilityScore"] = compatibilityScore;
    json["recommendationReason"] = recommendationReason;
    return json;
}

PackageInfo PackageInfo::fromJson(const QJsonObject &json) {
    PackageInfo pkg;
    pkg.name = json["name"].toString();
    pkg.method = static_cast<InstallMethod>(json["method"].toInt());
    pkg.source = json["source"].toString();
    pkg.description = json["description"].toString();
    pkg.version = json["version"].toString();
    pkg.size = json["size"].toString();
    pkg.category = static_cast<PackageCategory>(json["category"].toInt());
    pkg.popularity = json["popularity"].toInt();
    pkg.lastUpdated = json["lastUpdated"].toString();
    pkg.maintainer = json["maintainer"].toString();
    pkg.license = json["license"].toString();
    pkg.url = json["url"].toString();
    pkg.homepage = json["homepage"].toString();
    pkg.installed = json["installed"].toBool();
    pkg.securityScore = json["securityScore"].toDouble();
    pkg.compatibilityScore = json["compatibilityScore"].toDouble();
    pkg.recommendationReason = json["recommendationReason"].toString();
    return pkg;
}

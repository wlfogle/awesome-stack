#ifndef PACKAGEINFO_H
#define PACKAGEINFO_H

#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QJsonObject>

enum class InstallMethod {
    PACMAN,
    YAY,
    PARU,
    PIKAUR,
    TRIZEN,
    AURMAN,
    AURA,
    PAKKU,
    PIP,
    PIPX,
    CONDA,
    MAMBA,
    FLATPAK,
    SNAP,
    APPIMAGE,
    GIT,
    LOCAL,
    WEB_DOWNLOAD,
    BINARY_RELEASE,
    SOURCE_BUILD,
    DOCKER,
    NIX,
    HOMEBREW
};

enum class PackageCategory {
    DEVELOPMENT,
    SYSTEM,
    MULTIMEDIA,
    GAMES,
    INTERNET,
    OFFICE,
    GRAPHICS,
    EDUCATION,
    SCIENCE,
    UTILITIES,
    SECURITY,
    TERMINAL,
    OTHER
};

struct PackageInfo {
    QString name;
    InstallMethod method;
    QString source;
    QString description;
    QString version;
    QString size;
    QStringList dependencies;
    QStringList optionalDeps;
    QStringList postInstall;
    PackageCategory category;
    int popularity;
    QString lastUpdated;
    QString maintainer;
    QString license;
    QString url;
    QString homepage;
    bool installed;
    QDateTime installDate;
    double securityScore;
    double compatibilityScore;
    QString recommendationReason;

    PackageInfo() :
        method(InstallMethod::PACMAN),
        category(PackageCategory::OTHER),
        popularity(0),
        installed(false),
        securityScore(0.0),
        compatibilityScore(0.0)
    {}

    // Convert to/from JSON for storage
    QJsonObject toJson() const;
    static PackageInfo fromJson(const QJsonObject &json);
    
    // Utility methods
    QString methodString() const;
    QString categoryString() const;
    static InstallMethod stringToMethod(const QString &str);
    static PackageCategory stringToCategory(const QString &str);
};

// Helper functions
QString installMethodToString(InstallMethod method);
QString packageCategoryToString(PackageCategory category);
InstallMethod stringToInstallMethod(const QString &str);
PackageCategory stringToPackageCategory(const QString &str);

#endif // PACKAGEINFO_H

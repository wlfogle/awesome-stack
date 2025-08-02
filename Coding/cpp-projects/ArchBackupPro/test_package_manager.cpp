#include "src/packagemanager.h"
#include <QCoreApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    PackageManager pm;
    
    qDebug() << "Testing PackageManager functionality...";
    
    // Test backup functionality
    pm.backupPackageList("/home/lou/Documents/ArchBackups");
    
    qDebug() << "Package backup test completed.";
    
    return 0;
}

#include <QCoreApplication>
#include "server.h"

int main(int argc, char *argv[])
{
    QCoreApplication application(argc, argv);

    Server server;  // Création d'une instance du serveur
    server.startServer();  // Démarrer le serveur

    return application.exec();  // Lancer la boucle d'événements Qt
}

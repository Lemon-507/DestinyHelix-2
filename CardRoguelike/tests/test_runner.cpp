#include <QCoreApplication>

int runSkeletonTests(int argc, char *argv[]);
int runMapShrinkTests(int argc, char *argv[]);
int runCombatTests(int argc, char *argv[]);
int runNodeTests(int argc, char *argv[]);
int runGameManagerTests(int argc, char *argv[]);
int runRelicTests(int argc, char *argv[]);
int runIntegrationTests(int argc, char *argv[]);
int runEnemyTests(int argc, char *argv[]);
int runCardTextTests(int argc, char *argv[]);

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    int status = 0;
    status |= runSkeletonTests(argc, argv);
    status |= runMapShrinkTests(argc, argv);
    status |= runCombatTests(argc, argv);
    status |= runNodeTests(argc, argv);
    status |= runGameManagerTests(argc, argv);
    status |= runRelicTests(argc, argv);
    status |= runIntegrationTests(argc, argv);
    status |= runEnemyTests(argc, argv);
    status |= runCardTextTests(argc, argv);
    return status;
}

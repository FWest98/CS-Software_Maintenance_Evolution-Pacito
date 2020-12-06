#include "Pacito.h"
#include "patterns/Patterns.h"

Pacito::Pacito(char *classPath) {
    option = new Option();
    option->setClasspath(classPath);

    control = new Control(*option);
}

Pacito::~Pacito() {
    delete option;
    delete control;
}

int Pacito::run(char **files) {
    control->run(files);
}
#ifndef PACITO_PACITO_H
#define PACITO_PACITO_H

#include "option.h"
#include "control.h"

class Pacito {
private:
    Option* option;
    Control* control;

public:
    Pacito(char *);
    ~Pacito();

    RunStats run(char **);
    Control* getControl() { return control; }
};


#endif //PACITO_PACITO_H

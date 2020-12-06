#ifndef PACITO_PACITO_H
#define PACITO_PACITO_H

#include "../headers/Pacito_Pinot.h"
#include "option.h"
#include "control.h"

class Pacito {
private:
    Option* option;
    Control* control;

public:
    Pacito(char *);
    ~Pacito();

    int run(char **);
    Control* getControl() { return control; }
};


#endif //PACITO_PACITO_H

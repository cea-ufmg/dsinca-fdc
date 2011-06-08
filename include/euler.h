#ifndef EULER
#define EULER

#include <iostream>
#include "quaternio.h"

class Quat;

class Euler{
    public:
        double phi; 
        double  theta; 
        double  psi; 
        
        Euler();
        Euler(Quat); // Converte quaternio para euler
        
        Euler& operator=(Euler euler);
        Euler operator-(Euler euler);
        void imprime();
};
#endif

#ifndef GIRO
#define GIRO

#include <iostream>
#include "quaternio.h"
#include "euler.h"

class Giro{
    public:
        // Dados recebidos dos acelerômetros
        double p[4];
        double q[4];
        double r[4];

        //Dados recebidos dos girômetros filtrados
        float p_f[4];
        float q_f[4];
        float r_f[4];

        float aL[3];
        float bL[3];

        int angleRegion;
        
        Euler angle[2];

        //Módulo dos girômetros
        double mod;

        //Quatérnios usados na integração
        Quat quat[4];
        Quat dquat_dt, K1, K2, K3, K4;

        Giro();
       void filter();
        int makeContinuous();
        void getAngles();
        void refresh();

};
#endif

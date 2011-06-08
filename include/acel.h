#ifndef ACEL
#define ACEL

#include <iostream>
#include "euler.h"
#include "giro.h"
#include <string>

using namespace std;

class Acel{
    public:
        // Dados recebidos dos acelerômetros
        double x[3];
        double y[3];
        double z[3];

        // Dados recebidos dos acelerômetros filtrados
        double x_f[3];
        double y_f[3];
        double z_f[3];
        
        Euler angle[2];
        
        //Módulo dos aceletrometros
        double mod;

        //FILTRO PARA OS ACELERÔMETROS | 2ª ordem | Frequência de corte = 10 Hz
         double aL[3];
         double bL[3];

         double theta_X0[2];
         double theta_X1[2];
         double theta_X2[2];
        
         double phi_X0[2];
         double phi_X1[2];
         double phi_X2[2];

         double A_AL[3][3];
         double A_BL[3];
         double A_CL[3];
         double A_DL;
        
         double B_AL[3][3];
         double B_BL[3];
        double B_CL[3];
         double B_DL;
        
         double A[3][3];
         double B[3];
         double C[3];
         double D;
        
        int filterState;
        int angleRegion;

        Acel();
        int makeContinuous();
        void filter(string);
        double filter(string, Giro);
        void getAngles();
        void refresh();
};
#endif

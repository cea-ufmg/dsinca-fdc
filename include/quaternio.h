#ifndef QUATERNIO
#define QUATERNIO

#include "euler.h"

class Euler;

class Quat{
     public:
          double q0; 
          double q1; 
          double q2; 
          double q3;
          
          void imprime();
          
          Quat();
          Quat(Euler); // Converte euler para quaternio
          Quat(double,double,double,double);
          
          Quat operator*(double);
          Quat operator/(double);
          Quat operator+(Quat quat);
          Quat& operator=(const Quat quat);
          double sumPow();
          Quat modelQuat(double, double, double);
};

#endif

#include "quaternio.h"
#include <math.h> 
#include <iostream>

using namespace std; 

Quat::Quat(double _q0,double _q1, double _q2, double _q3){
     q0 = _q0;
     q1 = _q1;
     q2 = _q2;
     q3 = _q3;
}
////////////////////////////////////////////////////////////////////////////////
Quat::Quat(){
     q0 = -1;
     q1 = 0;
     q2 = 0;
     q3 = 0;
}
////////////////////////////////////////////////////////////////////////////////
void Quat::imprime(){
     cout << q0 << " | " << q1 << " | " << q2 << " | " << q3 << endl;
}
////////////////////////////////////////////////////////////////////////////////
Quat& Quat::operator=(Quat quat){
     
     q0 = quat.q0;
     q1 = quat.q1;
     q2 = quat.q2;
     q3 = quat.q3;
     
     return *this;
}    
////////////////////////////////////////////////////////////////////////////////
// Multiplica quaternio por constante
Quat Quat::operator*(double k){
     
     Quat aux;
     
     aux.q0 = q0*k;
     aux.q1 = q1*k;
     aux.q2 = q2*k;
     aux.q3 = q3*k;
     
     return aux;
}
////////////////////////////////////////////////////////////////////////////////
// Divide quaternio por constante
Quat Quat::operator/(double k){
     
     Quat aux;
     
     aux.q0 = q0/k;
     aux.q1 = q1/k;
     aux.q2 = q2/k;
     aux.q3 = q3/k;
     
     return aux;
}
////////////////////////////////////////////////////////////////////////////////
// Soma 2 quaternios
Quat Quat::operator+(Quat quat){
     
     Quat aux;
     
     aux.q0 = q0 + quat.q0;
     aux.q1 = q1 + quat.q1;
     aux.q2 = q2 + quat.q2;
     aux.q3 = q3 + quat.q3;
     
     return aux;
}
////////////////////////////////////////////////////////////////////////////////
// Soma de quaternios ao quadrado
double Quat::sumPow(){
     
     double aux = q0*q0 + q1*q1 + q2*q2 + q3*q3;
     return aux;
}  
////////////////////////////////////////////////////////////////////////////////
// Modelo que expressa a variação temporal do quaternio em função do vetor velocidade angular
Quat Quat::modelQuat(double p, double q, double r){

    // dq_dt = 1/2*A*B
    // 
    // |q0'|       | 0  -p  -q  -r | |q0|
    // |q1'| = 1/2*| p   0   r  -q |*|q1|
    // |q2'|       | p  -r   0   p | |q2|
    // |q3'|       | r   q  -p   0 | |q3|
    
    // Implementar aqui a calibaçao dos sensores!

    // calculo do modelo
    double A[4][4] = {{ 0, -p, -q, -r },
                     { p,  0,  r, -q },
                     { q, -r,  0,  p },
                     { r,  q, -p,  0 }};
    
    // Multiplicando por 1/2
    for (int i = 0; i<4; i++){
        for (int j = 0; j<4; j++){ 
            A[i][j] *= 0.5;
        }
    }
        
    double B[] = {q0, 
                 q1,
                 q2,
                 q3};
 
    double Y[4];
    
    for (int i = 0; i<4; i++){
        Y[i] = 0;
        for (int j = 0; j<4; j++)
            Y[i] += A[i][j]*B[j];
    }
    
    Quat dq_dt(Y[0],Y[1],Y[2],Y[3]);
    return dq_dt;
}
////////////////////////////////////////////////////////////////////////////////
// Converte representação em euler para quaternios
Quat::Quat(Euler euler){
                     // Em radianos
     double c1 = cos((euler.phi/2)/57.295);
     double c2 = cos((euler.theta/2)/57.295);
     double c3 = cos((euler.psi/2)/57.295);
     double s1 = sin((euler.phi/2)/57.295);
     double s2 = sin((euler.theta/2)/57.295);
     double s3 = sin((euler.psi/2)/57.295);
    
     //q0 = - [cos(phi/2)cos(teta/2)cos(psi/2) + sen(phi/2)sen(teta/2)sen(psi/2)]
     //q1 = - [sen(phi/2)cos(teta/2)cos(psi/2) + cos(phi?/2)sen(teta/2)sen(psi/2)]
     //q2 = - [cos(phi/2)sen(teta/2)cos(psi/2) + sen(phi?/2)cos(teta/2)sen(psi/2)]
     //q3 = - [cos(phi/2)cos(teta/2)sen(psi/2) + sen(phi?/2)sen(teta/2)cos(psi/2)]
    
     q0 = -(c1*c2*c3+s1*s2*s3); 
     q1 = -(s1*c2*c3-c1*s2*s3); 
     q2 = -(c1*s2*c3+s1*c2*s3); 
     q3 = -(c1*c2*s3-s1*s2*c3); 

}


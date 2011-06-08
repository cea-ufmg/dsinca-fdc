#include "euler.h"
#include <iostream>
#include <math.h> 

using namespace std; 

Euler::Euler(){
}
////////////////////////////////////////////////////////////////////////////////
// Imprime angulos de euler
void Euler::imprime(){
     cout << phi << " | " << theta << " | " << psi << endl;
}
////////////////////////////////////////////////////////////////////////////////
Euler& Euler::operator=(Euler euler){

     phi   = euler.phi;
     theta = euler.theta;
     psi   = euler.psi;

     return *this;
}
////////////////////////////////////////////////////////////////////////////////
Euler Euler::operator-(Euler euler){
     
     Euler aux;
     
     aux.phi   = phi   - euler.phi;
     aux.theta = theta - euler.theta;
     aux.psi   = psi   - euler.psi;

     return aux;
}
////////////////////////////////////////////////////////////////////////////////
// Obtém-se angulos de atitude em graus a partir de um quaternio
Euler::Euler(Quat Q){

    // phi   = arctan [2(q2q3 + q0q1)/(q0q0 - q1q1 - q2q2 + q3q3)]
    // theta = arcsin [-2(q1q3 + q0q2)]
    // psi   = arctan [2(q1q2 + q0q3)/(q0q0 + q1q1 - q2q2 - q3q3)]

    phi   = atan2( 2*(Q.q2*Q.q3 + Q.q0*Q.q1), (Q.q0*Q.q0 - Q.q1*Q.q1 - Q.q2*Q.q2 + Q.q3*Q.q3) );
    theta = asin (-2*(Q.q1*Q.q3 - Q.q0*Q.q2) );
    psi  = atan2( 2*(Q.q1*Q.q2 + Q.q0*Q.q3), (Q.q0*Q.q0 + Q.q1*Q.q1 - Q.q2*Q.q2 - Q.q3*Q.q3) );
    
    // q1q2 + q0q3 = 0.5
    if ( (Q.q1*Q.q2 + Q.q3*Q.q0) == 0.5){
        phi   = 0;
        theta = 3.141592/2;
        psi   = 2 * atan2(Q.q2,Q.q0);
    }
    // q1q2 + q0q3 = -0.5
    if ( (Q.q1*Q.q2 + Q.q3*Q.q0) == -0.5){
        phi   = 0 ;
        theta = -3.141592/2;
        psi   = -2 * atan2(Q.q2,Q.q0);
    }
    
    // Converte para graus
    phi   = (phi)*57.295;
    theta = (theta)*57.295;
    psi   = (psi)*57.295;
    
}

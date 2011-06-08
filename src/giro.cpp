#include "giro.h"
#include <iostream>
#include <math.h>
#include "quaternio.h"
#include "euler.h"

//#include <windows.h>
#include <time.h>
//#include <conio.h>
#include <stdio.h>
//#include <cmath.h>

using namespace std;

Giro::Giro(){

    p[0] = 0;  p[1] = 0;  p[2] = 0;   p[3] = 0;
    q[0] = 0;  q[1] = 0;  q[2] = 0;   q[3] = 0;
    r[0] = 0;  r[1] = 0;  r[2] = 0;   r[3] = 0;
    
//Girometros filtrados
    p_f[0] = 0;  p_f[1] = 0;  p_f[2] = 0;  p_f[3] = 0;
    q_f[0] = 0;  q_f[1] = 0;  q_f[2] = 0;  q_f[3] = 0;
    r_f[0] = 0;  r_f[1] = 0;  r_f[2] = 0;  r_f[3] = 0;

    angleRegion = 0;

    //==========================================================================
    // FILTRO ANTES DA DECIMAÇÃO | 2ª ordem | Frequência de corte = 20 Hz
    //==========================================================================
    aL[0] = 1.000000000000000;    aL[1] = -0.369527377351241;    aL[2] = 0.195815712655833;
    bL[0] = 0.206572083826148;    bL[1] =  0.413144167652296;    bL[2] = 0.206572083826148;

}

////////////////////////////////////////////////////////////////////////////////
// Filtra os girômetros
void Giro::filter(){

    //FILTRO PASSA-BAIXAS NOS ACELERÔMETROS
    p_f[2] = bL[0]*p[2] + bL[1]*p[1] + bL[2]*p[0]
            - aL[1]/aL[0]*p_f[1] - aL[2]/aL[0]*p_f[0];

    q_f[2] = bL[0]*q[2] + bL[1]*q[1] + bL[2]*q[0]
            - aL[1]/aL[0]*q_f[1] - aL[2]/aL[0]*q_f[0];

    r_f[2] = bL[0]*r[2] + bL[1]*r[1] + bL[2]*r[0]
            - aL[1]/aL[0]*r_f[1] - aL[2]/aL[0]*r_f[0];

}

void Giro::getAngles(){

    float T = 0.02;

    //cout << p[0] << " | " << endl;//<< q[0] << " | " << r[0] << " | " << T << endl;

    //INTEGRA GIRÔMETROS
    dquat_dt =  quat[2].modelQuat( p_f[0], q_f[0], r_f[0] );                                      K1 = dquat_dt*(T);
    dquat_dt = (quat[2] + K1/2).modelQuat( (p_f[0]+p_f[1])/2, (q_f[0]+q_f[1])/2, (r_f[0]+r_f[1])/2 );   K2 = dquat_dt*(T);
    dquat_dt = (quat[2] + K2/2).modelQuat( (p_f[0]+p_f[1])/2, (q_f[0]+q_f[1])/2, (r_f[0]+r_f[1])/2 );   K3 = dquat_dt*(T);
    dquat_dt = (quat[2] + K3).modelQuat( p_f[1], q_f[1], r_f[1] );                                K4 = dquat_dt*(T);
    quat[3]  =  quat[2] + K1/6 + K2/3 + K3/3 + K4/6;

    // NORMALIZA QUATERNIOS
    mod = pow(quat[3].sumPow(),0.5); //Módulo do quaternio
    //cout << quat[3].q0 << " | "<< quat[3].q1 << " | "<< quat[3].q2 << " | "<< quat[3].q3<<  endl;
    quat[3] = quat[3]/mod;

    // OBTENÇÃO DOS ÂNGULOS - VIA GIRÔMETROS
    angle[1] = quat[3];

}
int Giro::makeContinuous(){
    if(angle[1].phi - angle[0].phi >= 0)
    angleRegion = (int)-floor((angle[1].phi - angle[0].phi)/300.0);
    else
    angleRegion = (int)-ceil((angle[1].phi - angle[0].phi)/300.0);
    angle[1].phi += 360.0*angleRegion;

    return 1;
}

////////////////////////////////////////////////////////////////////////////////
// Atualiza dados do buffer circular
void Giro::refresh(){

    p[0] = p[1];         q[0] = q[1];         r[0] = r[1];
    p[1] = p[2];         q[1] = q[2];         r[1] = r[2];
    p[2] = p[3];         q[2] = q[3];         r[2] = r[3];

    p_f[0] = p_f[1];     q_f[0] = q_f[1];     r_f[0] = r_f[1];
    p_f[1] = p_f[2];     q_f[1] = q_f[2];     r_f[1] = r_f[2];
    p_f[2] = p_f[3];     q_f[2] = q_f[3];     r_f[2] = r_f[3];

    quat[0] = quat[1];
    quat[1] = quat[2];
    quat[2] = quat[3];
    
    angle[0] = angle[1];

}

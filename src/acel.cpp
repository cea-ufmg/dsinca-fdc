#include "acel.h"
#include <iostream>
#include <math.h>

using namespace std;

Acel::Acel(){

    filterState = 0;
    angleRegion = 0;

    x[0] = 0;  x[1] = 0;  x[2] = 0;
    y[0] = 0;  y[1] = 0;  y[2] = 0;
    z[0] = 0;  z[1] = 0;  z[2] = 0;

    // Dados recebidos dos acelerômetros filtrados
    x_f[0] = 0;  x_f[1] = 0;  x_f[2] = 0;
    y_f[0] = 0;  y_f[1] = 0;  y_f[2] = 0;
    z_f[0] = 0;  z_f[1] = 0;  z_f[2] = 0;

    //FILTRO PARA OS ACELERÔMETROS | 2ª ordem | Frequência de corte = 10 Hz
    aL[0] = 1.000000000000000;    aL[1] = -1.142980502539901;     aL[2] =  0.412801598096189;
    bL[0] = 0.067455273889072;     bL[1] = 0.134910547778144;      bL[2] = 0.067455273889072;

    //==========================================================================
    // FILTROS CHAVEADOS: de acordo com a dinâmica
    // (ESPAÇO DE ESTADOS)
    //==========================================================================
    // FILTRO A | Passa baixas (2.1 Hz)
    A_AL[0][0] =  0.765664545478018;    A_AL[0][1] =                  0;   A_AL[0][2] =                 0;
    A_AL[1][0] =  0.203711139575167;    A_AL[1][1] =  0.738628411912447;   A_AL[1][2] = -0.230747273140739;
    A_AL[2][0] =   0.027036133565571;    A_AL[2][1] =  0.230747273140739;   A_AL[2][2] =  0.969375685053186;

    A_BL[0] = 0.33140037792985;        A_BL[1] = 0.038234866762562;       A_BL[2] = 0.005074454773609;
    A_CL[0] = 0.00955871669064;         A_CL[1] = 0.08158148078906;       A_CL[2] = 0.696279450802505;
    A_DL    =  0.001794090690622;

    //==========================================================================
    // FILTRO B | Passa baixas (0.05 Hz)
    B_AL[0][0] =  0.993736471541615;    B_AL[0][1] =                  0;   B_AL[0][2] =                  0;
    B_AL[1][0] =  0.006243851132259;    B_AL[1][1] =  0.993716855840234;   B_AL[1][2] = -0.006263466833639;
    B_AL[2][0] =  0.00001961570138;    B_AL[2][1] =  0.006263466833639;   B_AL[2][2] =  0.999980322673874;

    B_BL[0] = 0.008857966894158;        B_BL[1] =  0.000027740790928;      B_BL[2] = 0.000000087150552;
    B_CL[0] = 0.000006935197732;        B_CL[1] =  0.002214469935902;       B_CL[2] = 0.707099824201178;
    B_DL   =  0.000000030812373;

    //==========================================================================
    //Matrizes auxiliares após a escolha do filtro (A ou B)

    A[0][0] =  0;    A[0][1] = 0;   A[0][2] =  0;
    A[1][0] =  0;    A[1][1] = 0;   A[1][2] =  0;
    A[2][0] =  0;    A[2][1] = 0;   A[2][2] =  0;

    B[0] = 0;        B[1] = 0;      B[2] = 0;
    C[0] = 0;        C[1] = 0;      C[2] = 0;
    D = 0;

}

////////////////////////////////////////////////////////////////////////////////
// Filtra os acelerômetros
void Acel::filter(string ang){
    
    //FILTRO PASSA-BAIXAS NOS ACELERÔMETROS
    x_f[2] = bL[0]*x[2] + bL[1]*x[1] + bL[2]*x[0]
            - aL[1]/aL[0]*x_f[1] - aL[2]/aL[0]*x_f[0];

    y_f[2] = bL[0]*y[2] + bL[1]*y[1] + bL[2]*y[0]
            - aL[1]/aL[0]*y_f[1] - aL[2]/aL[0]*y_f[0];

    z_f[2] = bL[0]*z[2] + bL[1]*z[1] + bL[2]*z[0]
            - aL[1]/aL[0]*z_f[1] - aL[2]/aL[0]*z_f[0];

    // NORMALIZA ACELERÔMETROS
    mod = pow(x_f[2]*x_f[2] + y_f[2]*y_f[2] + z_f[2]*z_f[2] ,0.5);
    x_f[2] = x_f[2]/mod;
    y_f[2] = y_f[2]/mod;
    z_f[2] = z_f[2]/mod;
}

double Acel::filter(string ang, Giro g){
    
    double Y=0;
    Euler AuxAngle;
	
    // ESCOLHE O FILTRO A/B(de acordo com a dinâmica dos girômetros)
    if( 1/*(pow( g.p[3]*g.p[3] + g.q[3]*g.q[3] + g.r[3]*g.r[3] , 0.5 ) > 0.045)*/){ 
        // FILTRO B ESCOLHIDO (Frequência de corte baixa - 0.05 Hz)
        for (int i=0; i<=2; i++){
            for (int j=0; j<=2; j++){
                A[i][j] = B_AL[i][j];
            }
            B[i] = B_BL[i];
            C[i] = B_CL[i];
        }
        D = B_DL;
        filterState = -1;

	//cout << " | B ";
        
   }
  else{
    // FILTRO A ESCOLHIDO (Frequência de corte maior - 2.1 Hz)
        for (int i=0; i<=2; i++){
            for (int j=0; j<=2; j++){
                A[i][j] = A_AL[i][j];
            }

            B[i] = A_BL[i];
            C[i] = A_CL[i];
        }
        D = A_DL;
        filterState = 1;

	//cout << " |  A ";
    }
    
    AuxAngle = angle[1] - g.angle[1];
    
    if (ang == "phi"){
    
        //Predição do próximo estado
        //X[k+1] = A*X[k] + B*U[k]
        phi_X0[1] = A[0][0]*phi_X0[0] + A[0][1]*phi_X1[0] + A[0][2]*phi_X2[0] + B[0]*AuxAngle.phi;
        phi_X1[1] = A[1][0]*phi_X0[0] + A[1][1]*phi_X1[0] + A[1][2]*phi_X2[0] + B[1]*AuxAngle.phi;
        phi_X2[1] = A[2][0]*phi_X0[0] + A[2][1]*phi_X1[0] + A[2][2]*phi_X2[0] + B[2]*AuxAngle.phi;
    
        //Equação de saída
        //Y[k] = C*X[k] + D
        Y = C[0]*phi_X0[0] + C[1]*phi_X1[0] + C[2]*phi_X2[0] + D;
        
    }
    else if (ang == "theta"){
        
        //Predição do próximo estado
        //X[k+1] = A*X[k] + B*U[k]
        theta_X0[1] = A[0][0]*theta_X0[0] + A[0][1]*theta_X1[0] + A[0][2]*theta_X2[0] + B[0]*AuxAngle.theta;
        theta_X1[1] = A[1][0]*theta_X0[0] + A[1][1]*theta_X1[0] + A[1][2]*theta_X2[0] + B[1]*AuxAngle.theta;
        theta_X2[1] = A[2][0]*theta_X0[0] + A[2][1]*theta_X1[0] + A[2][2]*theta_X2[0] + B[2]*AuxAngle.theta;

        //Equação de saída
        //Y[k] = C*X[k] + D
        Y = C[0]*theta_X0[0] + C[1]*theta_X1[0] + C[2]*theta_X2[0] + D;
    }
    
    //cout << Y << endl;
    return Y;
}

void Acel::getAngles(){
    
    double  aux = y_f[1]/cos(asin(x_f[1]));

    //Para não estourar asin (valor deve estar entre -1 e 1)
    if(aux > 1) aux = 1;
    else if (aux < -1) aux = -1;
    
    // OBTENÇÃO DOS ÂNGULOS - VIA ACELERÔMETROS
    if      (z[1] < 0) angle[1].phi   = -( asin( aux ) )*57.295;
    else if (z[1] > 0) angle[1].phi   = -atan2(y_f[1],-z_f[1])*57.295;

    angle[1].theta = ( asin(x_f[1]) )*57.295;
}

int Acel::makeContinuous(){
    if(angle[1].phi - angle[0].phi >= 0)
    angleRegion = (int)-floor((angle[1].phi - angle[0].phi)/300);
    else
    angleRegion = (int)-ceil((angle[1].phi - angle[0].phi)/300);
    angle[1].phi += 360*angleRegion;

    return 1;
}
////////////////////////////////////////////////////////////////////////////////
// Atualiza dados do buffer circular
void Acel::refresh(){

    x[0] = x[1];       y[0] = y[1];       z[0] = z[1];
    x[1] = x[2];       y[1] = y[2];       z[1] = z[2];

    x_f[0] = x_f[1];   y_f[0] = y_f[1];   z_f[0] = z_f[1];
    x_f[1] = x_f[2];   y_f[1] = y_f[2];   z_f[1] = z_f[2];
    
    theta_X0[0] = theta_X0[1];    phi_X0[0] = phi_X0[1];
    theta_X1[0] = theta_X1[1];    phi_X1[0] = phi_X1[1];
    theta_X2[0] = theta_X2[1];    phi_X2[0] = phi_X2[1];
    
    angle[0] = angle[1];

}

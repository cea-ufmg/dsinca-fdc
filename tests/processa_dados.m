clc;

daq = load daq_file.dat;
imu = load imu_file.dat;
gps = load gps_file.dat;

lat = gps(:,1);
long = gps(:,2);
alt = gps(:,3);
validade = gps(:,13);
t_gps = 1e-9*(gps(:,12)-gps(1,12));


figure;
plot((daq(:,17)-daq(1,17))*1e-9,daq(:,4))
xlabel('t(s)');
ylabel('canal 03 (V)');
title('pressao estatica');
grid;

figure;
plot((daq(:,17)-daq(1,17))*1e-9,daq(:,7))
xlabel('t(s)');
ylabel('canal 06 (V)');
title('pressao dinamica');
grid;

figure;
plot((daq(:,17)-daq(1,17))*1e-9,daq(:,6))
xlabel('t(s)');
ylabel('canal 05 (V)');
title('alpha');
grid;

figure;
plot((daq(:,17)-daq(1,17))*1e-9,daq(:,5))
xlabel('t(s)');
ylabel('canal 04 (V)');
title('beta');
grid;

figure;
plot((daq(:,17)-daq(1,17))*1e-9,daq(:,1))
xlabel('t(s)');
ylabel('canal 00 (V)');
title('motor');
grid;

figure;
plot((daq(:,17)-daq(1,17))*1e-9,daq(:,2))
xlabel('t(s)');
ylabel('canal 01 (V)');
title('profundor');
grid;

figure;
plot((daq(:,17)-daq(1,17))*1e-9,daq(:,3))
xlabel('t(s)');
ylabel('canal 02 (V)');
title('leme');
grid;

figure;
plot((daq(:,17)-daq(1,17))*1e-9,daq(:,16))
xlabel('t(s)');
ylabel('canal 15 (V)');
title('aileron');
grid;

figure;
plot(t_gps,validade);
axis([min(t_gps) max(t_gps) -0.1 6.2]);
xlabel('t(s)');
ylabel('adimensional');
title('GPS *VALIDADE*: 0 -> invalido; 1 -> OK; 4,5,6,7 -> Timeout');
grid;

figure;
plot(long,lat);
xlabel('longitude (graus)');
ylabel('latitude (graus)');
title('GPS *TRAJETORIA*');
grid;


figure;
plot((imu(:,8))*1e-9,imu(:,3))
xlabel('t(s)');
ylabel('(m/s²)');
title('Aceleracao em Z');
grid;

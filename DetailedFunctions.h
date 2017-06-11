#pragma once
#include <math.h>
#include "Navigation.h"
#include "Time.h"
#define PI 3.1415926535897932384626433832795
#define PI_d_2 (PI/2)
#define c 299792458.0
#define mu 3.986004415E14
#define we 7.2921151467e-5
#define R1 4.442807633e-10
#define SECPERDAY (3600*24)
#define PI2 (PI*2)
#define Deg (180.0/PI)

struct SatelliteImportantData {
	double Ek;
	double dt;
	double ddt;
	double dddt;
	double A;
	double e;
	double tgd;
	GPSTime TOC;
	GPSTime TOE;
};
void init();
bool satellite_position_calculation(EphemerisParameters * in, GPSTime & pre, SpaceLocation * out, SatelliteImportantData & data, int num, double fake_distance);
void user_position_calculation(SpaceLocation * satellites_position, double * fake_distance, double * fake_distance_err, double & To, int satellite_amount, SpaceLocation & location, uchar & flag, double * e_fd, int changed, bool * available, int banned_amount);
void first_hand_correction(double * fake_distance, IonosphericAndUtcParameters * ionopara, SpaceLocation * satellites_location, SatelliteImportantData * data, SpaceLocation * user_location, int satellite_amount, GPSTime & pre, bool * available, int & banned_amount);
void fake_distance_prediction(EphemerisParameters * in, GPSTime & pre, double * out, uchar * mask, double * e_fd, SpaceLocation & user_location, IonosphericAndUtcParameters * ionopara, double To);

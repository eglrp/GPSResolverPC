#pragma once
#include <malloc.h>
#include <math.h>
#include "MatC.h"
#include "Data.h"

#define PI 3.1415926535897932384626433832795
#define MAX_SATELLITE_NUMBER 32

const int available_flag = 199;
const int unavailable_flag = -99;


struct BLHLocation {
	double latitude;
	double longitude;
	double height;
	void getENUTrans(Matrix *& E)
	{
		if (E)free(E);
		E = malloc_mat(3, 3);
		double pos[] = { latitude,longitude,height };
		double sinp = sin(pos[0]), cosp = cos(pos[0]), sinl = sin(pos[1]), cosl = (pos[1]);
		E->data[0][0] = -sinl;       E->data[0][1] = cosl;        E->data[0][2] = 0.0;
		E->data[1][0] = -sinp*cosl;  E->data[1][1] = -sinp*sinl;  E->data[1][2] = cosp;
		E->data[2][0] = cosp*cosl;   E->data[2][1] = cosp*sinl;   E->data[2][2] = sinp;
	}
	Matrix * getENUTrans()
	{
		Matrix * E = malloc_mat(3, 3);
		double pos[] = { latitude,longitude,height };
		double sinp = sin(pos[0]), cosp = cos(pos[0]), sinl = sin(pos[1]), cosl = (pos[1]);
		E->data[0][0] = -sinl;       E->data[0][1] = cosl;        E->data[0][2] = 0.0;
		E->data[1][0] = -sinp*cosl;  E->data[1][1] = -sinp*sinl;  E->data[1][2] = cosp;
		E->data[2][0] = cosp*cosl;   E->data[2][1] = cosp*sinl;   E->data[2][2] = sinp;
		return E;
	}
};

struct SpaceLocation {
	double X;
	double Y;
	double Z;
	double operator-(SpaceLocation obj)
	{
		return sqrt(pow(X - obj.X, 2) + pow(Y - obj.Y, 2) + pow(Z - obj.Z, 2));
	}
	BLHLocation toBLH() const
	{
		const static double a = 6378137.0;
		const static double F = 1.0 / 298.257223563;
		const SpaceLocation * xyz = this;
		BLHLocation blh;
		double e2, Z, dZ, ZdZ, r, sinb, N, x2y2;
		int iter;
		iter = 0;
		r = 0.0;
		N = 0.0;
		sinb = 0.0;
		e2 = 2 * F - F * F;
		x2y2 = xyz->X * xyz->X + xyz->Y * xyz->Y;
		dZ = e2 * xyz->Z;
		do
		{
			Z = dZ;
			ZdZ = Z + xyz->Z;
			r = x2y2 + ZdZ*ZdZ;
			sinb = ZdZ / sqrt(r);
			N = a / sqrt(1 - e2*sinb*sinb);
			dZ = N * e2 * sinb;
			iter = iter + 1;
		} while ((iter <= 10) && (fabs(dZ - Z) > 1E-8));
		blh.longitude = atan2(xyz->Y, xyz->X);
		blh.latitude = atan2(ZdZ, sqrt(x2y2));
		blh.height = sqrt(x2y2 + ZdZ*ZdZ) - N;
		return blh;
	}
};
struct Node {
	SpaceLocation data;
	Node * next;
};
struct History {
	Node * first;
	Node * tail;
	int amount;
};
struct NavigationDataModel {
	FileHeader last_file;

	EphemerisParameters ephemeris[MAX_SATELLITE_NUMBER];
	uchar ephemeris_available[MAX_SATELLITE_NUMBER];
	int ephemeris_available_amount;

	IonosphericAndUtcParameters ionospheric;
	uchar ionospheric_available;

	SatelliteInfomation observation[MAX_SATELLITE_NUMBER];
	uchar observation_available[MAX_SATELLITE_NUMBER];
	int observation_available_amount;

	PDPFilterPositionParameters position;
	uchar position_available;

	SpaceLocation satellite_locations[MAX_SATELLITE_NUMBER];
	uchar solve_result_available;
	SpaceLocation user_location;
	double To;

	FILE * result_file;

	SatelliteInfomation last_observation[MAX_SATELLITE_NUMBER];
	uchar last_observation_available[MAX_SATELLITE_NUMBER];
	int last_observation_amount;

	History * history;

	double predict_fakedistance[MAX_SATELLITE_NUMBER];
	uchar predict_available;
};



//NavigationDataModel * navigation_data_model_shared_instance();
void navigation_data_model_update_ephdata(MessageEphemeris * data);
void navigation_data_model_update_ionutc(MessageIonosphericAndUtc * data);
void navigation_data_model_update_posi(MessagePDPFilterPosition * data);
void navigation_data_model_update_observation(MessageSatelliteObservation * data);
void navigation_model_calculation_process();
void navigation_data_model_print();
void navigation_model_reset();
void navigation_data_model_self_check();
void navigation_data_model_resulting();
#include "DetailedFunctions.h"
#include "Time.h"
#include "MatC.h"



double get_atan(double z, double y)
{
	double x = 0;
	if (z == 0)x = PI / 2;
	else if (y == 0)x = PI;
	else {
		x = atan(fabs(y / z));
		if ((y > 0) && (z < 0))x = PI - x;
		else if ((y < 0) && (z < 0))x = PI + x;
		else if ((y < 0) && (z > 0))x = 2 * PI - x;
	}
	return x;
}
FILE * fp1, *fp2, *fp3, *fp4;
double b1[MAX_SATELLITE_NUMBER];
double b2[MAX_SATELLITE_NUMBER];
double b3[MAX_SATELLITE_NUMBER];
double b4[MAX_SATELLITE_NUMBER];
void init()
{
	fp1 = fopen("f1.txt", "w");
	fp2 = fopen("f2.txt", "w");
	fp3 = fopen("f3.txt", "w");
	fp4 = fopen("f4.txt", "w");
}
bool satellite_position_calculation(EphemerisParameters * in, GPSTime & pre, SpaceLocation * out, SatelliteImportantData & data, int num, double fake_distance)
{
	//平均角速度n
	double n0 = sqrt(mu) / pow(in->semi_major_axis, 1.5);
	double n = n0 + in->mean_motion_difference;

	//归化时间tk
	//GPSTime pre_gps = GPSTime(pre);
	double t = pre.sec - fake_distance / c;
	double tk = t - in->reference_time_for_ephemeris;
	if (fabs(tk)>7200)
	{
		return false;
	}
	if (tk > 302400)
		tk -= 604800;
	else if (tk < -302400)
		tk += 604800;

	//平近点角Mk
	double Mk = in->mean_anomaly_of_reference_time + n * tk;

	//偏近点角Ek
	double Ek = Mk;
	double Ek2 = 0;
	while (1)
	{
		Ek2 = Mk + in->eccentricity * sin(Ek);
		if (fabs(Ek - Ek2) <= 1.0e-12)break;
		Ek = Ek2;
	}
	data.Ek = Ek;
	//真近点角fk
	double sqt_1_e2 = sqrt(1 - pow(in->eccentricity, 2));
	double cosfk = (cos(Ek) - in->eccentricity) / (1 - in->eccentricity * cos(Ek));
	double sinfk = (sqt_1_e2 * sin(Ek)) / (1 - in->eccentricity * cos(Ek));
	double fk = get_atan((cos(Ek) - in->eccentricity), sqt_1_e2 * sin(Ek));
	//升交距角faik
	double faik = fk + in->argument_of_perigee;

	//摄动改正项su、sr、si
	double cos_2_faik = cos(2 * faik);
	double sin_2_faik = sin(2 * faik);
	double su = in->argument_of_latitude_in_cosine * cos_2_faik + in->argument_of_latitude_in_sine * sin_2_faik;
	double sr = in->orbit_radius_in_cosine * cos_2_faik + in->orbit_radius_in_sine * sin_2_faik;
	double si = in->inclination_in_cosine * cos_2_faik + in->inclination_in_sine * sin_2_faik;

	//升交距角uk、卫星矢径rk、轨道倾角ik
	double uk = faik + su;
	double rk = in->semi_major_axis * (1 - in->eccentricity * cos(Ek)) + sr;
	double ik = in->inclination_angle_of_reference_time + si + in->rate_of_inclination_angle * tk;

	//卫星在轨道平面上的位置
	double xk = rk * cos(uk);
	double yk = rk * sin(uk);
	double zk = 0;

	//计算观测时刻的升交点经度L
	double L = in->right_ascension + (in->rate_of_right_ascention - we) * tk - we * in->reference_time_for_ephemeris;
	//double L = in->right_ascension + (in->rate_of_right_ascention - we) * (tk + in->reference_time_for_ephemeris);

	//计算卫星在WGS84下的位置
	out->X = xk * cos(L) - yk * cos(ik) * sin(L);
	out->Y = xk * sin(L) + yk * cos(ik) * cos(L);
	out->Z = yk * sin(ik);

	b1[num] = out->X;
	b2[num] = out->Y;
	b3[num] = out->Z;

	return true;
}
double vs[MAX_SATELLITE_NUMBER] = { 0 };


void user_position_calculation(SpaceLocation * satellites_position, double * fake_distance, double * fake_distance_err, double & To, int satellite_amount, SpaceLocation & location, uchar & flag, double * e_fd, int changed, bool * available, int banned_amount)
{
	if (satellite_amount - banned_amount < 4)
	{
		//fprintf(fp1, "%lf,%lf,%lf,%lf,%d,1\n", location.X, location.Y, location.Z, To, changed);
		flag = available_flag;
		return;
	}
	BLHLocation blh = location.toBLH();
	fprintf(fp1, "%3.20lf,%3.20lf,%3.20lf,%lf,%d,0\n", blh.latitude * 180.0 / PI, blh.longitude * 180.0 / PI, blh.height, To, changed);
	//fprintf(fp1, "%lf,%lf,%lf,%lf\n", location.X, location.Y, location.Z,To);
	
	//for (int i = 0;i < MAX_SATELLITE_NUMBER;i++)
	//{
	//	fprintf(fp2, "%lf,", e_fd[i]);
	//}
	//fprintf(fp2, "\n");
	SpaceLocation temp = location;
	Matrix * Z = malloc_mat(satellite_amount - banned_amount, 1);
	Matrix * H = malloc_mat(satellite_amount - banned_amount, 4);
	Matrix * D = malloc_mat(satellite_amount - banned_amount, satellite_amount - banned_amount);
	Matrix * X = NULL, * Sig = NULL, * V = NULL;
	while (true)
	{
		double X0[4];
		X0[0] = location.X;
		X0[1] = location.Y;
		X0[2] = location.Z;
		X0[3] = To;

		//Mat Z = Mat::zeros(sat_num, 1, CV_64F);
		//Mat H = Mat::zeros(sat_num, 4, CV_64F);
		//设置方差阵为值为4的对角阵
		//Mat D = Mat::eye(sat_num, sat_num, CV_64F) * 4;
		double S[MAX_SATELLITE_NUMBER];
		double DX0[MAX_SATELLITE_NUMBER];
		double DY0[MAX_SATELLITE_NUMBER];
		double DZ0[MAX_SATELLITE_NUMBER];
		int i = 0;
		for (int j = 0; j < satellite_amount; j++)
		{
			if (available[j])
			{
				S[i] = sqrt(pow(location.X - satellites_position[i].X, 2) + pow(location.Y - satellites_position[i].Y, 2) + pow(location.Z - satellites_position[i].Z, 2));
				DX0[i] = satellites_position[i].X - location.X;
				DY0[i] = satellites_position[i].Y - location.Y;
				DZ0[i] = satellites_position[i].Z - location.Z;
				i++;
			}
		}
		i = 0;
		for (int j = 0; j < satellite_amount; j++)
		{
			if (available[j]) {
				D->data[i][i] = fake_distance_err[i];
				//设置观测矩阵Z,Zi = Pi - P0i,P0i = Dis(X0,Si)+To
				Z->data[i][0] = fake_distance[i] - S[i] - X0[3];
				//Z.at<double>(i, 0) = 
				//设置系数矩阵
				H->data[i][0] = -DX0[i] / S[i];
				H->data[i][1] = -DY0[i] / S[i];
				H->data[i][2] = -DZ0[i] / S[i];
				H->data[i][3] = 1;
				i++;
			}
		}
		//最小二乘
		//mat_save(Z, "Z.txt");
		//mat_save(D, "D.txt");
		LMS(Z, H, D, X, Sig, V);
		location.X += X->data[0][0];//tot.at<double>(0, 0);
		location.Y += X->data[1][0];
		location.Z += X->data[2][0];
		To += X->data[3][0];


		
		if (sqrt(pow(location.X - temp.X, 2) + pow(location.Y - temp.Y, 2) + pow(location.Z - temp.Z, 2)) < 1e-3) {
			memset(vs, 0, sizeof(double) * MAX_SATELLITE_NUMBER);
			double ds[MAX_SATELLITE_NUMBER] = { 0 };
			double fds[MAX_SATELLITE_NUMBER] = { 0 };
			int j = 0, k = 0;
			for (int i = 0;i < MAX_SATELLITE_NUMBER;i++)
			{
				if (e_fd[i] != 0) {
					if (available[j]) {
						vs[i] = V->data[k][0];
						ds[i] = D->data[k][k];
						fds[i] = fake_distance[j];
						k++;
					}
					j++;
				}
			}
			for (int i = 0;i < MAX_SATELLITE_NUMBER;i++)
			{
				fprintf(fp3, "%lf,", vs[i]);
				fprintf(fp4, "%lf,", ds[i]);
				fprintf(fp2, "%lf,",fds[i]);
			}
			fprintf(fp3, "\n");
			fprintf(fp4, "\n");
			fprintf(fp2, "\n");
			free_mat(X);
			free_mat(Sig);
			free_mat(V);
			break;
		}
		free_mat(X);
		free_mat(Sig);
		free_mat(V);
		temp = location;
	}

	free_mat(Z);
	free_mat(H);
	free_mat(D);
	flag = available_flag;
}
double Hopfield(double hgt, double elev)
{
	double t0, p0, e0, h0;
	double t, p, e;
	double dend, elev2, denw, hw, hd, rkd, rkw;
	double trop;

	if (fabs(hgt)>30000.0)   return 0.0;

	t0 = 20 + 273.16;
	p0 = 1013.0;
	e0 = 0.5*exp(-37.2465 + 0.213166*t0 - 0.000256908*t0*t0);
	h0 = 0;
	hw = 11000.0;
	t = t0 - 0.0068*(hgt - h0);
	p = p0*pow(1.0 - 0.0068 / t0*(hgt - h0), 5);
	e = e0*pow((1 - 0.0068 / t0*(hgt - h0)), 2.0)*pow((1.0 - (hgt - h0) / hw), 4.0);
	elev2 = elev*elev * Deg * Deg;
	dend = sqrt(elev2 + 6.25) / Deg;
	denw = sqrt(elev2 + 2.25) / Deg;

	hd = 148.72*t0 - 488.3552;
	rkd = 1.552e-5*p / t*(hd - hgt);
	rkw = 7.46512e-2*(e / t / t)*(hw - hgt);
	trop = (rkd / sin(dend)) + (rkw / sin(denw));
	return trop;
}
double Klobutchar(GPSTime * gpstime, const double Elev, SpaceLocation * RCVPos, const double A, IonosphericAndUtcParameters * ionopara) {
	long double I, sai, phyi, phyu, phym, namdai, namdau;
	long double t, Ai, Pi, Xi, X, f, temp;
	int i, j;
	BLHLocation blh = RCVPos->toBLH();
	phyu = blh.latitude;
	namdau = blh.longitude;
	sai = 0.0137 / (Elev / PI + 0.11) - 0.022;//半周
	phyi = phyu / PI + sai * cos(A);//半周
	if (phyi > 0.416) phyi = 0.416;
	if (phyi < -0.416) phyi = -0.416;
	namdai = namdau / PI + sai * sin(A) / cos(phyi * PI);
	phym = phyi + 0.064 * cos((namdai - 1.617) * PI);
	t = 43200.0 * namdai + gpstime->sec;
	if (t >= 86400) t = fmod(t, 86400.0);
	else if (t < 0) t = t + 86400;

	Ai = ionopara->alpha_parameter[0] + ionopara->alpha_parameter[1] * phym + ionopara->alpha_parameter[2] * phym * phym + ionopara->alpha_parameter[3] * phym * phym * phym;
	if (Ai < 0) Ai = 0.0;
	Pi = ionopara->beta_parameter[0] + ionopara->beta_parameter[1] * phym + ionopara->beta_parameter[2] * phym * phym + ionopara->beta_parameter[3] * phym * phym * phym;
	if (Pi < 72000) Pi = 72000;

	Xi = 2 * PI * (t - 50400) / Pi;
	f = 1.0 + 16.0 * (0.53 - Elev / PI) * (0.53 - Elev / PI) * (0.53 - Elev / PI);
	X = 1.0 - Xi * Xi / 2.0 + Xi * Xi * Xi * Xi / 24.0;
	if (fabs(Xi) <= 1.57) {
		I = (5.0e-9 + Ai * X) * f;
	}
	else
		I = 5.0e-9 * f;
	return I;
}

double Klobuchar(GPSTime* gpstime, double Elev, SpaceLocation * rcvPos, IonosphericAndUtcParameters * ionopara)
{
	double H0 = 420000.0;
	double dH = 100000.0;

	BLHLocation rcv = rcvPos->toBLH();
	double phi_i = rcv.latitude / PI;
	double lambda_i = rcv.longitude / PI;
	double Tmp = exp(1.0 - exp((-rcv.height + H0) / dH));
	double alpha = (2.71828183 - Tmp) / (2.71828183 - exp(1.0 - exp(H0 / dH)));
	double sinE = sin(Elev);
	double iF = 2.037 / (sqrt(sinE*sinE + 0.076) + sinE);
	if (phi_i > 0.416)
	{
		phi_i = 0.416;
	}
	if (phi_i < -0.416)
	{
		phi_i = -0.416;
	}

	double phi_m = phi_i + 0.064 * cos((lambda_i - 1.617)*PI);

	double iAMP = 0.0;
	double iPER = 0.0;
	for (int n = 0; n < 4; n++)
	{
		iAMP += ionopara->alpha_parameter[n] * pow(phi_m, n);
		iPER += ionopara->beta_parameter[n] * pow(phi_m, n);
	}
	if (iAMP < 0.0)
	{
		iAMP = 0.0;
	}
	if (iPER < 72000.0)
	{
		iPER = 72000.0;
	}

	double t = 43200.0 * lambda_i + fmod(gpstime->sec, SECPERDAY);
	if (t >= 86400.0)
	{
		t = t - 86400.0;
	}
	if (t < 0)
	{
		t = t + 86400.0;
	}

	double x = PI2 * (t - 50400.0) / iPER; // x is in radians

	double t_iono = 0.0;
	if (fabs(x) < 1.57)
	{
		t_iono = iF * (5.0e-9 + iAMP * (1 - pow(x, 2) / 2 + pow(x, 4) / 24));
	}
	else
	{
		t_iono = iF * 5.0e-9;
	}

	double ion = t_iono * c * alpha;

	return (ion);
}
double xd[MAX_SATELLITE_NUMBER];
double yd[MAX_SATELLITE_NUMBER];
double zd[MAX_SATELLITE_NUMBER];
double ad[MAX_SATELLITE_NUMBER];
double bd[MAX_SATELLITE_NUMBER];
void elevation_and_azimuth(SpaceLocation * satellite, SpaceLocation * user_location,double out[2])
{
	double dpos[3] = {0};
	SpaceLocation ori{ 0,0,0 };
	dpos[0] = satellite->X - user_location->X;
	dpos[1] = satellite->Y - user_location->Y;
	dpos[2] = satellite->Z - user_location->Z;

	double user_distance_to_earth = *user_location - ori;
	double mod = sqrt(dpos[0] * dpos[0] + dpos[1] * dpos[1] + dpos[2] * dpos[2]);
	if (fabs(user_distance_to_earth * mod < 1.0)) {
		out[0] = PI_d_2;
	}
	else{
		double m = dpos[0] * user_location->X + dpos[1] * user_location->Y + dpos[2] * user_location->Z;
		double n = m / (mod * user_distance_to_earth);
		out[0] = PI_d_2 - acos(n);
	}

	BLHLocation blh = user_location->toBLH();
	double B = blh.latitude;
	double L = blh.longitude;
	double N = -sin(B) * cos(L) * dpos[0] - sin(B) * sin(L) * dpos[1] + cos(B) * dpos[2];
	double E = -sin(L) * dpos[0] + cos(L)* dpos[1];
	out[1] = atan2(E, N);
}

double elevation(double ** enu)
{
	double l1 = sqrt(enu[0][0] * enu[0][0] + enu[1][0] * enu[1][0] + enu[2][0] * enu[2][0]);
	double l2 = sqrt(enu[0][0] * enu[0][0] + enu[1][0] * enu[1][0]);
	double tot = acos(l2 / l1);
	if (enu[2][0] < 0)tot = -tot;
	if (tot > PI_d_2)tot = PI - tot;
	return tot;
}
void first_hand_correction(double * fake_distance, IonosphericAndUtcParameters * ionopara,SpaceLocation * satellites_location, SatelliteImportantData * data,SpaceLocation * user_location, int satellite_amount, GPSTime & pre,bool * available, int & banned_amount)
{
	banned_amount = 0;
	if (satellite_amount <= 0)return;
	//Matrix * elev = user_location ? user_location->toBLH().getENUTrans() : NULL;
	//Matrix * dpos = malloc_mat(3, 1);
	//Matrix * e = NULL;
	
	for (int i = 0;i < satellite_amount;i++)
	{
		available[i] = true;
		double dtc = pre - data[i].TOC - fake_distance[i] / c;
		double s = (data[i].dt + data[i].ddt * dtc + data[i].dddt * dtc);
		double r = (R1 * data[i].e * sin(data[i].Ek) * sqrt(data[i].A));
		fake_distance[i] += s * c;//星钟差
		fake_distance[i] -= r * c; //相对论
		fake_distance[i] -= data[i].tgd * c;//群延迟
		
		//地球自转改正
		double dA = we * (fake_distance[i] / c - s + r);
		satellites_location[i].X += satellites_location[i].Y * dA;
		satellites_location[i].Y -= satellites_location[i].X * dA;
		double height = user_location ? user_location->toBLH().height : 0;
		if (user_location && ionopara)
		{
			double ele_and_azi[2];
			elevation_and_azimuth(satellites_location + i, user_location, ele_and_azi);
			if (ele_and_azi[0] * 180.0 / PI < 10)
			{
				banned_amount++;
				available[i] = false;
			}
			fake_distance[i] -= Klobutchar(&pre, ele_and_azi[0], user_location, ele_and_azi[1], ionopara) * c;//电离层
			fake_distance[i] -= Hopfield(height, ele_and_azi[0]);//对流层
		}
	}
}
void fake_distance_prediction(EphemerisParameters * in, GPSTime & pre, double * out,uchar * mask,double * e_fd,SpaceLocation & user_location,IonosphericAndUtcParameters * ionopara,double To)
{
	double height = user_location.toBLH().height;

	GPSTime predict_time = pre;
	predict_time.sec++;
	SatelliteImportantData data[MAX_SATELLITE_NUMBER];
	SpaceLocation satellites_locations[MAX_SATELLITE_NUMBER];
	for (int i = 0;i < MAX_SATELLITE_NUMBER;i++)
	{
		if (mask[i] == available_flag)
		{
			satellite_position_calculation(in + i, predict_time, satellites_locations + i, data[i], i + 1, e_fd[i]);

			out[i] = satellites_locations[i] - user_location;

			data[i].A = in[i].semi_major_axis;
			data[i].dt = in[i].clock_aging_parameter_sec;
			data[i].ddt = in[i].clock_aging_parameter_1;
			data[i].dddt = in[i].clock_aging_parameter_isec;
			data[i].e = in[i].eccentricity;
			data[i].TOC = GPSTime(in[i].GPSweek, in[i].clock_correction_term);
			data[i].TOE = GPSTime(in[i].GPSweek, in[i].reference_time_for_ephemeris);
			data[i].tgd = in[i].group_delay_difference;


			
			double dtc = predict_time - data[i].TOC - out[i] / c;
			double s = (data[i].dt + data[i].ddt * dtc + data[i].dddt * dtc);
			double r = (R1 * data[i].e * sin(data[i].Ek) * sqrt(data[i].A));
			out[i] -= s * c;//星钟差
			out[i] += r * c; //相对论
			out[i] += data[i].tgd * c;//群延迟

			
			if (ionopara)
			{
				double ele_and_azi[2];
				elevation_and_azimuth(satellites_locations + i, &user_location, ele_and_azi);
				out[i] += Klobutchar(&predict_time, ele_and_azi[0], &user_location, ele_and_azi[1], ionopara) * c;
				out[i] += Hopfield(height, ele_and_azi[0]);
			}

			out[i] += To;
			out[i] += vs[i];
		}
	}
}
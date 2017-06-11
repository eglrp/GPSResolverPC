#pragma once
#pragma warning(disable : 4996)

#include <malloc.h>
#include <stdio.h>
#include <string.h>


#define buffer_size 100
#define uchar unsigned char
#define ushort unsigned short
#define ulong unsigned long
#define uint unsigned int

const uchar head_flag[3] = {0xAA,0x44,0x12};//3 Sync bytes 

const short id_ephem = 7;
const short id_posi = 469;
const short id_sat_range = 43;
const short id_pseudorange = 47;
const short id_ionutc = 8;

const short crc_size = 4;
enum TimeStatus {
	unknown = 20,
	approximately = 60,
	coarse_adjusting = 80,
	coarse = 100,
	coarse_steering = 120,
	fire_wheeling = 130,
	fine = 160,
	fine_backup_steering = 170,
	fine_steering = 180,
	sat_time = 200,
};

struct PDPFilterPositionParameters {
	ulong solution_status;
	ulong position_type;
	double latitude;
	double longitude;
	double height;
	float undulation;
	ulong datum_id;
	float latitude_deviation;
	float longitude_deviation;
	float height_deviation;
	char base_station[4];
	float differential_age;
	float solution_age;
	uchar satellite_num;
	uchar satellite_num_in_the_solution;
	uchar reserved[6];
};
struct IonosphericAndUtcParameters
{
	double alpha_parameter[4];//a0 to a3
	double beta_parameter[4]; //b0 to b3
	ulong utc_reference_week_number;//utc_wn
	ulong reference_time_of_utc_parameters;//tot
	double utc_constant_term;//A0
	double utc_first_order_term;//A1
	ulong future_week_number;//ulong
	ulong day_number;//dn
	long delta_time_leap_seconds;//deltat ls
	long future_delta_time_leap_seconds;//deltat lsf
	long time_difference;//deltat utc


};

struct SatelliteInfomation
{
	ushort satellite_number;
	ushort glonass_frequency;
	double pseudorange;
	float pseudorange_deviation;//std
	double carrier_phase;//in cycles
	float carrier_phase_deviation;
	float doppler;//Hz
	float carrier_nosie_density_ratio;//C/No
	float lock_time;//sec
	uchar tracking_status[4];
};

//struct  PseudorangePositionParameters {
//
//};
struct EphemerisParameters
{
	ulong satellite_number;//prn
	double time_of_subframe_zero;//tow
	ulong health_status;
	ulong issue_of_ephemeris_1;//IODE1
	ulong issue_of_ephemeris_2;//IODE2
	ulong GPSweek;
	ulong Zweek;
	double reference_time_for_ephemeris;//toe
	double semi_major_axis;//A
	double mean_motion_difference;//dN
	double mean_anomaly_of_reference_time;//M0
	double eccentricity;//ecc
	double argument_of_perigee;//omega
	double argument_of_latitude_in_cosine;//cuc
	double argument_of_latitude_in_sine;//cus
	double orbit_radius_in_cosine;//crc
	double orbit_radius_in_sine;//crs
	double inclination_in_cosine;//cic
	double inclination_in_sine;//cis
	double inclination_angle_of_reference_time;//I0
	double rate_of_inclination_angle;//dI
	double right_ascension;//omega_0
	double rate_of_right_ascention;//d(omega)
	ulong issue_of_data_clock;//iodc
	double clock_correction_term;//toc
	double group_delay_difference;//tgd
	double clock_aging_parameter_sec;//af1
	double clock_aging_parameter_1;//af2
	double clock_aging_parameter_isec;//af3
	int anti_spoofing;//AS 0 = false
	double corrected_mean_motion;//N
	double user_range_accuracy_variance;//URA
};
struct FileHeader
{
	uchar sync_flag[3];
	uchar header_length;
	ushort message_id;
	char message_type;
	uchar port_address;
	ushort message_length;
	ushort sequence;
	uchar idle_time;
	uchar  time_status;
	ushort week;
	long GPS_sec_milisecond;
	ulong receiver_status;
	ushort reserved;
	ushort receiver_sw_version;
};
struct MessageEphemeris 
{
	FileHeader header;
	EphemerisParameters parameters;
};
struct MessageIonosphericAndUtc
{
	FileHeader header;
	IonosphericAndUtcParameters parameters;
};
struct MessagePDPFilterPosition 
{
	FileHeader header;
	PDPFilterPositionParameters parameters;
};
struct MessageSatelliteObservation {
	FileHeader header;
	long observation_number;
	SatelliteInfomation * infomation;
};

uchar get_uchar(void * pointee);
char get_char(void * pointee);
short get_short(void * pointee);
ushort get_ushort(void * pointee);
long get_long(void * pointee);
ulong get_ulong(void * pointee);
double get_double(void * pointee);
float get_float(void * pointee);
int get_int(void * pointee);
void read_file_for_another_frame(FILE * fp, int & message_id);

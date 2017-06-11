#pragma once
#include "Data.h"
enum TrackingState {
	idle = 0,
	sky_search = 1,
	wide_frequency_band_pullin = 2,
	narrow_frequency_band_pullin = 3,
	phase_lock_loop = 4,
	reacquisition = 5,
	steering = 6,
	frequency_lock_loop = 7,
};
enum  CorrelatorType {
	not_a_number = 0,
	standard_correlator = 1,//spacing = 1 chip
	narrow_correlator = 2,//spacing < 1 chip
	pulse_aperture_correlator = 4,//PAC
};
enum SatelliteSystem {
	GPS = 0,
	GLONASS = 1,
	WAAS = 2,
	Other = 7,
};
enum SignalType {
	L1_C_A = 0,
};

//second hand infomation
struct ChannelTrackingStatus {
	TrackingState tracking_state;
	uint channel_number;
	bool phase_lock_flag;
	bool parity_flag;
	bool code_locked_flag;
	CorrelatorType correlator_spacing;
	uchar satellite_system;
	bool grouping;
	uint signal_type;
	bool forward_error_correction;
	bool primary;
	bool carrier_phase_measurement_half_cycle_added;
	bool prn_lock_flag;
	bool channel_assignment;// 0 = Automatic , 1 = Forced
};

void get_tracking_status(uchar in[4], ChannelTrackingStatus & out)
{
	//printf("%2x %2x %2x %2x", in[0], in[1], in[2], in[3]);
	out.tracking_state = (TrackingState) (in[0] & 0x1F);
	out.channel_number = (in[1] & 0x03 << 3) & (in[0] & 0xE0 >> 5);
	out.phase_lock_flag = (in[1] & 0x04) >> 2;
	out.parity_flag = (in[1] & 0x08) >> 3;
	out.code_locked_flag = (in[1] & 0x10) >> 4;
	out.correlator_spacing = (CorrelatorType)(in[1] & 0xE0 >> 5);
	out.satellite_system = (SatelliteSystem)(in[2] & 0x03);
	out.grouping = (in[2] & 0x10) >> 4;
	out.signal_type = (in[3] & 0x03 << 3) & (in[2] & 0xE0 >> 5);
	out.forward_error_correction = (in[3] & 0x04) >> 2;
	out.primary = in[3] & 0x08 >> 3;
	out.carrier_phase_measurement_half_cycle_added = in[3] & 0x10 >> 4;
	out.prn_lock_flag = in[3] & 0x40 >> 6;
	out.channel_assignment = in[3] & 0x80 >> 7;
	//(TrackingState)(in[0] & 0x1F);
	//out.channel_number = in & 0x3E0 >> 5;
	//out.phase_lock_flag = in & 0x400 >> 10;
	//out.parity_flag = in & 0x800 >> 11;
	//out.code_locked_flag = in & 0x1000 >> 12;
	//out.correlator_spacing = (CorrelatorType)(in & 0xE000 >> 13);
	//out.satellite_system = (SatelliteSystem)(in & 0x70000 >> 16);
	//out.grouping = in & 0x100000 >> 20;
	//out.signal_type = (SignalType)(in & 0x3E00000 >> 21);
	//out.forward_error_correction = in & 0x4000000 >> 26;
	//out.primary = in & 0x8000000 >> 27;
	//out.carrier_phase_measurement_half_cycle_added = in & 0x10000000 >> 28;
	//out.prn_lock_flag = in & 0x40000000 >> 30;
	//out.channel_assignment = in & 0x80000000 >> 31;
	//
}

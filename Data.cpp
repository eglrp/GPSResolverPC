#include "Data.h"
#include "Navigation.h"
#include "CRC.h"
uchar get_uchar(void * pointee)
{
	uchar tot = 0;
	memcpy(&tot, pointee, 1);
	return tot;
}


char get_char(void * pointee)
{
	char tot = 0;
	memcpy(&tot, pointee, 1);
	return tot;
}
short get_short(void * pointee)
{
	short tot = 0;
	memcpy(&tot, pointee, 2);
	return tot;
}
ushort get_ushort(void * pointee)
{
	ushort tot = 0;
	memcpy(&tot, pointee, 2);
	return tot;
}
long get_long(void * pointee)//GPSec
{
	long tot = 0;
	memcpy(&tot, pointee, 4);
	return tot;
}
ulong get_ulong(void * pointee)
{
	ulong tot = 0;
	memcpy(&tot, pointee, 4);
	return tot;
}



double get_double(void * pointee)
{
	double tot = 0;
	memcpy(&tot, pointee, 8);
	return tot;
}
float get_float(void * pointee)
{
	float tot = 0;
	memcpy(&tot, pointee, 4);
	return tot;
}
int get_int(void * pointee)
{
	int tot = 0;
	memcpy(&tot, pointee, 4);
	return tot;
}


void read_file_for_another_frame(FILE * fp, int & message_id) 
{

	void * temp_buffer = alloca(buffer_size);
	bool flag_data_in_position = false;
	int message_length = 0;
	while (!flag_data_in_position)
	{
		fread(temp_buffer, buffer_size, 1, fp);
		for (int i = 0; i<buffer_size - 3;i++)
		{
			if (memcmp(((uchar*)temp_buffer) + i, head_flag, 3) == 0)
			{
				message_length = get_ushort((uchar*)temp_buffer + i + 8);
				message_id = get_ushort((uchar*)temp_buffer + i + 4);
				fseek(fp, i - buffer_size, SEEK_CUR);
				flag_data_in_position = true;
				break;
			}
		}
	}
	int frame_size = 28 + message_length;

	void * data = alloca(frame_size);
	void * crc = alloca(crc_size);
	fread(data, frame_size, 1, fp);
	fread(crc, crc_size, 1, fp);
	switch (message_id)
	{
	case id_ephem:
		navigation_data_model_update_ephdata((MessageEphemeris*)data);
		break;
	case id_ionutc:
		navigation_data_model_update_ionutc((MessageIonosphericAndUtc*)data);
		break;
	case id_posi:
		navigation_data_model_update_posi((MessagePDPFilterPosition*)data);
		break;
	case id_pseudorange:

		break;
	case id_sat_range:
		//static bool isFirst = true;
		MessageSatelliteObservation * body = (MessageSatelliteObservation*)alloca(sizeof(MessageSatelliteObservation));
		memcpy(body, data, sizeof(FileHeader) + 4);
		int infomation_size = body->observation_number * sizeof(SatelliteInfomation);
		body->infomation = (SatelliteInfomation*)alloca(infomation_size);
		memcpy(body->infomation, (uchar*)data + sizeof(FileHeader) + 4, infomation_size);

		//if (isFirst)
		{
			navigation_data_model_update_observation(body);
			//isFirst = false;
		}
		break;
	}
	ulong crc_file = get_ulong(crc);
	ulong crc_calc = CalculateBlockCRC32(frame_size, (uchar*)data);
	if (crc_file != crc_calc)
	{
		throw "crc check failed in frame.";
	}
	//fseek(fp,header->message_length + 4,SEEK_CUR);

}
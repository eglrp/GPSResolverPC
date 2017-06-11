#include "Navigation.h"
#include "DetailedFunctions.h"
#include "ChannelStatus.h"
#include "Time.h"

void add_history(History * history, SpaceLocation & user)
{
	Node * node = (Node*)malloc(sizeof(Node));
	node->data = user;
	node->next = NULL;
	
	if (history->amount == 0) {
		history->first = node;
		history->tail = node;
	}
	else {
		history->tail->next = node;
		history->tail = node;
	}
	history->amount++;
}
NavigationDataModel * shared_instance()
{
	static NavigationDataModel * model = NULL;
	while (model == NULL)
	{
		model = (NavigationDataModel*)calloc(sizeof(NavigationDataModel),1);
		for (int i = 0;i < MAX_SATELLITE_NUMBER;i++)
		{
			model->ephemeris_available[i] = unavailable_flag;
			model->observation_available[i] = unavailable_flag;
		}
		model->position_available = unavailable_flag;
		model->ionospheric_available = unavailable_flag;
		model->solve_result_available = unavailable_flag;

		model->last_observation_amount = 0;
		model->ephemeris_available_amount = 0;
		model->last_observation_amount = 0;

		model->history = (History*)malloc(sizeof(History));
		model->history->amount = 0;
		model->history->first = NULL;
		model->history->tail = NULL;

		model->solve_result_available = unavailable_flag;
		//model->user_location = { -2268150.8553497856,5010325.1286611324,3219232.1319357492 };
		model->predict_available = unavailable_flag;

		model->result_file = fopen("result_locations.txt", "w");
	}
	return model;
}
void navigation_data_model_update_ephdata(MessageEphemeris * data)
{
	NavigationDataModel * model = shared_instance();
	int satellite_index = data->parameters.satellite_number - 1;
	if (model->ephemeris_available[satellite_index] != available_flag)model->ephemeris_available_amount++;
	model->ephemeris_available[satellite_index] = available_flag;
	model->ephemeris[satellite_index] = data->parameters;
	model->last_file = data->header;
}
void navigation_data_model_update_ionutc(MessageIonosphericAndUtc * data)
{
	NavigationDataModel * model = shared_instance();
	model->ionospheric_available = available_flag;
	model->ionospheric = data->parameters;
	model->last_file = data->header;
}
void navigation_data_model_update_posi(MessagePDPFilterPosition * data)
{
	NavigationDataModel * model = shared_instance();
	model->position_available = available_flag;
	model->position = data->parameters;
	model->last_file = data->header;
}
void navigation_data_model_update_observation(MessageSatelliteObservation * data)
{
	NavigationDataModel * model = shared_instance();
	ChannelTrackingStatus channel_tracking_status;
	for(int i = 0;i<MAX_SATELLITE_NUMBER;i++)model->observation_available[i] = unavailable_flag;
	model->observation_available_amount = data->observation_number;
	for (int i = 0;i < data->observation_number;i++)
	{
		get_tracking_status(data->infomation[i].tracking_status, channel_tracking_status);
		if (channel_tracking_status.tracking_state != sky_search)// 未锁星则不可用。
		{
			int satellite_index = data->infomation[i].satellite_number - 1;
			model->observation_available[satellite_index] = available_flag;
			
			model->observation[satellite_index] = data->infomation[i];
		}
	}
	model->last_file = data->header;
}

void navigation_model_reset()
{
	NavigationDataModel * model = shared_instance();
	model->last_observation_amount = model->observation_available_amount;
	model->observation_available_amount = 0;
	memcpy(model->last_observation_available, model->observation_available,  MAX_SATELLITE_NUMBER);
	memcpy(model->last_observation, model->observation, sizeof(SatelliteInfomation) * MAX_SATELLITE_NUMBER);
	for (int i = 0;i < MAX_SATELLITE_NUMBER;i++)
	{
		model->observation_available[i] = unavailable_flag;
	}
	add_history(model->history, model->user_location);
}
void navigation_model_calculation_process()
{
	NavigationDataModel * model = shared_instance();
	int satellite_amount = 0;
	GPSTime t = GPSTime(model->last_file.week, model->last_file.GPS_sec_milisecond / 1000.0);
	SpaceLocation satellite_locations[MAX_SATELLITE_NUMBER];
	SatelliteImportantData data[MAX_SATELLITE_NUMBER];
	double satellite_fake_distance[MAX_SATELLITE_NUMBER];
	double satellite_obs_err[MAX_SATELLITE_NUMBER];
	double e_fd[MAX_SATELLITE_NUMBER] = { 0 };
	bool correction_judge_available[MAX_SATELLITE_NUMBER];
	int correction_banned_amount = 0;
	for (int i = 0;i < MAX_SATELLITE_NUMBER;i++)
	{
		if (model->ephemeris_available[i] == available_flag && model->observation_available[i] == available_flag)
		{
			bool continus_check = true;
			/*double diff = 0;
			if (model->predict_available == available_flag)
			{
				diff = fabs(model->predict_fakedistance[i] - model->observation[i].pseudorange);
				continus_check = diff < 30;
			}*/
			if (model->observation[i].pseudorange_deviation < 100 && continus_check)
			{
				if (satellite_position_calculation(model->ephemeris + i, t, satellite_locations + satellite_amount, data[satellite_amount], i, model->observation[i].pseudorange))
				{
					model->satellite_locations[i] = satellite_locations[satellite_amount];
					satellite_fake_distance[satellite_amount] = model->observation[i].pseudorange;
					satellite_obs_err[satellite_amount] = pow(model->observation[i].pseudorange_deviation, 2);
					e_fd[i] = model->observation[i].pseudorange;

					data[satellite_amount].A = model->ephemeris[i].semi_major_axis;
					data[satellite_amount].dt = model->ephemeris[i].clock_aging_parameter_sec;
					data[satellite_amount].ddt = model->ephemeris[i].clock_aging_parameter_1;
					data[satellite_amount].dddt = model->ephemeris[i].clock_aging_parameter_isec;
					data[satellite_amount].e = model->ephemeris[i].eccentricity;
					data[satellite_amount].TOC = GPSTime(model->ephemeris[i].GPSweek, model->ephemeris[i].clock_correction_term);
					data[satellite_amount].TOE = GPSTime(model->ephemeris[i].GPSweek, model->ephemeris[i].reference_time_for_ephemeris);
					data[satellite_amount].tgd = model->ephemeris[i].group_delay_difference;
					satellite_amount++;
				}
			}
		}

	}
	IonosphericAndUtcParameters * ip = model->ionospheric_available == available_flag ? &model->ionospheric : NULL;
	SpaceLocation * up = model->solve_result_available == available_flag ? &model->user_location : NULL;
	first_hand_correction(satellite_fake_distance, ip, satellite_locations, data, up, satellite_amount, t, correction_judge_available, correction_banned_amount);
	int changed = model->observation_available_amount - model->last_observation_amount;
	//if (model->last_observation_amount > model->observation_available_amount)changed = 1;
	//else {
	//	for (int i = 0;i < MAX_SATELLITE_NUMBER;i++)
	//	{
	//		if (model->last_observation_available[i] != model->observation_available[i]) {
	//			changed = true;
	//			break;
	//		}
	//	}
	//}
	if (t.sec == 544500)
	{
		int i = 0;
	}
	user_position_calculation(satellite_locations, satellite_fake_distance, satellite_obs_err, model->To, satellite_amount, model->user_location, model->solve_result_available, e_fd, changed, correction_judge_available,correction_banned_amount);
	
	if (model->solve_result_available == available_flag)
	{
		fake_distance_prediction(model->ephemeris, t, model->predict_fakedistance, model->ephemeris_available, e_fd, model->user_location, ip,model->To);
		model->predict_available = available_flag;
	}
}
void navigation_data_model_self_check()
{
	NavigationDataModel * model = shared_instance();
	SpaceLocation user = model->user_location;
	printf("\t自检\n");
	for (int i = 0;i < MAX_SATELLITE_NUMBER;i++)
	{
		if (model->observation_available[i] == available_flag && model->ephemeris_available[i] == available_flag)
		{
			double l1 = model->satellite_locations[i] - user;
			double l2 = model->observation[i].pseudorange;
			double l3 = model->ephemeris[i].clock_aging_parameter_sec * c;
		
			printf("\t\tprn %2d: %lf - ( %lf + %lf) = %lf\n", i + 1,l1, l2, l3,l1 - (l2 + l3));
		}
	}
}
void navigation_data_model_print()
{
	const NavigationDataModel * model = shared_instance();
	printf("    ---------------------------------------------------------------\n");
	printf("\t\tGPS周：%d\t\t\tGPS秒：%d\n", model->last_file.week, model->last_file.GPS_sec_milisecond / 1000);
	printf("\t1.星历:\t共%d颗卫星\n",model->ephemeris_available_amount);
	int can_use_flag[MAX_SATELLITE_NUMBER] = { unavailable_flag };
	int can_use_amount = 0;;
	for (int i = 0;i < MAX_SATELLITE_NUMBER;i++)
	{
		if (model->ephemeris_available[i] == available_flag)
		{
			can_use_flag[i] = available_flag;can_use_amount++;
			printf("\t\tprn : %2d\thealth : %d\n", model->ephemeris[i].satellite_number, model->ephemeris[i].health_status);
		}
	}
	printf("\t2.观测：\t共%d颗卫星\n", model->observation_available_amount);
	for (int i = 0;i < MAX_SATELLITE_NUMBER;i++)
	{
		if (model->observation_available[i] == available_flag)
		{
			printf("\t\tprn : %2d\ttracking status:%d\n", model->observation[i].satellite_number,model->observation[i].tracking_status);
		}
		else if (can_use_flag[i] == available_flag) {
			can_use_flag[i] = unavailable_flag;
			can_use_amount--;
		}
	}
	printf("\t#.可参与解算卫星数：%d\n", can_use_amount);
	if (can_use_amount != 0)
	{
		printf("\t\tprn:");
	}
	for (int i = 0;i < MAX_SATELLITE_NUMBER;i++)
	{
		if (can_use_flag[i] == available_flag)
		{
			printf("%2d,", i + 1);
		}
	}
	printf("\n\t3.接收机内部滤波定位结果：");
	printf(model->position_available == available_flag ? "可用\n" : "不可用\n");
	if (model->position_available == available_flag)
	{
		printf("\tLat : %3.10lf,Lon : %3.10lf,Height : %lf\n", model->position.latitude, model->position.longitude, model->position.height);
	}

	printf("\t4.电离层及UTC参数：");
	printf(model->ionospheric_available == available_flag ? "可用\n" : "不可用\n");

	printf("\t5.自解：");
	printf(model->solve_result_available == available_flag ? "可用\n" : "不可用\n");
	if (model->solve_result_available == available_flag)
	{
		BLHLocation location = model->user_location.toBLH();
		printf("\tLat : %3.10lf,Lon : %3.10lf,Height : %5.3lf\n", location.latitude * 180.0 / PI, location.longitude * 180.0 / PI, location.height);
	
		//fprintf(model->result_file, "%d,%3.12lf,%3.12lf,%5.4lf\n",model->last_file.GPS_sec_milisecond/1000, location.latitude * 180.0 / PI, location.longitude * 180.0 / PI, location.height);
	}
}
void navigation_data_model_resulting()
{
	NavigationDataModel * model = shared_instance();
	SpaceLocation mean_location = { 0,0,0 };
	for (Node * node = model->history->first;node != NULL;node = node->next)
	{
		mean_location.X += node->data.X;
		mean_location.Y += node->data.Y;
		mean_location.Z += node->data.Z;
	}
	mean_location.X /= model->history->amount;
	mean_location.Y /= model->history->amount;
	mean_location.Z /= model->history->amount;

	Matrix * E = mean_location.toBLH().getENUTrans();
	Matrix * dpos = malloc_mat(3, 1);
	Matrix * e = NULL;

	for (Node * node = model->history->first;node != NULL;node = node->next)
	{
		dpos->data[0][0] = node->data.X - mean_location.X;
		dpos->data[1][0] = node->data.Y - mean_location.Y;
		dpos->data[2][0] = node->data.Z - mean_location.Z;
		mat_multiply(E, dpos, e);
		fprintf(model->result_file, "%lf,%lf,%lf\n", e->data[0][0], e->data[1][0], e->data[2][0]);
	}

	free_mat(E);
	free_mat(dpos);
	free_mat(e);
}
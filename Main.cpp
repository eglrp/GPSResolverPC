#pragma warning(disable : 4996)
#include <stdlib.h>
#include "Data.h"
#include "Navigation.h"
#include "DetailedFunctions.h"




int main()
{
	FILE * fp = fopen("20150207.bin", "rb");
	init();
	//FILE * fp_write = fopen("statistic.txt", "w");
	system("pause");
	//freopen("console.txt", "w", stdout);
	int count = 0;
	while(!feof(fp))
	{
		try {
			count++;
			int i = 0;
			while(i != id_sat_range)read_file_for_another_frame(fp, i);
			navigation_model_calculation_process();
			//navigation_data_model_print();
			//navigation_data_model_self_check();
			navigation_model_reset();
			//system("pause");
			//system("cls");
			//fprintf(fp_write, "%d\n", i);
		}
		catch (char * description)
		{
			printf("error : %s\n", description);
			system("pause");
			break;
		}
	}
	navigation_data_model_resulting();
	//fclose(fp_write);
	_fcloseall();
	//fclose(fp);
	printf("done");
	system("pause");
	return 0;
}
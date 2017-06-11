#include "Time.h"

bool isBigMonth(int month)
{
	const int bigMonth[] = { 1,3,5,7,8,10,12 };
	for (int i = 0;i < 7;i++)
	{
		if (bigMonth[i] == month)return true;
	}
	return false;
}
int date_amount_of_month(int year, int month)
{
	if (month != 2)
	{
		return isBigMonth(month) ? 31 : 30;
	}
	else {
		if (year % 4 == 0 && year % 100 != 0 || year % 400 == 0)
		{
			return 29;
		}
		else return 28;
	}

}
bool utc::operator>(utc u1)
{
	if (year == u1.year)
		if (month == u1.month)
			if (date == u1.date)
				if (hour == u1.hour)
					if (minute == u1.minute)
						if (sec == u1.sec)
							return false;
						else if (sec > u1.sec)return true;
						else return false;
					else if (minute > u1.minute)return true;
					else return false;
				else if (hour > u1.hour)return true;
				else return false;
			else if (date > u1.date)return true;
			else return false;
		else if (month > u1.month)return true;
		else return false;
	else if (year > u1.year)return true;
	else return false;
}
utc::utc(int y, int m, int d, int h, int min, double s) :year(y), month(m), date(d), hour(h), minute(min), sec(s) {}
utc::utc() : year(0), month(0), date(0), hour(0), minute(0), sec(0.0) {}
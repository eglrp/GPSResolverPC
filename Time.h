#pragma once
#include <math.h>
bool isBigMonth(int month);
int date_amount_of_month(int year, int month);
struct utc
{
	int year;
	int month;
	int date;
	int hour;
	int minute;
	double sec;
	utc(int y, int m, int d, int h, int min, double s);
	utc();
	bool operator>(utc u1);
	void update_one_sec()
	{
		sec++;
		while (sec >= 60) {
			sec -= 60;
			minute++;
			while (minute >= 60)
			{
				minute -= 60;
				hour++;
				while (hour >= 24)
				{
					hour -= 24;
					date++;
					int max_date = date_amount_of_month(year, month);
					while (date > max_date)
					{
						date -= max_date;
						month++;
						while (month > 12)
						{
							month -= 12;
							year++;
						}
					}
				}
			}
		}
	}
};
struct MJDTime {
	int days;
	double frac_day;
	MJDTime(utc * time)
	{
		int y, m, temp;
		y = time->year + (time->year < 80 ? 2000 : 1900);
		m = time->month;
		if (m <= 2) {
			y--;
			m += 12;
		}
		temp = (int)(365.25 * y);
		temp += (int)(30.6001 * (m + 1));
		temp += time->date;
		temp -= 679019;

		days = temp;
		frac_day = time->hour + time->minute / 60.0 + time->sec / 3600.0;
		frac_day /= 24.0;
	}
};
struct GPSTime {
	int week;
	double sec;
	GPSTime(int w, double s) :week(w), sec(s) {}
	GPSTime(MJDTime * time)
	{
		week = (int)((time->days - 44244) / 7);
		int remain = time->days - week * 7 - 44244;
		sec = (remain + time->frac_day) * 86400.0;
	}
	GPSTime(utc * time)
	{
		MJDTime m_time = MJDTime(time);
		week = (int)((m_time.days - 44244) / 7.0);
		int remain = m_time.days - week * 7 - 44244;
		sec = (remain + m_time.frac_day) * 86400.0;
	}
	GPSTime() = default;
	double operator-(GPSTime time2)//asuming week is the same
	{
		return sec - time2.sec;
	}
};
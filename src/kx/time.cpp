#include "kx/time.h"
#include "kx/util.h"
#include "kx/io.h"
#include "kx/debug.h"

#include <chrono>
#include <thread>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <mutex>

namespace kx {

//assumes a format like 2018-08-10 14:00:00
static constexpr int LEAP_YEAR_PSUM[]{
    0, 0, 1, 1, 1, 1, 2, 2, 2, 2, //1970s
    3, 3, 3, 3, 4, 4, 4, 4, 5, 5, //1980s
    5, 5, 6, 6, 6, 6, 7, 7, 7, 7, //1990s
    8, 8, 8, 8, 9, 9, 9, 9, 10, 10, //2000s
    10, 10, 11, 11, 11, 11, 12, 12, 12, 12, //2010s
    13, 13, 13, 13, 14, 14, 14, 14, 15, 15, //2020s
    15, 15, 16, 16, 16, 16, 17, 17, 17, 17  //2030s
};
#ifdef KX_DEBUG
static constexpr int DAYS_PER_MONTH[]{
    0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};
#endif
static constexpr int DAYS_PER_MONTH_PSUM[]{
    0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365
};

Time::Delta::Delta(int ticks, Length length):
    ns(ticks * to_ns(length))
{}
Time::Delta::Delta(int64_t ticks, Length length):
    ns(ticks * to_ns(length))
{}
Time::Delta::Delta(double ticks, Length length):
    ns(ticks * to_ns(length))
{}
Time::Delta::Delta(std::string_view time_str, Format format)
{
    switch(format) {
    case Format::HH_MM_SS_ffffff: {
        k_expects(time_str.size() == 15); //quick sanity check

        int hour = (time_str[0]-'0')*10 + (time_str[1]-'0');
        int minute = (time_str[3]-'0')*10 + (time_str[4]-'0');
        int seconds = (time_str[6]-'0')*10 + (time_str[7]-'0');
        int us = (time_str[9]-'0')*100000 + (time_str[10]-'0')*10000 + (time_str[11]-'0')*1000 +
                 (time_str[12]-'0')*100 + (time_str[13]-'0')*10 + (time_str[14]-'0');

        int64_t sec = 0;

        sec += hour*60*60;
        sec += minute*60;
        sec += seconds;

        this->ns = 1e9*sec + 1e3*us;
        break;}
    default:
        log_error("unsupported Time::Delta string format");
    }
}
int64_t Time::Delta::to_int64(Length length) const
{
    return ns / to_ns(length);
}
double Time::Delta::to_double(Length length) const
{
    return ns / (double)to_ns(length);
}
bool Time::Delta::operator < (Time::Delta other) const
{
    return ns < other.ns;
}
bool Time::Delta::operator <= (Time::Delta other) const
{
    return ns <= other.ns;
}
bool Time::Delta::operator > (Time::Delta other) const
{
    return ns > other.ns;
}
bool Time::Delta::operator >= (Time::Delta other) const
{
    return ns >= other.ns;
}
bool Time::Delta::operator == (Time::Delta other) const
{
    return ns == other.ns;
}
bool Time::Delta::operator != (Time::Delta other) const
{
    return ns != other.ns;
}
Time::Delta Time::Delta::operator + (Delta other) const
{
    return Delta(ns + other.ns, Time::Length::ns);
}
void Time::Delta::operator += (Delta other)
{
    ns += other.ns;
}

Time::Delta Time::Delta::operator - (Delta other) const
{
    return Delta(ns - other.ns, Time::Length::ns);
}
void Time::Delta::operator -= (Delta other)
{
    ns -= other.ns;
}

Time::Delta Time::Delta::operator * (double val) const
{
    return Time::Delta(ns * val, Time::Length::ns);
}
Time::Delta Time::Delta::operator * (int64_t val) const
{
    return Time::Delta(ns * val, Time::Length::ns);
}
Time::Delta Time::Delta::operator * (uint64_t val) const
{
    return Time::Delta((int64_t)(ns * val), Time::Length::ns);
}
void Time::Delta::operator *= (double val)
{
    ns *= val;
}
void Time::Delta::operator *= (int64_t val)
{
    ns *= val;
}
void Time::Delta::operator *= (uint64_t val)
{
    ns *= val;
}

Time::Delta Time::Delta::operator / (double val) const
{
    return Time::Delta(ns / val, Time::Length::ns);
}
Time::Delta Time::Delta::operator / (int64_t val) const
{
    return Time::Delta(ns / val, Time::Length::ns);
}
Time::Delta Time::Delta::operator / (uint64_t val) const
{
    return Time::Delta((int64_t)(ns / val), Time::Length::ns);
}
double Time::Delta::operator / (Delta other) const
{
    return ns / (double)other.ns;
}
void Time::Delta::operator /= (double val)
{
    ns /= val;
}
void Time::Delta::operator /= (int64_t val)
{
    ns /= val;
}
void Time::Delta::operator /= (uint64_t val)
{
    ns /= val;
}
bool Time::Delta::is_multiple_of(Time::Delta delta) const
{
    return ns % delta.ns == 0;
}

int64_t Time::to_ns(Length length)
{
    switch(length)
    {
    case Length::nanosecond:
    case Length::ns:
        return 1;
    case Length::microsecond:
    case Length::us:
        return 1e3;
    case Length::millisecond:
    case Length::ms:
        return 1e6;
    case Length::second:
        return 1e9;
    case Length::minute:
        return 6e10;
    case Length::hour:
        return 36e11;
    case Length::day:
        return 864e11;
    case Length::week:
        return 432e12;
    default:
        //shouldn't reach here
        k_assert(false);
        return 0;
    }
}
Time Time::NA()
{
    Time ret;
    ret.ns_since_epoch = -1;
    return ret;
}
Time Time::epoch()
{
    Time ret;
    ret.ns_since_epoch = 0;
    return ret;
}

static constexpr int MIN_YEAR = 1; //1971
static constexpr int MAX_YEAR = 67; //2037

Time Time::from_YmD(int year, int month, int day)
{
    year -= 1970;
    k_expects(year>=MIN_YEAR && year<=MAX_YEAR);
    k_expects(month>=1 && month<=12);
    if(year%4==2 && month==2) //February in a leap year
        k_expects(day<=29);
    else
        k_expects(day<=DAYS_PER_MONTH[month]);

    int64_t sec_since_epoch = 0;
    sec_since_epoch += (year*365 + LEAP_YEAR_PSUM[year-1])*24*60*60;
    sec_since_epoch += DAYS_PER_MONTH_PSUM[month-1]*24*60*60;
    sec_since_epoch += ((year%4==2) && (month>=3))*24*60*60;
    sec_since_epoch += (day-1)*24*60*60;

    Time t;
    t.ns_since_epoch = 1e9 * sec_since_epoch;
    return t;
}

Time::Time(int ticks, Length length):
    ns_since_epoch(ticks * to_ns(length))
{}
Time::Time(int64_t ticks, Length length):
    ns_since_epoch(ticks * to_ns(length))
{}
Time::Time(double ticks, Length length):
    ns_since_epoch(ticks * to_ns(length))
{}

Time::Time(std::string_view time_str, Format format)
{
    //We use k_expects on the year/month/day instead of k_ensures because if those checks fail,
    //it's much more likely to be an error in the user input (preconditions) than our internal
    //calculations (postconditions), which are very simple.
    switch(format) {
    case Format::YYYYmmDD: {
        k_expects(time_str.size() == 8); //quick sanity check

        int year = (time_str[0]-'0')*1000 + (time_str[1]-'0')*100 +
                   (time_str[2]-'0')*10 + (time_str[3]-'0') - 0;
        int month = (time_str[4]-'0')*10 + (time_str[5]-'0');
        int day = (time_str[6]-'0')*10 + (time_str[7]-'0');

        *this = from_YmD(year, month, day);
        break;}
    case Format::YYYY_mm_DD: {
        k_expects(time_str.size() == 10); //quick sanity check

        int year = (time_str[0]-'0')*1000 + (time_str[1]-'0')*100 +
                   (time_str[2]-'0')*10 + (time_str[3]-'0') - 0;
        int month = (time_str[5]-'0')*10 + (time_str[6]-'0');
        int day = (time_str[8]-'0')*10 + (time_str[9]-'0');

        *this = from_YmD(year, month, day);
        break;}
    case Format::YYYYmmDD_HH_MM_SS: {
        k_expects(time_str.size() == 17); //quick sanity check

        int year = (time_str[0]-'0')*1000 + (time_str[1]-'0')*100 +
                   (time_str[2]-'0')*10 + (time_str[3]-'0') - 1970;
        int month = (time_str[4]-'0')*10 + (time_str[5]-'0');
        int day = (time_str[6]-'0')*10 + (time_str[7]-'0');
        int hour = (time_str[9]-'0')*10 + (time_str[10]-'0');
        int minute = (time_str[12]-'0')*10 + (time_str[13]-'0');
        int seconds = (time_str[15]-'0')*10 + (time_str[16]-'0');

        k_expects(year>=MIN_YEAR && year<=MAX_YEAR);
        k_expects(month>=1 && month<=12);
        if(year%4==2 && month==2) //February in a leap year
            k_expects(day<=29);
        else
            k_expects(day<=DAYS_PER_MONTH[month]);

        int64_t sec_since_epoch = 0;
        sec_since_epoch += (year*365 + LEAP_YEAR_PSUM[year-1])*24*60*60;
        sec_since_epoch += DAYS_PER_MONTH_PSUM[month-1]*24*60*60;
        sec_since_epoch += ((year%4==2) && (month>=3))*24*60*60;
        sec_since_epoch += (day-1)*24*60*60;
        sec_since_epoch += hour*60*60;
        sec_since_epoch += minute*60;
        sec_since_epoch += seconds;

        this->ns_since_epoch = 1e9 * sec_since_epoch;
        break;}
    case Format::YYYY_mm_DD_HH_MM_SS: {
        k_expects(time_str.size() == 19); //quick sanity check

        int year = (time_str[0]-'0')*1000 + (time_str[1]-'0')*100 +
                   (time_str[2]-'0')*10 + (time_str[3]-'0') - 1970;
        int month = (time_str[5]-'0')*10 + (time_str[6]-'0');
        int day = (time_str[8]-'0')*10 + (time_str[9]-'0');
        int hour = (time_str[11]-'0')*10 + (time_str[12]-'0');
        int minute = (time_str[14]-'0')*10 + (time_str[15]-'0');
        int seconds = (time_str[17]-'0')*10 + (time_str[18]-'0');

        k_expects(year>=MIN_YEAR && year<=MAX_YEAR);
        k_expects(month>=1 && month<=12);
        if(year%4==2 && month==2) //February in a leap year
            k_expects(day<=29);
        else
            k_expects(day<=DAYS_PER_MONTH[month]);

        int64_t sec_since_epoch = 0;
        sec_since_epoch += (year*365 + LEAP_YEAR_PSUM[year-1])*24*60*60;
        sec_since_epoch += DAYS_PER_MONTH_PSUM[month-1]*24*60*60;
        sec_since_epoch += ((year%4==2) && (month>=3))*24*60*60;
        sec_since_epoch += (day-1)*24*60*60;
        sec_since_epoch += hour*60*60;
        sec_since_epoch += minute*60;
        sec_since_epoch += seconds;

        this->ns_since_epoch = 1e9 * sec_since_epoch;
        break;}
    case Format::YYYY_mm_DD_HH_MM_SS_ffffff: {
        k_expects(time_str.size() == 26); //quick sanity check

        int year = (time_str[0]-'0')*1000 + (time_str[1]-'0')*100 +
                   (time_str[2]-'0')*10 + (time_str[3]-'0') - 1970;
        int month = (time_str[5]-'0')*10 + (time_str[6]-'0');
        int day = (time_str[8]-'0')*10 + (time_str[9]-'0');
        int hour = (time_str[11]-'0')*10 + (time_str[12]-'0');
        int minute = (time_str[14]-'0')*10 + (time_str[15]-'0');
        int seconds = (time_str[17]-'0')*10 + (time_str[18]-'0');
        int us = (time_str[20]-'0')*100000 + (time_str[21]-'0')*10000 + (time_str[22]-'0')*1000 +
                 (time_str[23]-'0')*100 + (time_str[24]-'0')*10 + (time_str[25]-'0');

        k_expects(year>=MIN_YEAR && year<=MAX_YEAR);
        k_expects(month>=1 && month<=12);
        if(year%4==2 && month==2) //February in a leap year
            k_expects(day<=29);
        else
            k_expects(day<=DAYS_PER_MONTH[month]);

        int64_t sec_since_epoch = 0;
        sec_since_epoch += (year*365 + LEAP_YEAR_PSUM[year-1])*24*60*60;
        sec_since_epoch += DAYS_PER_MONTH_PSUM[month-1]*24*60*60;
        sec_since_epoch += ((year%4==2) && (month>=3))*24*60*60;
        sec_since_epoch += (day-1)*24*60*60;
        sec_since_epoch += hour*60*60;
        sec_since_epoch += minute*60;
        sec_since_epoch += seconds;

        this->ns_since_epoch = 1e9*sec_since_epoch + 1e3*us;
        break;}
    case Format::mm_DD_YYYY_HH_MM_SS_ffffff: {
        k_expects(time_str.size() == 26); //quick sanity check

        int year = (time_str[6]-'0')*1000 + (time_str[7]-'0')*100 +
                   (time_str[8]-'0')*10 + (time_str[9]-'0') - 1970;
        int month = (time_str[0]-'0')*10 + (time_str[1]-'0');
        int day = (time_str[3]-'0')*10 + (time_str[4]-'0');
        int hour = (time_str[11]-'0')*10 + (time_str[12]-'0');
        int minute = (time_str[14]-'0')*10 + (time_str[15]-'0');
        int seconds = (time_str[17]-'0')*10 + (time_str[18]-'0');
        int us = (time_str[20]-'0')*100000 + (time_str[21]-'0')*10000 + (time_str[22]-'0')*1000 +
                 (time_str[23]-'0')*100 + (time_str[24]-'0')*10 + (time_str[25]-'0');

        k_expects(year>=MIN_YEAR && year<=MAX_YEAR);
        k_expects(month>=1 && month<=12);
        if(year%4==2 && month==2) //February in a leap year
            k_expects(day<=29);
        else
            k_expects(day<=DAYS_PER_MONTH[month]);

        int64_t sec_since_epoch = 0;
        sec_since_epoch += (year*365 + LEAP_YEAR_PSUM[year-1])*24*60*60;
        sec_since_epoch += DAYS_PER_MONTH_PSUM[month-1]*24*60*60;
        sec_since_epoch += ((year%4==2) && (month>=3))*24*60*60;
        sec_since_epoch += (day-1)*24*60*60;
        sec_since_epoch += hour*60*60;
        sec_since_epoch += minute*60;
        sec_since_epoch += seconds;

        this->ns_since_epoch = 1e9*sec_since_epoch + 1e3*us;
        break;}
    default:
        log_error("unsupported time format");
        break;
    }
}
Time Time::now()
{
    auto t = std::chrono::high_resolution_clock::now();
    auto t_ns = std::chrono::time_point_cast<std::chrono::nanoseconds>(t);
    return Time(t_ns.time_since_epoch().count(), Time::Length::ns);
}

int64_t Time::to_int64(Length length) const
{
    return ns_since_epoch / to_ns(length);
}
double Time::to_double(Length length) const
{
    return ns_since_epoch / (double)to_ns(length);
}
bool Time::operator < (Time other) const
{
    return ns_since_epoch < other.ns_since_epoch;
}
bool Time::operator <= (Time other) const
{
    return ns_since_epoch <= other.ns_since_epoch;
}
bool Time::operator > (Time other) const
{
    return ns_since_epoch > other.ns_since_epoch;
}
bool Time::operator >= (Time other) const
{
    return ns_since_epoch >= other.ns_since_epoch;
}
bool Time::operator == (Time other) const
{
    return ns_since_epoch == other.ns_since_epoch;
}
bool Time::operator != (Time other) const
{
    return ns_since_epoch != other.ns_since_epoch;
}
Time::Delta Time::operator - (Time other) const
{
    return Delta(ns_since_epoch - other.ns_since_epoch, Time::Length::ns);
}
Time Time::operator - (Delta delta) const
{
    return Time(ns_since_epoch - delta.ns, Time::Length::ns);
}
Time Time::operator + (Time::Delta delta) const
{
    return Time(ns_since_epoch + delta.ns, Time::Length::ns);
}
void Time::operator += (Time::Delta delta)
{
    ns_since_epoch += delta.ns;
}
void Time::operator -= (Time::Delta delta)
{
    ns_since_epoch -= delta.ns;
}
bool Time::is_multiple_of(Time::Delta delta) const
{
    return ns_since_epoch % delta.ns == 0;
}

std::string Time::to_str(Format format) const
{
    if(*this == Time::NA())
        return "time_NA";

    switch(format) {
    case Format::YYYYmmDD: {
        std::tm tm = *this;
        std::stringstream ss;
        ss << std::put_time(&tm, "%Y%m%d");
        return ss.str();}
    case Format::YYYY_mm_DD: {
        std::tm tm = *this;
        std::stringstream ss;
        ss << std::put_time(&tm, "%Y-%m-%d");
        return ss.str();}
    case Format::HH_MM_SS: {
        std::tm tm = *this;
        std::stringstream ss;
        ss << std::put_time(&tm, "%H:%M:%S");
        return ss.str();}
    case Format::YYYY_mm_DD_HH_MM_SS: {
        std::tm tm = *this;
        std::stringstream ss;
        ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        return ss.str();}
    default:
        log_error("unsupported time format");
        break;
    }
    return "unsupported time format";
}
std::string Time::to_str(std::string_view format_str) const
{
    if(ns_since_epoch == -1)
        return "time_na";
    std::tm tm = *this;
    std::stringstream ss;
    ss << std::put_time(&tm, format_str.data());
    return ss.str();
}
std::time_t Time::to_time_t() const
{
    return std::time_t(to_int64(Length::second));
}
Time::operator std::tm() const
{
    std::tm tm{};
    auto time_t_this = to_time_t();
    #ifdef _WIN32
    gmtime_s(&tm, &time_t_this);
    #else
    gmtime_r(&time_t_this, &tm);
    #endif
    return tm;
}

void sleep_for(Time::Delta length)
{
    auto ns = length.to_int64(Time::Length::ns);
    std::this_thread::sleep_for(std::chrono::nanoseconds(ns));
}

}

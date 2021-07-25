#pragma once

#include <ctime>
#include <cstdint>
#include <string>

namespace kx {

/** This is an unzoned time that does not account for leap seconds.
 *  This internally keeps track of ns since epoch.
 *  Don't change the internal representation because some functions process Times
 *  as binary data.
 */
class Time final
{
    int64_t ns_since_epoch;
public:
    /** Length has no month or day since those are variable (months have a variable
      * number of days and years do too due to leap years)
      */
    enum class Length{
        nanosecond, ns,
        microsecond, us,
        millisecond, ms,
        second,
        minute,
        hour,
        day,
        week
    };
    enum class Format {
        HH_MM_SS,
        HH_MM_SS_ffffff,
        YYYYmmDD,
        YYYY_mm_DD,
        YYYYmmDD_HH_MM_SS,
        YYYY_mm_DD_HH_MM_SS,
        YYYY_mm_DD_HH_MM_SS_ffffff,
        mm_DD_YYYY_HH_MM_SS_ffffff
    };

    class Delta final
    {
        friend Time;

        int64_t ns;
    public:
        Delta() = default;
        explicit Delta(int ticks, Length length);
        explicit Delta(int64_t ticks, Length length);
        explicit Delta(double ticks, Length length);
        Delta(std::string_view time_str, Format format);

        int64_t to_int64(Length length) const;
        double to_double(Length length) const;

        bool operator < (Delta other) const;
        bool operator <= (Delta other) const;
        bool operator > (Delta other) const;
        bool operator >= (Delta other) const;
        bool operator == (Delta other) const;
        bool operator != (Delta other) const;

        Delta operator + (Delta other) const;
        void operator += (Delta other);

        Delta operator - (Delta other) const;
        void operator -= (Delta other);

        Delta operator * (double val) const;
        Delta operator * (int64_t val) const;
        Delta operator * (uint64_t val) const;
        void operator *= (double val);
        void operator *= (int64_t val);
        void operator *= (uint64_t val);

        Delta operator / (double val) const;
        Delta operator / (int64_t val) const;
        Delta operator / (uint64_t val) const;
        double operator / (Delta other) const;
        void operator /= (double val);
        void operator /= (int64_t val);
        void operator /= (uint64_t val);

        bool is_multiple_of(Delta delta) const;
    };

    static int64_t to_ns(Length length);

    /** IMPORTANT NOTE: Time::NA() is internally stored by setting ns_since_epoch to -1.
     *  You can still use the arithmetic operators on this, and you'll get a non-NA result.
     *  In addition, two NAs appear equal to each other. This is probably undesirable.
     *  I could check if a time was equal to NA in the arithmetic operators, but this would
     *  considerably slow down the arithmetic operators, so I don't.
     */
    static Time NA();
    static Time epoch();
    static Time from_YmD(int year, int month, int day);

    Time() = default;
    explicit Time(int ticks, Length length);
    explicit Time(int64_t ticks, Length length);
    explicit Time(double ticks, Length length);
    ///TODO: Ensure (Year, month, date) (which assign to *this) work
    Time(std::string_view time_str, Format format); ///supports years in [1971, 2037]

    static Time now(); ///returns UTC

    int64_t to_int64(Length length) const;
    double to_double(Length length) const;

    bool operator < (Time other) const;
    bool operator <= (Time other) const;
    bool operator > (Time other) const;
    bool operator >= (Time other) const;
    bool operator == (Time other) const;
    bool operator != (Time other) const;
    Delta operator - (Time other) const;
    Time operator - (Delta delta) const;
    Time operator + (Delta delta) const;
    void operator += (Delta delta);
    void operator -= (Delta delta);

    bool is_multiple_of(Delta delta) const;

    std::string to_str(Format format) const;
    std::string to_str(std::string_view format_str) const;

    /** operator time_t() leads to implicit conversions since time_t is typedefed as a
     *  integer type, so we make this a non-operator function instead.
     */
    std::time_t to_time_t() const;

    operator std::tm() const;
};

void sleep_for(Time::Delta length);

}

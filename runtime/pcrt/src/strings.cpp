#include "plstr.h"
#include "plmem.h"
#include <math.h>

Mutex sPrintMut;

void printva_(vararg(primal::string) list)
{
    AutoLock al(sPrintMut);
    for (auto& s : list)
    {
        osPrint(s.cz(), s.length());
    }
    osPrint("\n", 1);
}

usize argcount_()
{
    return (usize)sProcArgc;
}

primal::string argstr_(usize n)
{
    if (n < (usize)sProcArgc)
    {
        return primal::string(sProcArgv[n]);
    }
    return primal::string();
}

void prints(const char* str)
{
    osPrint(str, 0);
}

namespace primal
{

string string::float64Str(float64 num)
{
    // Adapted from https://stackoverflow.com/a/7097567/2473607
    constDef PRECISION = 0.00000000000001;
    string sobj;
    zstr s = sobj.ensureAlloc(32);

    // handle special cases
    if (isnan(num))
    {
        sobj.append("nan");
    }
    else if (isinf(num))
    {
        sobj.append("inf");
    }
    else if (num == 0.0)
    {
        sobj.append("0");
    }
    else
    {
        char* c = s;
        int neg = (num < 0);
        if (neg)
        {
            num = -num;
        }
        // calculate magnitude
        int m = log10(num);
        int useexp = (m >= 14 || (neg && m >= 9) || m <= -9);
        int m1;
        if (neg)
        {
            *(c++) = '-';
        }
        // set up for scientific notation
        if (useexp)
        {
            if (m < 0)
            {
                m -= 1.0;
            }
            num = num / pow(10.0, m);
            m1 = m;
            m = 0;
        }
        if (m < 1.0)
        {
            m = 0;
        }
        // convert the number
        while (num > PRECISION || m >= 0)
        {
            double weight = pow(10.0, m);
            if (weight > 0 && !isinf(weight))
            {
                int digit = floor(num / weight);
                num -= (digit * weight);
                *(c++) = '0' + digit;
            }
            if (m == 0 && num > 0)
            {
                *(c++) = '.';
            }
            m--;
        }
        if (useexp)
        {
            // convert the exponent
            *(c++) = 'e';
            if (m1 > 0)
            {
                *(c++) = '+';
            }
            else
            {
                *(c++) = '-';
                m1 = -m1;
            }
            m = 0;
            while (m1 > 0)
            {
                *(c++) = '0' + m1 % 10;
                m1 /= 10;
                m++;
            }
            c -= m;
            for (int i = 0, j = m - 1; i < j; i++, j--)
            {
                // swap
                c[i] ^= c[j];
                c[j] ^= c[i];
                c[i] ^= c[j];
            }
            c += m;
        }
        *(c) = '\0';
        sobj.mLen = strlen(s);
    }

    return sobj;
}


string string::uint64Str(uint64 num, int8 base, bool minus)
{
    string sobj;
    // For base 10 or higher, use only the fixed internal buffer
    zstr str = sobj.ensureAlloc(base >= 10 ? 30 : 66);

    // Convert to string (but chars will be in reverse order)
    usize i = 0;
    do
    {
        char digit = (char)(num % base);
        digit += (digit < 0xA) ? '0' : ('A' - 0xA);
        str[i++] = digit;

        num /= base;
    } while (num);
    if (minus)
    {
        str[i++] = '-';
    }

    str[i] = '\0';
    sobj.mLen = i;

    // Reverse the string
    i--;
    for (usize j = 0; j < i; j++, i--)
    {
        char tmp = str[i];
        str[i] = str[j];
        str[j] = tmp;
    }
    return sobj;
}

string string::int64Str(int64 num)
{
    if (num < 0)
    {
        return uint64Str(-1 * num, 10, true);
    }
    else
    {
        return uint64Str(num, 10);
    }
}


} // namespace primal
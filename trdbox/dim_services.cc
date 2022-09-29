#include "dim_services.hh"

#include <math.h>
#include <string.h>
#include <string>

using namespace std;

dim_int::dim_int(string name)
   : DimService(name.c_str(),value),
     value(0)
{}


void dim_int::assign(int x)
{
   if (value != x) {
      value = x;
      updateService();
   }
}

dim_int::operator int()
{
   return value;
}

dim_float::dim_float(string name)
   : DimService(name.c_str(),value),
     value(0.),
     deadband(0.01)
{}

void dim_float::operator=(float x)
{
   if (fabs(x-value) >= deadband) {
      value = x;
      updateService();
   }
}

dim_float::operator float()
{
   return value;
}



dim_string::dim_string(string name)
   : DimService(name.c_str(), buffer)
{
   strncpy(buffer, "none", BUF_SIZE);
}

void dim_string::operator=(char *str)
{
   strncpy(buffer, str, BUF_SIZE);
   buffer[BUF_SIZE-1]='\0'; // Force NULL termination
   updateService();
}

void dim_string::operator=(string str)
{
   strncpy(buffer, str.c_str(), BUF_SIZE);

   if(str.size()>BUF_SIZE-1) {  // indicate, that the string has been truncated
      buffer[BUF_SIZE-7] = ' ';
      buffer[BUF_SIZE-6] = '[';
      buffer[BUF_SIZE-5] = '.';
      buffer[BUF_SIZE-4] = '.';
      buffer[BUF_SIZE-3] = '.';
      buffer[BUF_SIZE-2] = ']';
   }

   buffer[BUF_SIZE-1]='\0'; // Force NULL termination
   updateService();
}



dim_string::operator string()
{
   return std::string(buffer);
}

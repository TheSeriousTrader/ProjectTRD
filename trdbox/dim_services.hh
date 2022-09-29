#ifndef DIM_SERVICES_HH
#define DIM_SERVICES_HH

#include <string>
#include <dim/dis.hxx>

class dim_int : public DimService
{
public:
   dim_int(std::string name);

  void operator=(int x)   { assign (x); }
  void operator--()       { assign (value-1); }
  void operator--(int x)  { assign (value-1); }
  void operator++()       { assign (value+1); }
  void operator++(int x)  { assign (value+1); }
  void operator+=(int x)  { assign (value+x); }
  void operator-=(int x)  { assign (value-x); }
  
  operator int();
 
protected:
  void assign(int x);
  int value;

};



class dim_float : public DimService
{
public:
   dim_float(std::string name);

   void operator=(float x);
   operator float();

protected:
   float value;
   float deadband;
   
};


class dim_string : public DimService
{
public:
   dim_string(std::string name);

   void operator=(char *str);
   void operator=(std::string str);
   operator std::string();

protected:
   static const int BUF_SIZE=250;
   char buffer[BUF_SIZE];
   
};

#endif

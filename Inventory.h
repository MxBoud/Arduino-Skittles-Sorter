#ifndef Inventory_h
#define Inventory_h
#include <arduino.h>

class Inventory {
public:
  String items[2];
  void MeasureAndPush(String);
};

#endif 

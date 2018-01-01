#include "Inventory.h"
#include <arduino.h>

void Inventory::MeasureAndPush(String addedMeasurement) {
  items[1] = items[0];
  items[0] = addedMeasurement;
  Serial.println("Inventory has been pushed.");
  Serial.println(items[1]);
}

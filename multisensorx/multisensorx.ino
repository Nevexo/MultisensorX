// MultisensorX (MSX) - A NodeMCU Multisensor Harness
// By Nevexo (github.com/nevexo/multisensorx)

#include "CONFIG.h"
void setup() {

  if (FEATURE_SERIAL_LOGGING == true) {
    Serial.begin(SERIAL_BUARD);
    Serial.println("[MSX] Welcome to MultisensorX!");
  }
}


void loop() {
  // put your main code here, to run repeatedly:
}

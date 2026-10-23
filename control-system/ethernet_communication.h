#pragma once

#include <Arduino.h>

#include "state.h"

// Changed to take SystemState for error reporting
void initializeEthernetCommunication(SystemState& state);
void pollEthernetCommunication(SystemState& state);
bool ethernetInterfaceReady();
IPAddress ethernetLocalIP();
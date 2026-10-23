/**
 * ethernet_communication.cpp
 *
 * DHCP-only Ethernet transport for the on-board WIZnet W5100S.
 */
#include "hardware/watchdog.h"

#include "ethernet_communication.h"

#include <SPI.h>
#include <Ethernet.h>

#include "config.h"
#include "error_handling.h"
#include "serial_communication.h"

namespace {

EthernetServer server(TCP_PORT);
EthernetClient client;
bool dhcpAcquired = false;
uint32_t lastDhcpAttemptMs = 0;

void printStatus(const String& message) {
  if (isSerialHostConnected()) {
    Serial.println(message);
  }
}

void stopClient() {
  if (client) {
    client.stop();
  }
  client = EthernetClient();
}

bool acquireDhcpLease() {
  printStatus("Requesting DHCP lease...");

  watchdog_enable(WATCHDOG_STARTUP_TIMEOUT_MS, true);
  const bool leaseObtained = Ethernet.begin(const_cast<uint8_t*>(ETHERNET_MAC_ADDRESS),
                                            ETHERNET_DHCP_TIMEOUT_MS,
                                            ETHERNET_DHCP_RESPONSE_TIMEOUT_MS) != 0;
  watchdog_enable(I2C_WATCHDOG_TIMEOUT_MS, true);

  if (!leaseObtained) {
    return false;
  }

  printStatus("DHCP lease acquired: " + Ethernet.localIP().toString());
  return true;
}

}  // namespace

// UPDATED: Takes SystemState&
void initializeEthernetCommunication(SystemState& state) {
  Ethernet.init(WNET_CS_PIN);

  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    // UPDATED: Added state to raiseError
    raiseError(Serial, state, ErrorCode::Transport, "TRANSPORT", "W5100S hardware not detected");
    return;
  }

  lastDhcpAttemptMs = millis();
  dhcpAcquired = acquireDhcpLease();

  if (!dhcpAcquired) {
    // UPDATED: Added state to raiseError
    raiseError(Serial, state, ErrorCode::Transport, "TRANSPORT", "DHCP lease not acquired; will keep retrying");
    return;
  }

  server.begin();
}

bool ethernetInterfaceReady() {
  return dhcpAcquired && (Ethernet.localIP() != IPAddress(0, 0, 0, 0));
}

IPAddress ethernetLocalIP() {
  return Ethernet.localIP();
}

void pollEthernetCommunication(SystemState& state) {
  if (state.transport_mode != TransportMode::Ethernet) {
    stopClient();
    return;
  }

  if (Ethernet.linkStatus() == LinkOFF) {
    stopClient();
    dhcpAcquired = false;
    return;
  }

  if (!dhcpAcquired) {
    if (millis() - lastDhcpAttemptMs >= ETHERNET_DHCP_RETRY_MS) {
      lastDhcpAttemptMs = millis();
      dhcpAcquired = acquireDhcpLease();
      if (dhcpAcquired) {
        server.begin();
      }
    }
    return;
  }

  watchdog_enable(WATCHDOG_STARTUP_TIMEOUT_MS, true);
  Ethernet.maintain();
  watchdog_enable(I2C_WATCHDOG_TIMEOUT_MS, true);

  if (!client || !client.connected()) {
    stopClient();
    client = server.available();
  }

  if (client && client.connected()) {
    handleSerialCommands(client, state, CommandSource::Ethernet);
  }
}
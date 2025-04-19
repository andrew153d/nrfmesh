/*
 * Copyright 2020 Andrew Rossignol andrew.rossignol@gmail.com
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <arpa/inet.h>
#include <fcntl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <RF24/RF24.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <tclap/CmdLine.h>
#include <unistd.h>

#include "nerfnet/net/primary_radio_interface.h"
#include "nerfnet/net/secondary_radio_interface.h"
#include "nerfnet/util/log.h"
#include "nerfnet/util/time.h"

using namespace std;

// A description of the program.
constexpr char kDescription[] =
    "A tool for creating a network tunnel over cheap NRF24L01 radios.";

// The version of the program.
constexpr char kVersion[] = "0.0.1";

// Sets flags for a given interface. Quits and logs the error on failure.
void SetInterfaceFlags(const std::string_view& device_name, int flags) {
  int fd = socket(AF_INET, SOCK_DGRAM, 0);
  CHECK(fd >= 0, "Failed to open socket: %s (%d)", strerror(errno), errno);

  struct ifreq ifr = {};
  ifr.ifr_flags = flags;
  strncpy(ifr.ifr_name, std::string(device_name).c_str(), IFNAMSIZ);
  int status = ioctl(fd, SIOCSIFFLAGS, &ifr);
  CHECK(status >= 0, "Failed to set tunnel interface: %s (%d)",
      strerror(errno), errno);
  close(fd);
}

void SetIPAddress(const std::string_view& device_name,
                  const std::string_view& ip, const std::string& ip_mask) {
  int fd = socket(AF_INET, SOCK_DGRAM, 0);
  CHECK(fd >= 0, "Failed to open socket: %s (%d)", strerror(errno), errno);

  struct ifreq ifr = {};
  strncpy(ifr.ifr_name, std::string(device_name).c_str(), IFNAMSIZ);

  ifr.ifr_addr.sa_family = AF_INET;
  CHECK(inet_pton(AF_INET, std::string(ip).c_str(),
        &reinterpret_cast<struct sockaddr_in*>(&ifr.ifr_addr)->sin_addr) == 1,
      "Failed to assign IP address: %s (%d)", strerror(errno), errno);
  int status = ioctl(fd, SIOCSIFADDR, &ifr);
  CHECK(status >= 0, "Failed to set tunnel interface ip: %s (%d)",
      strerror(errno), errno);

  ifr.ifr_netmask.sa_family = AF_INET;
  CHECK(inet_pton(AF_INET, std::string(ip_mask).c_str(),
        &reinterpret_cast<struct sockaddr_in*>(&ifr.ifr_netmask)->sin_addr) == 1,
      "Failed to assign IP mask: %s (%d)", strerror(errno), errno);
  status = ioctl(fd, SIOCSIFNETMASK, &ifr);
  CHECK(status >= 0, "Failed to set tunnel interface mask: %s (%d)",
      strerror(errno), errno);
  close(fd);
}

// Opens the tunnel interface to listen on. Always returns a valid file
// descriptor or quits and logs the error.
int OpenTunnel(const std::string_view& device_name) {
  int fd = open("/dev/net/tun", O_RDWR);
  CHECK(fd >= 0, "Failed to open tunnel file: %s (%d)", strerror(errno), errno);

  struct ifreq ifr = {};
  ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
  strncpy(ifr.ifr_name, std::string(device_name).c_str(), IFNAMSIZ);

  int status = ioctl(fd, TUNSETIFF, &ifr);
  CHECK(status >= 0, "Failed to set tunnel interface: %s (%d)",
      strerror(errno), errno);
  return fd;
}

int main(int argc, char **argv)
{
  // Load configuration file
  ConfigParser config("/etc/nrfmesh/nrfmesh.conf");
  config.load();

  // char payload[32] = "Hello World!";

  // RF24 radio(config.getConfig().ce_pin, 0);

  // if (!radio.begin())
  // {
  //   std::cout << "radio hardware is not responding!!" << std::endl;
  //   return 0; // quit now
  // }

  // uint8_t address[2][6] = {"1Node", "2Node"};

  // int radioNumber = config.getConfig().device_id;
  // LOGI("Radio number: %d", radioNumber);

  // radio.setPayloadSize(sizeof(payload));
  // radio.setPALevel(RF24_PA_LOW);
  // radio.openWritingPipe(address[radioNumber]);
  // radio.openReadingPipe(1, address[!radioNumber]);
  // struct timespec startTimer, endTimer;
  
  // auto start = nerfnet::TimeNowUs();
  // while(1)
  // {
  //   radio.startListening(); // put radio in RX mode
  //   delay(10);            // wait for 1 second
  //   if (radio.available())
  //   {
  //     uint8_t pipe;
  //     if (radio.available(&pipe))
  //     { // is there a payload? get the pipe number that received it
  //       uint8_t bytes = radio.getPayloadSize(); // get the size of the payload
  //       radio.read(&payload, bytes);            // fetch payload from FIFO
  //       cout << "Received " << (unsigned int)bytes;      // print the size of the payload
  //       cout << ": " << payload << endl;                 // print the payload's value
  //     }
  //   }
  //   if(nerfnet::TimeNowUs() - start > 1000000)
  //   {
  //     start = nerfnet::TimeNowUs();           // end the timer
  //     radio.stopListening(); // put radio in TX mode
  //     bool result = radio.write(&payload[0], sizeof(payload)); // transmit & save the report
  //     if(result)
  //     {
  //       LOGI("Transmission successful!");
  //     }else
  //     {
  //       LOGI("Transmission failed or timed out");
  //     }
  //   }
  // }

  // if (radioNumber == 0)
  // {
  //   // master
  //   radio.stopListening(); // put radio in TX mode

  //   unsigned int failure = 0; // keep track of failures
  //   while (failure < 60)
  //   {
  //     clock_gettime(CLOCK_MONOTONIC_RAW, &startTimer);         // start the timer
  //     bool result = radio.write(&payload[0], sizeof(payload)); // transmit & save the report
  //     uint32_t timerElapsed = nerfnet::TimeNowUs();            // end the timer

  //     if (result)
  //     {
  //       // payload was delivered
  //       cout << "Transmission successful! Time to transmit = " << endl;
  //       // cout << timerElapsed;                     // print the timer result
  //       // cout << " us. Sent: " << payload << endl; // print payload sent
  //       //  payload += 0.01;                          // increment float payload
  //     }
  //     else
  //     {
  //       // payload was not delivered
  //       cout << "Transmission failed or timed out" << endl;
  //       failure++;
  //     }

  //     // to make this example readable in the terminal
  //     delay(1000); // slow transmissions down by 1 second
  //   }
  //   cout << failure << " failures detected. Leaving TX role." << endl;
  // }
  // else
  // {
  //   radio.startListening(); // put radio in RX mode

  //   time_t startTimer = time(nullptr); // start a timer
  //   while (time(nullptr) - startTimer < 60)
  //   { // use 6 second timeout
  //     uint8_t pipe;
  //     if (radio.available(&pipe))
  //     {                                         // is there a payload? get the pipe number that received it
  //       uint8_t bytes = radio.getPayloadSize(); // get the size of the payload
  //       radio.read(&payload, bytes);            // fetch payload from FIFO
  //       cout << "Received " << (unsigned int)bytes << endl;      // print the size of the payload
  //       // cout << " bytes on pipe " << (unsigned int)pipe; // print the pipe number
  //       // cout << ": " << payload << endl;                 // print the payload's value
  //       startTimer = time(nullptr); // reset timer
  //     }
  //   }
  //   cout << "Nothing received in 6 seconds. Leaving RX role." << endl;
  //   radio.stopListening();
  // }
  // exit(0);
  TCLAP::CmdLine cmd(kDescription, ' ', kVersion);
  TCLAP::ValueArg<int> mode_arg("", "mode",
                                "Set the mode of operation (0: PRIMARY, 1: SECONDARY, 2: COMMON).",
                                false, 0, "integer", cmd);

  cmd.parse(argc, argv);

  // return 0;

  std::string tunnel_ip = config.getConfig().tunnel_ip_address; // tunnel_ip_arg.getValue();
  // if (!tunnel_ip_arg.isSet())
  // {
  //   if (primary_arg.getValue())
  //   {
  //     tunnel_ip = "192.168.10.1";
  //   }
  //   else if (secondary_arg.getValue())
  //   {
  //     tunnel_ip = "192.168.10.2";
  //   }
  //   else if (common_arg.getValue())
  //   {
  //     tunnel_ip = config.get("ip_address");
  //   }
  // }

  // Setup tunnel.
  int tunnel_fd = OpenTunnel(interface_name_arg.getValue());
  LOGI("tunnel '%s' opened", interface_name_arg.getValue().c_str());
  SetInterfaceFlags(interface_name_arg.getValue(), IFF_UP);
  LOGI("tunnel '%s' up", interface_name_arg.getValue().c_str());
  SetIPAddress(interface_name_arg.getValue(), tunnel_ip,
      tunnel_ip_mask.getValue());
  LOGI("tunnel '%s' configured with '%s' mask '%s'",
       interface_name_arg.getValue().c_str(), tunnel_ip.c_str(),
       tunnel_ip_mask.getValue().c_str());

  if ((!mode_arg.isSet() && config.getConfig().mode == PRIMARY) || (mode_arg.isSet() && mode_arg.getValue() == 0))
  {
    LOGI("Starting primary radio interface");
    nerfnet::PrimaryRadioInterface radio_interface(
        config.getConfig().ce_pin, tunnel_fd,
        0xFFAB, 0xFFAB,
        config.getConfig().channel, config.getConfig().poll_interval);
    radio_interface.SetTunnelLogsEnabled(config.getConfig().enable_tunnel_logs);
    radio_interface.Run();
  }
  else if ((!mode_arg.isSet() && config.getConfig().mode == SECONDARY) || (mode_arg.isSet() && mode_arg.getValue() == 1))
  {
    LOGI("Starting secondary radio interface");
    nerfnet::SecondaryRadioInterface radio_interface(
        config.getConfig().ce_pin, tunnel_fd,
        0xFFAB, 0xFFAB,
        config.getConfig().channel);
    radio_interface.SetTunnelLogsEnabled(config.getConfig().enable_tunnel_logs);
    radio_interface.Run();
  }
  else if ((!mode_arg.isSet() && config.getConfig().mode == COMMON) || (mode_arg.isSet() && mode_arg.getValue() == 2))
  {
    LOGI("Starting common radio interface");
    nerfnet::CommonRadioInterface radio_interface(
        config.getConfig().device_id,
        config.getConfig().ce_pin, tunnel_fd,
        0xFFAB, 0XBBAF,
        config.getConfig().channel, config.getConfig().poll_interval);
    radio_interface.SetTunnelLogsEnabled(config.getConfig().enable_tunnel_logs);
    radio_interface.Run();
  }

  return 0;
}

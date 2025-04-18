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

#include "nerfnet/net/config_parser.h"
#include "nerfnet/net/primary_radio_interface.h"
#include "nerfnet/net/secondary_radio_interface.h"
#include "nerfnet/net/common_radio_interface.h"
#include "nerfnet/util/log.h"

// A description of the program.
constexpr char kDescription[] =
    "A tool for creating a network tunnel over cheap NRF24L01 radios.";

// The version of the program.
constexpr char kVersion[] = "0.0.1";

// Sets flags for a given interface. Quits and logs the error on failure.
void SetInterfaceFlags(const std::string_view &device_name, int flags)
{
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

void SetIPAddress(const std::string_view &device_name,
                  const std::string_view &ip, const std::string &ip_mask)
{
  int fd = socket(AF_INET, SOCK_DGRAM, 0);
  CHECK(fd >= 0, "Failed to open socket: %s (%d)", strerror(errno), errno);

  struct ifreq ifr = {};
  strncpy(ifr.ifr_name, std::string(device_name).c_str(), IFNAMSIZ);

  ifr.ifr_addr.sa_family = AF_INET;
  CHECK(inet_pton(AF_INET, std::string(ip).c_str(),
                  &reinterpret_cast<struct sockaddr_in *>(&ifr.ifr_addr)->sin_addr) == 1,
        "Failed to assign IP address: %s (%d)", strerror(errno), errno);
  int status = ioctl(fd, SIOCSIFADDR, &ifr);
  CHECK(status >= 0, "Failed to set tunnel interface ip: %s (%d)",
        strerror(errno), errno);

  ifr.ifr_netmask.sa_family = AF_INET;
  CHECK(inet_pton(AF_INET, std::string(ip_mask).c_str(),
                  &reinterpret_cast<struct sockaddr_in *>(&ifr.ifr_netmask)->sin_addr) == 1,
        "Failed to assign IP mask: %s (%d)", strerror(errno), errno);
  status = ioctl(fd, SIOCSIFNETMASK, &ifr);
  CHECK(status >= 0, "Failed to set tunnel interface mask: %s (%d)",
        strerror(errno), errno);
  close(fd);
}

// Opens the tunnel interface to listen on. Always returns a valid file
// descriptor or quits and logs the error.
int OpenTunnel(const std::string_view &device_name)
{
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

  //LOGI("Config interface name: %s", config.getConfig().interface_name.c_str());

  // // Parse command-line arguments.
  // TCLAP::CmdLine cmd(kDescription, ' ', kVersion);
  // TCLAP::ValueArg<std::string> interface_name_arg("i", "interface_name",
  //                                                 "Set to the name of the tunnel device.", false, "nerf0", "name", cmd);
  // TCLAP::ValueArg<uint16_t> ce_pin_arg("", "ce_pin",
  //                                      "Set to the index of the NRF24L01 chip-enable pin.", false, 22, "index",
  //                                      cmd);

  // TCLAP::SwitchArg primary_arg("", "primary",
  //                              "Run this side of the network in primary mode.", false);
  // TCLAP::SwitchArg secondary_arg("", "secondary",
  //                                "Run this side of the network in secondary mode.", false);
  // TCLAP::SwitchArg common_arg("", "common",
  //                             "Run this side of the network in common mode.", false);

  // TCLAP::ValueArg<std::string> tunnel_ip_arg("", "tunnel_ip",
  //                                            "The IP address to assign to the tunnel interface.", false, "", "ip",
  //                                            cmd);
  // TCLAP::ValueArg<std::string> tunnel_ip_mask("", "tunnel_mask",
  //                                             "The network mask to use for the tunnel interface.", false,
  //                                             "255.255.255.0", "mask", cmd);

  // std::vector<TCLAP::Arg *> xor_args = {&primary_arg, &secondary_arg, &common_arg};
  // cmd.xorAdd(xor_args);

  // TCLAP::ValueArg<uint32_t> primary_addr_arg("", "primary_addr",
  //                                            "The address to use for the primary side of nerfnet.",
  //                                            false, 0x90019001, "address", cmd);
  // TCLAP::ValueArg<uint32_t> secondary_addr_arg("", "secondary_addr",
  //                                              "The address to use for the secondary side of nerfnet.",
  //                                              false, 0x90009000, "address", cmd);

  // TCLAP::ValueArg<uint8_t> channel_arg("", "channel",
  //                                      "The channel to use for transmit/receive.", false, 1, "channel", cmd);
  // TCLAP::ValueArg<uint32_t> poll_interval_us_arg("", "poll_interval_us",
  //                                                "Used by the primary radio only to determine how often to poll.",
  //                                                false, 100, "microseconds", cmd);

  // TCLAP::SwitchArg enable_tunnel_logs_arg("", "enable_tunnel_logs",
  //                                         "Set to enable verbose logs for read/writes from the tunnel.", cmd);

  // cmd.parse(argc, argv);

  //return 0;

  std::string tunnel_ip = config.getConfig().tunnel_ip_address;//tunnel_ip_arg.getValue();
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
  int tunnel_fd = OpenTunnel(config.getConfig().interface_name);
  LOGI("tunnel '%s' opened", config.getConfig().interface_name.c_str());
  SetInterfaceFlags(config.getConfig().interface_name, IFF_UP);
  LOGI("tunnel '%s' up", config.getConfig().interface_name.c_str());
  SetIPAddress(config.getConfig().interface_name, tunnel_ip,
               config.getConfig().tunnel_netmask);
  LOGI("tunnel '%s' configured with '%s' mask '%s'",
       config.getConfig().interface_name.c_str(), tunnel_ip.c_str(),
       config.getConfig().tunnel_netmask.c_str());

  if (config.getConfig().mode == PRIMARY)
  {
    nerfnet::PrimaryRadioInterface radio_interface(
        config.getConfig().ce_pin, tunnel_fd,
        0xFFAB, 0XBBAF,
        config.getConfig().channel, config.getConfig().poll_interval);
    radio_interface.SetTunnelLogsEnabled(config.getConfig().enable_tunnel_logs);  
    radio_interface.Run();
  }
  else if (config.getConfig().mode == SECONDARY)
  {
    nerfnet::SecondaryRadioInterface radio_interface(
        config.getConfig().ce_pin, tunnel_fd,
        0xFFAB, 0XBBAF,
        config.getConfig().channel);
    radio_interface.SetTunnelLogsEnabled(config.getConfig().enable_tunnel_logs);
    radio_interface.Run();
  }
  else if (config.getConfig().mode == COMMON)
  {
    nerfnet::CommonRadioInterface radio_interface(
        config.getConfig().ce_pin, tunnel_fd,
        0xFFAB, 0XBBAF,
        config.getConfig().channel, config.getConfig().poll_interval);
    radio_interface.SetTunnelLogsEnabled(config.getConfig().enable_tunnel_logs);
    radio_interface.Run();
  }

  return 0;
}

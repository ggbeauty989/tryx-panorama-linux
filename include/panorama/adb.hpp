#pragma once

#include <optional>
#include <string>
#include <vector>

namespace panorama {

class Adb {
 public:
  static constexpr const char* MEDIA_PATH = "/sdcard/pcMedia/";

  // Returns true iff the `adb` binary is on PATH and executable.
  static bool is_available();
  static bool is_device_connected();
  static bool push(const std::string& local_path,
                   const std::string& remote_name);
  static std::optional<std::vector<std::string>> list_media();
  static bool file_exists(const std::string& filename);
  static bool remove(const std::string& filename);
  static bool reboot();

  // Drop the cached TRYX serial. Call when the device is disconnected so a
  // future hot-plug with a different unit doesn't keep targeting the old one.
  static void reset_cache();

 private:
  static std::string find_tryx_serial();
  static std::string escape_shell_arg(const std::string& input);
  static std::optional<std::string> run_command(
      const std::vector<std::string>& args);
};

}  // namespace panorama

#include "reed/media.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>

namespace fs = std::filesystem;

namespace reed {

std::string Media::get_extension(const std::string& path) {
  auto ext = fs::path(path).extension().string();
  std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
  return ext;
}

std::string Media::get_basename(const std::string& path) {
  return fs::path(path).stem().string();
}

std::string Media::get_filename(const std::string& path) {
  return fs::path(path).filename().string();
}

MediaType Media::detect_type(const std::string& path) {
  auto ext = get_extension(path);

  if (ext == ".gif") {
    return MediaType::Gif;
  }

  if (ext == ".mp4" || ext == ".webm" || ext == ".mkv" || ext == ".avi" ||
      ext == ".mov") {
    return MediaType::Video;
  }

  if (ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".bmp" ||
      ext == ".webp") {
    return MediaType::Image;
  }

  return MediaType::Unknown;
}

std::string Media::get_converted_name(const std::string& original) {
  return get_basename(original) + ".mp4";
}

bool Media::is_ffmpeg_available() {
  return std::system("ffmpeg -version > /dev/null 2>&1") == 0;
}

static std::string shell_escape(const std::string& s) {
  std::string result = "'";
  for (char c : s) {
    if (c == '\'') {
      result += "'\\''";
    } else {
      result += c;
    }
  }
  result += "'";
  return result;
}

bool Media::convert_gif_to_mp4(const std::string& input,
                               const std::string& output) {
  fs::create_directories(TMP_DIR);
  std::string cmd = "ffmpeg -y -i " + shell_escape(input) +
      " -movflags faststart -pix_fmt yuv420p"
      " -vf \"scale=trunc(iw/2)*2:trunc(ih/2)*2\" " +
      shell_escape(output) + " > /dev/null 2>&1";
  int ret = std::system(cmd.c_str());
  return ret == 0 && fs::exists(output);
}

bool Media::convert_to_mp4(const std::string& input,
                           const std::string& output) {
  fs::create_directories(TMP_DIR);
  std::string cmd = "ffmpeg -y -i " + shell_escape(input) +
      " -c:v libx264 -preset fast -crf 23"
      " -movflags faststart -pix_fmt yuv420p"
      " -vf \"scale=trunc(iw/2)*2:trunc(ih/2)*2\""
      " -an " + shell_escape(output) + " > /dev/null 2>&1";
  int ret = std::system(cmd.c_str());
  return ret == 0 && fs::exists(output);
}

bool Media::needs_conversion(const std::string& path) {
  auto ext = get_extension(path);
  return ext == ".webm" || ext == ".mkv" || ext == ".avi" || ext == ".mov" ||
         ext == ".gif";
}

}  // namespace reed

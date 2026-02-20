#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <csignal>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace {

volatile std::sig_atomic_t g_stop = 0;

void HandleSignal(int) {
  g_stop = 1;
}

std::string UrlDecode(const std::string& input) {
  std::string out;
  out.reserve(input.size());
  for (std::size_t i = 0; i < input.size(); ++i) {
    if (input[i] == '%' && i + 2 < input.size()) {
      const auto hex = input.substr(i + 1, 2);
      char* end = nullptr;
      long value = std::strtol(hex.c_str(), &end, 16);
      if (end != nullptr && *end == '\0') {
        out.push_back(static_cast<char>(value));
        i += 2;
        continue;
      }
    }
    if (input[i] == '+') {
      out.push_back(' ');
    } else {
      out.push_back(input[i]);
    }
  }
  return out;
}

std::string ContentTypeFor(const std::filesystem::path& path) {
  static const std::map<std::string, std::string> kTypes = {
      {".html", "text/html; charset=utf-8"},
      {".js", "application/javascript; charset=utf-8"},
      {".mjs", "application/javascript; charset=utf-8"},
      {".css", "text/css; charset=utf-8"},
      {".json", "application/json; charset=utf-8"},
      {".wasm", "application/wasm"},
      {".png", "image/png"},
      {".jpg", "image/jpeg"},
      {".jpeg", "image/jpeg"},
      {".wav", "audio/wav"},
      {".ico", "image/x-icon"},
  };
  const auto ext = path.extension().string();
  const auto it = kTypes.find(ext);
  if (it != kTypes.end()) {
    return it->second;
  }
  return "application/octet-stream";
}

bool StartsWith(const std::string& s, const std::string& prefix) {
  return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

void SendAll(int clientFd, const std::string& data) {
  std::size_t sent = 0;
  while (sent < data.size()) {
    const ssize_t n = send(clientFd, data.data() + sent, data.size() - sent, 0);
    if (n <= 0) {
      return;
    }
    sent += static_cast<std::size_t>(n);
  }
}

void SendFileResponse(int clientFd, int status, const std::string& statusText, const std::string& contentType,
                      const std::vector<char>& body) {
  std::ostringstream header;
  header << "HTTP/1.1 " << status << " " << statusText << "\r\n"
         << "Content-Type: " << contentType << "\r\n"
         << "Content-Length: " << body.size() << "\r\n"
         << "Connection: close\r\n"
         << "\r\n";
  SendAll(clientFd, header.str());
  if (!body.empty()) {
    std::size_t sent = 0;
    while (sent < body.size()) {
      const ssize_t n = send(clientFd, body.data() + sent, body.size() - sent, 0);
      if (n <= 0) {
        break;
      }
      sent += static_cast<std::size_t>(n);
    }
  }
}

std::vector<char> ReadFile(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    return {};
  }
  return std::vector<char>(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

std::filesystem::path ResolvePath(const std::filesystem::path& root, std::string requestTarget) {
  if (requestTarget.empty() || requestTarget == "/") {
    requestTarget = "/slam-raylib.html";
  }
  const auto questionPos = requestTarget.find('?');
  if (questionPos != std::string::npos) {
    requestTarget = requestTarget.substr(0, questionPos);
  }

  const std::string decoded = UrlDecode(requestTarget);
  std::filesystem::path relative(decoded);
  if (relative.is_absolute()) {
    relative = relative.lexically_relative("/");
  }
  relative = relative.lexically_normal();
  if (StartsWith(relative.string(), "..")) {
    return {};
  }

  const std::filesystem::path full = (root / relative).lexically_normal();
  const auto rootCanonical = std::filesystem::weakly_canonical(root);
  const auto fullCanonical = std::filesystem::weakly_canonical(full);
  if (fullCanonical.empty()) {
    return {};
  }
  const auto rootStr = rootCanonical.string();
  const auto fullStr = fullCanonical.string();
  if (!StartsWith(fullStr, rootStr)) {
    return {};
  }
  return fullCanonical;
}

int ParsePort(const std::string& value) {
  char* end = nullptr;
  const long port = std::strtol(value.c_str(), &end, 10);
  if (end == nullptr || *end != '\0' || port <= 0 || port > 65535) {
    throw std::runtime_error("invalid port: " + value);
  }
  return static_cast<int>(port);
}

}  // namespace

int main(int argc, char** argv) {
  std::filesystem::path root = std::filesystem::current_path();
  int port = 8090;
  std::string host = "127.0.0.1";

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--root" && i + 1 < argc) {
      root = argv[++i];
    } else if (arg == "--host" && i + 1 < argc) {
      host = argv[++i];
    } else if (arg == "--port" && i + 1 < argc) {
      port = ParsePort(argv[++i]);
    } else if (arg == "--help") {
      std::cout << "Usage: " << argv[0] << " [--root <directory>] [--host <ipv4>] [--port <1-65535>]\n";
      return 0;
    } else {
      std::cerr << "Unknown argument: " << arg << '\n';
      return 2;
    }
  }

  std::error_code ec;
  root = std::filesystem::weakly_canonical(root, ec);
  if (ec || !std::filesystem::exists(root) || !std::filesystem::is_directory(root)) {
    std::cerr << "Invalid root directory: " << root << '\n';
    return 2;
  }

  std::signal(SIGINT, HandleSignal);
  std::signal(SIGTERM, HandleSignal);

  const int serverFd = socket(AF_INET, SOCK_STREAM, 0);
  if (serverFd < 0) {
    std::cerr << "socket() failed: " << std::strerror(errno) << '\n';
    return 1;
  }

  int reuse = 1;
  setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  if (host == "0.0.0.0") {
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
  } else {
    if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) != 1) {
      std::cerr << "Invalid IPv4 host: " << host << '\n';
      close(serverFd);
      return 2;
    }
  }
  addr.sin_port = htons(static_cast<uint16_t>(port));

  if (bind(serverFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
    std::cerr << "bind() failed: " << std::strerror(errno) << '\n';
    close(serverFd);
    return 1;
  }
  if (listen(serverFd, 32) < 0) {
    std::cerr << "listen() failed: " << std::strerror(errno) << '\n';
    close(serverFd);
    return 1;
  }

  std::cout << "[INFO] Serving " << root << " on http://" << host << ":" << port << '\n';
  while (!g_stop) {
    sockaddr_in clientAddr{};
    socklen_t clientLen = sizeof(clientAddr);
    const int clientFd = accept(serverFd, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
    if (clientFd < 0) {
      if (errno == EINTR && g_stop) {
        break;
      }
      continue;
    }

    char buffer[8192];
    const ssize_t bytesRead = recv(clientFd, buffer, sizeof(buffer) - 1, 0);
    if (bytesRead <= 0) {
      close(clientFd);
      continue;
    }
    buffer[bytesRead] = '\0';
    const std::string request(buffer);
    const std::size_t lineEnd = request.find("\r\n");
    const std::string firstLine = request.substr(0, lineEnd);
    std::istringstream lineStream(firstLine);
    std::string method;
    std::string target;
    std::string version;
    lineStream >> method >> target >> version;

    if (method != "GET") {
      const std::string msg = "Method Not Allowed\n";
      SendFileResponse(clientFd, 405, "Method Not Allowed", "text/plain; charset=utf-8",
                       std::vector<char>(msg.begin(), msg.end()));
      close(clientFd);
      continue;
    }

    const std::filesystem::path path = ResolvePath(root, target);
    if (path.empty() || !std::filesystem::exists(path) || std::filesystem::is_directory(path)) {
      const std::string msg = "Not Found\n";
      SendFileResponse(clientFd, 404, "Not Found", "text/plain; charset=utf-8",
                       std::vector<char>(msg.begin(), msg.end()));
      close(clientFd);
      continue;
    }

    const std::vector<char> body = ReadFile(path);
    if (body.empty() && std::filesystem::file_size(path, ec) > 0) {
      const std::string msg = "Internal Server Error\n";
      SendFileResponse(clientFd, 500, "Internal Server Error", "text/plain; charset=utf-8",
                       std::vector<char>(msg.begin(), msg.end()));
      close(clientFd);
      continue;
    }

    SendFileResponse(clientFd, 200, "OK", ContentTypeFor(path), body);
    close(clientFd);
  }

  close(serverFd);
  return 0;
}

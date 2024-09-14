#include <fcntl.h>

#include "cold/Io.h"
#include "cold/Log.h"

using namespace Cold;

void SetNonBlocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags < 0) {
    FATAL("fcntl error");
    return;
  }
  if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
    FATAL("fcntl error");
  }
}

Task<> DoEcho(IoContext* context, int sock) {
  SetNonBlocking(sock);
  auto event = context->TakeIoEvent(sock);
  event->EnableReadingET();
  char buf[1024];
  while (true) {
    auto ret =
        co_await detail::ReadAwaitable(event, sock, buf, sizeof(buf), false);
    if (ret < 0) {
      ERROR("read error");
      break;
    }
    if (ret == 0) {
      break;
    }
    ret = co_await detail::WriteAwaitable(event, sock, buf,
                                          static_cast<size_t>(ret), false);
    if (ret < 0) {
      ERROR("write error");
      break;
    }
  }
  event->ReturnIoEvent();
  close(sock);
  co_return;
}

Task<> DoAccept(IoContext* context) {
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    FATAL("socket error");
    co_return;
  }
  SetNonBlocking(sock);
  // reuse addr
  int reuse = 1;
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
    FATAL("setsockopt error");
    co_return;
  }
  sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(8888);
  if (bind(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
    FATAL("bind error");
    co_return;
  }
  if (listen(sock, SOMAXCONN) < 0) {
    FATAL("listen error");
    co_return;
  }

  auto event = context->TakeIoEvent(sock);
  event->EnableReadingET();
  while (true) {
    auto ret = co_await detail::AcceptrAwaitable(event, sock, nullptr, nullptr);
    if (ret < 0) {
      ERROR("accept error");
    } else {
      context->CoSpawn(DoEcho(context, ret));
    }
  }
}

int main() {
  LogManager::GetInstance().GetDefaultRaw()->SetLevel(LogLevel::TRACE);
  IoContext ioContext;
  ioContext.CoSpawn(DoAccept(&ioContext));
  ioContext.Start();
}
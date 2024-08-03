# libcold C++20实现的协程网络库   

## 构建   
```shell
git clone git@github.com:woaichiboluo/libcold.git   
cd libcold
mkdir build
cd build 
cmake .. # cmake .. -DCMAKE_BUILD_TYPE=release
make -j8
```  

## 配置模块  
配置模块通过json实现，使用json-pointer来完成对嵌套json进行查询与设置。   
```c++
Base::Config::GetGloablDefaultConfig().GetOrDefault("/ssl/server-handshake-timeout", 10)));
config.GetOrDefault("/logs/name", std::string("main-logger"));    
```  

## 日志模块  
仿照spdlog基于fmt库实现的日志模块，支持多Logger与多Sink输出,支持颜色日志,支持自定义日志输出格式
```c++
#include "cold/log/LogSinkFactory.h"
#include "cold/log/Logger.h"

using namespace Cold;

int main() {
  auto sink1 = Base::LogSinkFactory::MakeSink("stdoutcolorsink");
  auto sink2 =
      Base::LogSinkFactory::MakeSink<Base::BasicFileSink>("example.log");
  auto logger = Base::LoggerFactory::MakeLogger("example", {sink1, sink2});
  auto logger1 = Base::LoggerFactory::MakeLogger<Base::StdoutSink>("example");
  logger1->SetPattern("%T %L <%N:%t> %c [%b:%l]%n");
  // use main logger
  Base::INFO("info hello world");
  Base::WARN("warn hello world");
  Base::ERROR("error hello world");
  // use logger
  Base::Info(logger, "info hello world");
  Base::Warn(logger, "info hello world");
  Base::Error(logger, "info hello world");
  // use logger1
  Base::Info(logger1, "info hello world");
}
```  

## 协程模块  
利用实现的`IoService`以及封装好的`Task`类，可以快速完成协程的调用。
```c++
#include "cold/coro/IoService.h"

using namespace Cold;

Base::Task<> DoSleep(Base::IoService& service) {
  co_await Base::Sleep(service, std::chrono::seconds(3));
  Base::INFO("Sleep Complete.");
  service.Stop();
}

Base::Task<> DoCounting(int& count) {
  for (int i = 0; i < 1000; ++i) ++count;
  Base::INFO("Counting Complete. count: {}", count);
  co_return;
}

int main() {
  Base::IoService service;
  service.CoSpawn(DoSleep(service));
  int c = 0;
  service.CoSpawn(DoCounting(c));
  service.Start();
}
``` 

## 定时器模块  


## 网络模块   
libcold对Socket进行了封装，对标准IO进行了协程化，在编写Socket应用时，可以用同步编程实现异步编程一样的效果。 
典型的EchoServer,EchoClient实现如下.
```c++
#include <chrono>

#include "cold/net/IpAddress.h"
#include "cold/net/TcpServer.h"

using namespace Cold;

class EchoServer : public Net::TcpServer {
 public:
  EchoServer(const Net::IpAddress& addr, size_t poolSize = 0,
             bool reusePort = false, bool enableSSL = false)
      : Net::TcpServer(addr, poolSize, reusePort, enableSSL) {}
  ~EchoServer() override = default;

  Base::Task<> OnConnect(Net::TcpSocket socket) override {
    co_await DoEcho(std::move(socket));
  }

  Base::Task<> DoEcho(Net::TcpSocket socket) {
    auto timeout = std::chrono::seconds(10);
    while (true) {
      char buf[4096];
      auto n = co_await socket.ReadWithTimeout(buf, sizeof(buf), timeout);
      if (n <= 0) {
        socket.Close();
        break;
      }
      n = co_await socket.WriteNWithTimeout(buf, static_cast<size_t>(n),
                                            timeout);
      if (n < 0) {
        socket.Close();
        break;
      }
    }
  }
};

int main() {
  Net::IpAddress addr(8888);
  EchoServer server(addr);
  Base::INFO("EchoServer run at {}", addr.GetIpPort());
  server.Start();
}
```  
EchoClient   
```c++
#include "cold/coro/Io.h"
#include "cold/net/TcpClient.h"

using namespace Cold;

class EchoClient : public Net::TcpClient {
 public:
  EchoClient(Base::IoService& service) : TcpClient(service) {}
  ~EchoClient() override = default;

  Base::Task<> OnConnect() override {
    Base::INFO("Connect Success. Server address:{}",
               socket_.GetRemoteAddress().GetIpPort());
    Base::AsyncIO io(socket_.GetIoService(), STDIN_FILENO);
    while (true) {
      char buf[4096];
      auto n = co_await io.AsyncRead(buf, sizeof buf);
      if (n <= 0) break;
      assert(n >= 1);
      std::string_view message(buf, static_cast<size_t>(n - 1));
      if (message == "quit") break;
      n = co_await socket_.WriteN(message.data(), message.size());
      if (n < 0) break;
      co_await io.AsyncWrite(buf, static_cast<size_t>(n));
      const char* newline = "\n";
      co_await io.AsyncWrite(newline, 1);
    }
    socket_.Close();
    socket_.GetIoService().Stop();
  }
};

int main() {
  Base::IoService ioService;
  EchoClient client(ioService);
  Net::IpAddress addr(8888);
  ioService.CoSpawn(client.Connect(addr));
  ioService.Start();
}
```

## HTTP模块   
libcold的HTTP模块仿照JavaWeb实现了Servlet,Filter,HttpSession.   
只需使用HttpServer模块就可以快速构建一个Http服务器  
```c++
#include "cold/net/http/HttpServer.h"

using namespace Cold;

using namespace Cold::Net::Http;

int main() {
  Net::IpAddress addr(8080);
  HttpServer server(addr, 4);
  server.AddServlet("/hello", [](HttpRequest& request, HttpResponse& response) {
    auto body = MakeTextBody();
    body->SetContent("<h1>Hello World</h1>");
    response.SetBody(std::move(body));
  });
  server.AddServlet("/hello1",
                    [](HttpRequest& request, HttpResponse& response) {
                      response.SendRedirect("/hello");
                    });
  Base::INFO("example1:basic servlet usage. Run at port: 8080");
  server.Start();
}
```   
只需要在构造HttpServer时，指定enableSSL为true，即可开启HTTPS    
```c++
#include "cold/net/http/HttpServer.h"
#include "cold/net/ssl/SSLContext.h"

using namespace Cold;

using namespace Cold::Net::Http;

int main(int argc, char** argv) {
  if (argc < 3) {
    fmt::print("Usage: {} <cert> <key>\n", argv[0]);
    return 0;
  }
  Net::IpAddress addr(8080);
  Net::SSLContext::GetInstance().LoadCert(argv[1], argv[2]);
  HttpServer server(addr, 4, false, true);
  Base::INFO("example4:basic https usage. Run at port: 8080");
  server.Start();
}
```  
也可以和经典的JavaWeb一样,利用Session来实现登录等操作。  
详细见 [http/example3](./examples/http/example3.cpp)  

## RPC模块  
基于Protobuf实现了RPC模块， 
对于RpcProvider，只需要实现对应的Service，将Service添加到RpcServer中即可。  
```c++
#include "cold/log/Logger.h"
#include "cold/net/rpc/RpcServer.h"
#include "echo.pb.h"

class EchoServiceImpl : public Echo::EchoService {
 public:
  EchoServiceImpl() = default;
  ~EchoServiceImpl() override = default;
  void DoEcho(::google::protobuf::RpcController* controller,
              const ::Echo::EchoRequest* request,
              ::Echo::EchoResponse* response,
              ::google::protobuf::Closure* done) override {
    Cold::Base::INFO("Call method DoEcho :{}", request->data());
    response->set_data(request->data());
    done->Run();
  }
};

int main() {
  Cold::Net::IpAddress addr(8888);
  Cold::Net::Rpc::RpcServer server(addr);
  EchoServiceImpl service;
  server.AddService(&service);
  server.Start();
}
```   
对于RpcClient，支持对Stub调用的协程化，可以以同步代码形式完成Rpc调用。   
```c++
#include "cold/net/IpAddress.h"
#include "cold/net/rpc/RpcClient.h"
#include "echo.pb.h"

using namespace Cold;

class EchoRpcClient : public Net::Rpc::RpcClient<Echo::EchoService::Stub> {
 public:
  EchoRpcClient(Base::IoService& service, bool enableSSL = false,
                bool ipv6 = false)
      : RpcClient(service, enableSSL, ipv6) {}
  ~EchoRpcClient() override = default;

  Base::Task<> OnConnect() override {
    Base::INFO("Connect Success. Server address:{}",
               GetSocket().GetRemoteAddress().GetIpPort());
    co_await DoEchoRpc();
  }

  Base::Task<> DoEchoRpc() {
    Base::AsyncIO io(socket_.GetIoService(), STDIN_FILENO);
    socket_.GetIoService().CoSpawn(channel_.DoRpc());
    while (true) {
      char buf[4096];
      auto n = co_await io.AsyncRead(buf, sizeof buf);
      if (n <= 0) break;
      assert(n >= 1);
      std::string_view message(buf, static_cast<size_t>(n - 1));
      if (message == "quit") break;
      Echo::EchoRequest request;
      request.set_data(message);
      auto response = std::make_unique<Echo::EchoResponse>();
      co_await StubCall(&Echo::EchoService::Stub::DoEcho, nullptr, &request,
                        response.get());
      Base::INFO("{}", response->data());
    }
    socket_.Close();
    socket_.GetIoService().Stop();
    co_return;
  }
};

int main() {
  Base::IoService service;
  EchoRpcClient client(service);
  service.CoSpawn(client.Connect(Net::IpAddress(8888)));
  service.Start();
}
```  
## SSL模块  

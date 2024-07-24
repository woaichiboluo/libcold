
#include "cold/net/http/HttpServer.h"

using namespace Cold;

using namespace Cold::Net::Http;

struct User {
  std::string username;
  std::string password;
};

class LoginFilter : public HttpFilter {
 public:
  bool DoFilter(HttpRequest& request, HttpResponse& response) {
    auto session = request.GetSession();
    auto user = session->GetAttribute<User>("user");
    if (user == nullptr) {
      request.SetAttribute("message", "您还没有登录");
      request.GetServletContext()->ForwardTo("/loginpage", request, response);
      return false;
    }
    return true;
  }
};

class DefaultPageServlet : public HttpServlet {
 public:
  void Handle(HttpRequest& request, HttpResponse& response) {
    auto session = request.GetSession();
    auto body = MakeTextBody();
    auto user = session->GetAttribute<User>("user");
    response.SetHeader("Content-type", "text/html;charset=utf-8");
    if (user) {
      body->Append(fmt::format("{},您已登录<br>", user->username));
      body->Append("<a href = '/logout'>点击登出</a>");
    } else {
      body->Append("您还没登录<br>");
      body->Append("<a href = '/loginpage'>点击登录</a>");
    }
    response.SetBody(std::move(body));
  }
};

class LoginPageServlet : public HttpServlet {
 public:
  void Handle(HttpRequest& request, HttpResponse& response) {
    if (request.GetSession()->GetAttribute<User>("user") != nullptr) {
      response.SendRedirect("/user/home");
      return;
    }
    auto body = MakeTextBody();
    response.SetHeader("Content-type", "text/html;charset=utf-8");
    if (request.HasAttribute("message")) {
      body->Append(fmt::format("<h2 style='color:red;'>{}</h2>",
                               *request.GetAttribute<std::string>("message")));
    }
    body->Append(
        R"(
        <form method="post" action="/login">
            username: <input type="text" name="username" placeholder="用户名"><br>
            password: <input type="password" name="password" placeholder="登录密码"><br>
            <input type="submit" name="submit" value="登录" class="btn">
        </form>)");
    response.SetBody(std::move(body));
  }
};

class LoginServlet : public HttpServlet {
 public:
  void Handle(HttpRequest& request, HttpResponse& response) {
    auto session = request.GetSession();
    if (session->GetAttribute<User>("user")) {
      response.SendRedirect("/");
      return;
    }
    auto username = request.GetParameter("username");
    auto password = request.GetParameter("password");
    fmt::println("username:{}", username);
    fmt::println("password:{}", password);
    if (username == "admin" && password == "admin") {
      User u{"admin", "admin"};
      session->SetAttribute("user", u);
      response.SendRedirect("/user/home");
    } else {
      request.SetAttribute("message", "账户或者密码错误");
      request.GetServletContext()->ForwardTo("/loginpage", request, response);
    }
  }
};

class UserHomePageServlet : public HttpServlet {
 public:
  void Handle(HttpRequest& request, HttpResponse& response) {
    auto session = request.GetSession();
    auto user = session->GetAttribute<User>("user");
    auto body = MakeTextBody();
    response.SetHeader("Content-type", "text/html;charset=utf-8");
    body->Append("<h1>/user/home</h1>");
    body->Append(fmt::format("{},您已登录<br>", user->username));
    body->Append("<a href = '/logout'>点击登出</a>");
    response.SetBody(std::move(body));
  }
};

int main() {
  Net::IpAddress addr(8080);
  HttpServer server(addr, 4);
  server.AddServlet("/", std::make_unique<DefaultPageServlet>());
  server.AddServlet("/loginpage", std::make_unique<LoginPageServlet>());
  server.AddServlet("/login", std::make_unique<LoginServlet>());
  server.AddServlet("/user/home", std::make_unique<UserHomePageServlet>());
  server.AddFilter("/user/**", std::make_unique<LoginFilter>());
  server.AddServlet("/logout",
                    [](HttpRequest& request, HttpResponse& response) {
                      request.GetSession()->Invalidate();
                      response.SendRedirect("/");
                    });
  Base::INFO("example3:basic login demo. Run at port: 8080");
  server.Start();
}
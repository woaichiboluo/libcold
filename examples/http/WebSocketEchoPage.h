#ifndef EXAMPLES_HTTP_WEBSOCKETECHOPAGE
#define EXAMPLES_HTTP_WEBSOCKETECHOPAGE

constexpr const char* kEchoPage = R"(
<!DOCTYPE html><html>
<head>
<title> WebSocket Echo</title>
</head>
<body>
    <input type = 'text' id = 'input' />
    <button onclick = 'send()'>Send</button> <br/>
    <div id = 'container'></div>
</body>
<script>
    var ws = new WebSocket('ws://localhost:8080');
    ws.onopen = function() {
        // add message to container
        var div = document.createElement('div');
        div.innerHTML = 'Connected';
        document.getElementById('container').appendChild(div);
    };

    ws.onmessage = function(evt) {
        // add message to container
        var div = document.createElement('div');
        div.innerHTML = evt.data;
        document.getElementById('container').appendChild(div);
    };
    function send() {
        var msg = document.getElementById('input').value;
        ws.send(msg);
    }
</script>
</html> )";

#endif /* EXAMPLES_HTTP_WEBSOCKETECHOPAGE */

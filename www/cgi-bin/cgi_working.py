#!/usr/bin/env python3

# diagnostic CGI script - doesn't do anything, just displays what the server
# passed to the CGI process: method, query string, environment variables.
# proves the full CGI tunnel works (server -> fork -> execve -> stdout -> response)

import os
import datetime

# CGI environment variables set by the server before execve()
method = os.environ.get("REQUEST_METHOD", "unknown")
query = os.environ.get("QUERY_STRING", "")
proto = os.environ.get("SERVER_PROTOCOL", "unknown")
now = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")

# parse query string into key=value pairs (ex: ?name=eval&foo=bar)
params = {}
if query:
    for pair in query.split("&"):
        if "=" in pair:
            k, v = pair.split("=", 1)
            params[k] = v

# CGI output: headers first, blank line, then body - server reads this from stdout
print("Content-Type: text/html")
print()

print("""<!DOCTYPE html>
<html>
<head>
    <title>CGI Demo</title>

    <link rel="stylesheet" href="/css/webserv.css">
    <link rel="stylesheet" href="/css/css-pokemon-gameboy.css">

</head>
<body>

<div class="container">

    <h1>CGI is Working!</h1>

""")

print(f"""
    <div class="panel">
        <h2>Request Info</h2>
        <ul>
            <li><strong>Method:</strong> {method}</li>
            <li><strong>Protocol:</strong> {proto}</li>
            <li><strong>Time:</strong> {now}</li>
        </ul>
    </div>
""")

# display query parameters if any were sent (ex: /cgi-bin/cgi_working.py?name=eval)
if params:
    print("""
    <div class="panel">
        <h2>Query Parameters</h2>
        <ul>
""")

    for k, v in params.items():
        print(f"            <li><strong>{k}</strong>: {v}</li>")

    print("""
        </ul>
    </div>
""")

# display key CGI environment variables - set by the server before execve()
print("""
    <div class="panel">
        <h2>Environment Variables</h2>
        <ul>
""")

cgi_vars = [
    "REQUEST_METHOD",
    "QUERY_STRING",
    "CONTENT_TYPE",
    "CONTENT_LENGTH",
    "SCRIPT_NAME",
    "REQUEST_URI",
    "SERVER_PROTOCOL"
]

for var in cgi_vars:
    val = os.environ.get(var, "<not set>")
    print(f"            <li><strong>{var}</strong>: {val}</li>")

print("""
        </ul>
    </div>

    <div class="panel">
        <a class="button" href="/">Back to Home</a>
    </div>

</div>

</body>
</html>
""")

#!/usr/bin/env python3
import os
import datetime

method = os.environ.get("REQUEST_METHOD", "unknown")
query  = os.environ.get("QUERY_STRING", "")
proto  = os.environ.get("SERVER_PROTOCOL", "unknown")
now    = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")

# parse query string into key=value pairs
params = {}
if query:
    for pair in query.split("&"):
        if "=" in pair:
            k, v = pair.split("=", 1)
            params[k] = v

print("Content-Type: text/html")
print("")
print("""<!DOCTYPE html>
<html>
<head>
    <title>CGI Demo</title>
    <style>
        body {
            font-family: 'Segoe UI', sans-serif;
            background: #1a1a2e;
            color: #eee;
            display: flex;
            flex-direction: column;
            align-items: center;
            padding: 40px;
        }
        h1 { color: #e94560; }
        .card {
            background: #16213e;
            border-radius: 12px;
            padding: 24px;
            margin: 16px 0;
            width: 500px;
            box-shadow: 0 4px 20px rgba(0,0,0,0.4);
        }
        .card h2 { color: #0f3460; margin: 0 0 12px 0; color: #e94560; }
        table { width: 100%; border-collapse: collapse; }
        td { padding: 6px 10px; border-bottom: 1px solid #0f3460; }
        td:first-child { color: #e94560; font-weight: bold; width: 40%; }
        .badge {
            display: inline-block;
            background: #e94560;
            color: white;
            border-radius: 6px;
            padding: 2px 10px;
            font-size: 0.85em;
        }
    </style>
</head>
<body>
    <h1>CGI is working!</h1>
""")

print(f"""
    <div class="card">
        <h2>Request Info</h2>
        <table>
            <tr><td>Method</td><td><span class="badge">{method}</span></td></tr>
            <tr><td>Protocol</td><td>{proto}</td></tr>
            <tr><td>Time</td><td>{now}</td></tr>
        </table>
    </div>
""")

if params:
    print("""    <div class="card">
        <h2>Query Parameters</h2>
        <table>""")
    for k, v in params.items():
        print(f"            <tr><td>{k}</td><td>{v}</td></tr>")
    print("""        </table>
    </div>""")

print("""    <div class="card">
        <h2>Environment</h2>
        <table>""")

cgi_vars = ["REQUEST_METHOD", "QUERY_STRING", "CONTENT_TYPE",
            "CONTENT_LENGTH", "SCRIPT_NAME", "REQUEST_URI", "SERVER_PROTOCOL"]
for var in cgi_vars:
    val = os.environ.get(var, "<not set>")
    print(f"            <tr><td>{var}</td><td>{val}</td></tr>")

print("""        </table>
    </div>
</body>
</html>""")

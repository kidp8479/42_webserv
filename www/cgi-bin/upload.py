#!/usr/bin/env python3
import sys
import os

# use the local cgi.py (copied from Python 3.11 stdlib)
# cgi module was removed from stdlib in Python 3.13
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import cgi

# absolute path to upload directory - required because CGI runs in its own directory
UPLOAD_DIR = os.path.join(
    os.path.dirname(os.path.abspath(__file__)),
    "../../www/demo/upload/received"
)

# CGI responses are written to stdout: headers, blank line, then body
# the server reads this and builds the HTTP response
def send_response(status, message):
    body = """<!DOCTYPE html>
<html>
<head>
    <title>webserv - upload result</title>
    <link rel="stylesheet" href="/css/css-pokemon-gameboy.css">
    <link rel="stylesheet" href="/css/webserv.css">
</head>
<body>
    <div class="framed primary">
        <h2>webserv - upload result</h2>
        <p class="message">{}</p>
        <div><a href="/" class="button">Back to upload</a></div>
    </div>
</body>
</html>""".format(message)
    sys.stdout.write("Status: {}\r\n".format(status))
    sys.stdout.write("Content-Type: text/html\r\n")
    sys.stdout.write("\r\n")
    sys.stdout.write(body)

# parse the multipart/form-data body from stdin using CGI env vars (CONTENT_TYPE, CONTENT_LENGTH)
form = cgi.FieldStorage()

# look for the field named "file" - must match name="file" in the HTML <input>
file_field = form["file"] if "file" in form else None

# validate: field must exist and have a filename
if file_field is None or not file_field.filename:
    send_response("400 Bad Request", "No file provided.")
    sys.exit(0)

# basename strips any path prefix from the filename (prevents path traversal attacks)
filename = os.path.basename(file_field.filename)
if not filename:
    send_response("400 Bad Request", "Invalid filename.")
    sys.exit(0)

dest = os.path.join(UPLOAD_DIR, filename)

# write file to disk in binary mode, catch any filesystem error
try:
    with open(dest, "wb") as f:
        f.write(file_field.file.read())
    send_response("201 Created", "File '{}' uploaded successfully.".format(filename))
except Exception as e:
    send_response("500 Internal Server Error", "Could not save file: {}".format(str(e)))

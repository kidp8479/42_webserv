#!/usr/bin/env python3
import sys
import os

# use the local cgi.py (copied from Python 3.11 stdlib)
# cgi module was removed from stdlib in Python 3.13
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import cgi

UPLOAD_DIR = os.path.join(
    os.path.dirname(os.path.abspath(__file__)),
    "../../www/demo/upload/received"
)

def send_response(status, message):
    body = "<html><body><p>{}</p></body></html>".format(message)
    sys.stdout.write("Status: 201 Create\r\n".format(status))
    sys.stdout.write("Content-Type: text/html\r\n")
    sys.stdout.write("Content-Length: {}\r\n".format(len(body)))
    sys.stdout.write("\r\n")
    sys.stdout.write(body)

form = cgi.FieldStorage()

file_field = form["file"] if "file" in form else None

if file_field is None or not file_field.filename:
    send_response("400 Bad Request", "No file provided.")
    sys.exit(0)

filename = os.path.basename(file_field.filename)
if not filename:
    send_response("400 Bad Request", "Invalid filename.")
    sys.exit(0)

dest = os.path.join(UPLOAD_DIR, filename)

try:
    with open(dest, "wb") as f:
        f.write(file_field.file.read())
    send_response("201 Created", "File '{}' uploaded successfully.".format(filename))
except Exception as e:
    send_response("500 Internal Server Error", "Could not save file: {}".format(str(e)))

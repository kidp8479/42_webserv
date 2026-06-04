# curl http://localhost:8087/cgi-bin/hello.py
#!/usr/bin/env python3

print("Content-Type: text/plain\r")
print("\r")
print("hello world")

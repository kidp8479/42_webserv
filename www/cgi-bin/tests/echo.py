# curl -X POST http://localhost:8087/cgi-bin/echo.py -d "hello cgi"
#!/usr/bin/env python3

import sys

body = sys.stdin.read()

print("Content-Type: text/plain")
print()
print(body)

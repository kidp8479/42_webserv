# http://localhost:8087/cgi-bin/headers.py
# curl -H "X-Test: hello" http://localhost:8087/cgi-bin/headers.py
import os

print("Content-Type: text/plain")
print()

for k, v in os.environ.items():
    if k.startswith("HTTP_"):
        print(k, v)

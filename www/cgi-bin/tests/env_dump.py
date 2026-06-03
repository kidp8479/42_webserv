# http://localhost:8087/cgi-bin/env_dump.py
# curl -v http://localhost:8087/cgi-bin/env_dump.py
import os

print("Content-Type: text/plain")
print()

for k, v in os.environ.items():
    if k.startswith("REQUEST") or k.startswith("HTTP") or k.startswith("SERVER"):
        print(k, "=", v)

# curl http://localhost:8087/cgi-bin/slow.py
#!/usr/bin/env python3
import time

time.sleep(3)

print("Content-Type: text/plain\n")
print("SLOW DONE")

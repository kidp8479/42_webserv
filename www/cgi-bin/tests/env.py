
# http://localhost:8087/cgi-bin/env.py?name=alice
# curl -v http://localhost:8087/cgi-bin/env.py?name=alice

import os

print("Content-Type: text/plain")
print()

print(os.environ.get("QUERY_STRING"))

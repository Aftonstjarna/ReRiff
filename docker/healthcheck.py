import json
import urllib.request

opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))
with opener.open('http://127.0.0.1:5144/', timeout=2) as response:
    if response.status != 200 or json.load(response).get('server') != 'RiffApiLocal':
        raise SystemExit(1)

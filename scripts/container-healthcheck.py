"""Check the local API without following proxy settings."""
import json
import urllib.request

opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))
with opener.open('http://127.0.0.1:5144/classes/Version', timeout=2) as response:
    if response.status != 200 or not isinstance(json.load(response).get('results'), list):
        raise SystemExit(1)

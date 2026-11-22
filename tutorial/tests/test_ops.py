"""Tests for Parse __op operations."""
import json
import urllib.request
import urllib.error

BASE = "http://127.0.0.1:5144"
HEADERS = {"X-Parse-Application-Id": "T", "Content-Type": "application/json"}


def req(method, path, body=None):
    data = json.dumps(body).encode() if body is not None else None
    r = urllib.request.Request(BASE + path, data=data, headers=HEADERS, method=method)
    try:
        with urllib.request.urlopen(r) as resp:
            return resp.status, resp.read()
    except urllib.error.HTTPError as e:
        return e.code, e.read()


ok = True

# create object with counters
st, body = req("POST", "/classes/Track", body={"Title": "OpTest", "Plays": 10, "Finishes": 1, "Tags": ["a"]})
oid = json.loads(body)["objectId"]

# Increment
st, body = req("PUT", "/classes/Track/" + oid, body={"Plays": {"__op": "Increment", "amount": 5}})
assert st == 200, body
st, body = req("GET", "/classes/Track/" + oid)
o = json.loads(body)
print("Increment:", o["Plays"])
ok &= o["Plays"] == 15

# Increment default amount
req("PUT", "/classes/Track/" + oid, body={"Finishes": {"__op": "Increment"}})
st, body = req("GET", "/classes/Track/" + oid)
print("Increment default:", json.loads(body)["Finishes"])
ok &= json.loads(body)["Finishes"] == 2

# Add / AddUnique / Remove on array
req("PUT", "/classes/Track/" + oid, body={"Tags": {"__op": "Add", "objects": ["b"]}})
req("PUT", "/classes/Track/" + oid, body={"Tags": {"__op": "AddUnique", "objects": ["b", "c"]}})
st, body = req("GET", "/classes/Track/" + oid)
tags = json.loads(body)["Tags"]
print("Array ops:", tags)
ok &= tags == ["a", "b", "c"]
req("PUT", "/classes/Track/" + oid, body={"Tags": {"__op": "Remove", "objects": ["a"]}})
st, body = req("GET", "/classes/Track/" + oid)
print("Remove:", json.loads(body)["Tags"])
ok &= json.loads(body)["Tags"] == ["b", "c"]

# Delete op removes field
req("PUT", "/classes/Track/" + oid, body={"Tags": {"__op": "Delete"}})
st, body = req("GET", "/classes/Track/" + oid)
o = json.loads(body)
print("Delete op:", "Tags" in o)
ok &= "Tags" not in o

# cleanup
req("DELETE", "/classes/Track/" + oid)

print("OPS TESTS:", "ALL PASS" if ok else "FAILED")
raise SystemExit(0 if ok else 1)
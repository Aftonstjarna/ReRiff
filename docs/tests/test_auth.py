"""Tests for auth endpoints."""
import json
import urllib.request
import urllib.error

BASE = "http://127.0.0.1:5144"

ok = True


def get(path, headers=None):
    r = urllib.request.Request(BASE + path, headers=headers or {}, method="GET")
    try:
        with urllib.request.urlopen(r) as resp:
            return resp.status, json.loads(resp.read())
    except urllib.error.HTTPError as e:
        return e.code, e.read()


# First login creates user
st, u1 = get("/login?username=TESTUSER1&password=PW1")
print("login1:", st, u1.get("objectId"), u1.get("sessionToken", "")[:8])
ok &= st == 200 and "objectId" in u1 and "sessionToken" in u1 and u1["username"] == "TESTUSER1"
id1 = u1["objectId"]

# Second login same credentials -> same user
st, u2 = get("/login?username=TESTUSER1&password=PW1")
print("login2:", st, u2.get("objectId"))
ok &= st == 200 and u2["objectId"] == id1

# Different password, same username -> still same local user
st, u3 = get("/login?username=TESTUSER1&password=OTHER")
ok &= st == 200 and u3["objectId"] == id1

# Second account -> different user
st, u4 = get("/login?username=TESTUSER2&password=PW2")
print("login3:", st, u4.get("objectId"))
ok &= st == 200 and u4["objectId"] != id1

# users/me with session token
st, me = get("/users/me", {"X-Parse-Session-Token": u4["sessionToken"]})
print("users/me:", st, me.get("username"))
ok &= st == 200 and me["objectId"] == u4["objectId"]

# users/me with bad token -> 404
st, _ = get("/users/me", {"X-Parse-Session-Token": "r:bad"})
ok &= st == 404

# cleanup: delete test users via classes API
for uid in (id1, u4["objectId"]):
    r = urllib.request.Request(BASE + "/classes/_User/" + uid, method="DELETE")
    urllib.request.urlopen(r)

print("AUTH TESTS:", "ALL PASS" if ok else "FAILED")
raise SystemExit(0 if ok else 1)
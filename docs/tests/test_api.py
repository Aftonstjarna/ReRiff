"""End-to-end tests for the extended riffapi-local server (Parse REST emulation)."""
import json
import os
import urllib.request
import urllib.error
import urllib.parse

BASE = os.environ.get("RIFFAPI_TEST_URL", "http://127.0.0.1:5144")
HEADERS = {
    "X-Parse-Application-Id": "TEST_APP_ID",
    "X-Parse-Windows-Key": "TEST_WINDOWS_KEY",
    "Content-Type": "application/json",
}

passed = 0
failed = 0


def req(method, path, body=None, raw=None):
    url = BASE + path
    data = None
    hdrs = dict(HEADERS)
    if raw is not None:
        data = raw
        hdrs["Content-Type"] = "application/octet-stream"
    elif body is not None:
        data = json.dumps(body).encode("utf-8")
    r = urllib.request.Request(url, data=data, headers=hdrs, method=method)
    try:
        with urllib.request.urlopen(r) as resp:
            return resp.status, resp.read()
    except urllib.error.HTTPError as e:
        return e.code, e.read()


def check(name, cond, extra=""):
    global passed, failed
    if cond:
        passed += 1
        print("PASS: %s" % name)
    else:
        failed += 1
        print("FAIL: %s %s" % (name, extra))


# 1. Canned endpoints still work
st, body = req("GET", "/classes/Configuration")
check("canned Configuration", st == 200 and b"OnTrackCoinValue" in body)
st, body = req("GET", "/classes/News")
check("ReRiff News status", st == 200 and b"ReRiffServerStatus" in body and b"RiffApiLocalNews" not in body)
st, body = req("GET", "/classes/Version")
check("canned Version", st == 200 and b'"Ver":100' in body)
st, body = req("GET", "/events/AppOpened")
check("AppOpened", st == 200 and body == b"{}")

# 2. File upload (like a ghost binary)
ghost_bytes = bytes(range(256)) * 40  # 10240 bytes of binary
st, body = req("POST", "/files/17SJWbBsrv-1234567890.ghost", raw=ghost_bytes)
up = json.loads(body)
check("file upload status 201", st == 201, str(st))
check("file upload has name+url", "url" in up and "name" in up, str(up))
check("file upload name preserved", up.get("name") == "17SJWbBsrv-1234567890.ghost", str(up))

# 3. File download round-trip
st, body = req("GET", "/files/" + up["name"])
check("file download status 200", st == 200, str(st))
check("file download bytes match", body == ghost_bytes, "%d vs %d bytes" % (len(body), len(ghost_bytes)))

# 4. Create Ghost object referencing the file (like the game does)
ghost_obj = {
    "Score": 123456,
    "PlayerAlias": "TestPlayer",
    "PlayerID": "76561198000000001",
    "UploaderID": "76561198000000001",
    "Uploader": "TestPlayer",
    "SongID": "17SJWbBsrv",
    "Mode": 0,
    "Pack": 0,
    "GhostData": {"__type": "File", "name": up["name"], "url": up["url"]},
}
st, body = req("POST", "/classes/Ghost", body=ghost_obj)
created = json.loads(body)
check("create Ghost", st == 200 and "objectId" in created and "createdAt" in created, body.decode())
gid = created.get("objectId")

# 5. Query Ghost with where + order + limit
qpath = '/classes/Ghost?where=%s&order=-Score&limit=10' % urllib.parse.quote(json.dumps({"SongID": "17SJWbBsrv"}))
st, body = req("GET", qpath)
q = json.loads(body)
check("query Ghost by SongID", st == 200 and len(q.get("results", [])) == 1, body.decode()[:200])
if q.get("results"):
    r0 = q["results"][0]
    check("query returns fields", r0.get("Score") == 123456 and r0.get("PlayerAlias") == "TestPlayer")
    check("query returns file ref", isinstance(r0.get("GhostData"), dict) and r0["GhostData"].get("url") == up["url"])

# 6. Insert more ghosts, verify ordering
for score in (999, 500000, 42):
    req("POST", "/classes/Ghost", body=dict(ghost_obj, Score=score))
st, body = req("GET", '/classes/Ghost?where=%s&order=-Score&limit=2' % urllib.parse.quote(json.dumps({"SongID": "17SJWbBsrv"})))
q = json.loads(body)
scores = [r["Score"] for r in q.get("results", [])]
check("order -Score desc + limit", scores == [500000, 123456], str(scores))

st, body = req("GET", '/classes/Ghost?where=%s&order=Score&limit=100' % urllib.parse.quote(json.dumps({"SongID": "17SJWbBsrv"})))
q = json.loads(body)
scores = [r["Score"] for r in q.get("results", [])]
check("order Score asc", scores == sorted(scores), str(scores))

# 7. $in query (e.g. friends' ghosts)
st, body = req("GET", '/classes/Ghost?where=%s' % urllib.parse.quote(json.dumps({"PlayerID": {"$in": ["76561198000000001", "other"]}})))
q = json.loads(body)
check("$in query", st == 200 and len(q.get("results", [])) == 4, str(len(q.get("results", []))))

# 8. count
st, body = req("GET", '/classes/Ghost?where=%s&count=1&limit=0' % urllib.parse.quote(json.dumps({"SongID": "17SJWbBsrv"})))
q = json.loads(body)
check("count=1", q.get("count") == 4 and q.get("results") == [], str(q))

# 9. GET single object
st, body = req("GET", "/classes/Ghost/" + gid)
single = json.loads(body)
check("get single object", st == 200 and single.get("objectId") == gid)

# 10. PUT update
st, body = req("PUT", "/classes/Ghost/" + gid, body={"Score": 777777})
check("update object", st == 200 and "updatedAt" in json.loads(body), body.decode())
st, body = req("GET", "/classes/Ghost/" + gid)
check("update persisted", json.loads(body).get("Score") == 777777)

# 11. Batch
batch_body = {"requests": [
    {"method": "POST", "path": "/classes/Ghost", "body": {"Score": 1, "SongID": "batchtest"}},
    {"method": "PUT", "path": "/classes/Ghost/" + gid, "body": {"PlayerAlias": "Updated"}},
]}
st, body = req("POST", "/batch", body=batch_body)
bres = json.loads(body)
check("batch 2 ops", st == 200 and len(bres) == 2 and "success" in bres[0] and "success" in bres[1], body.decode()[:300])
st, body = req("GET", "/classes/Ghost/" + gid)
check("batch update applied", json.loads(body).get("PlayerAlias") == "Updated")

# 12. DELETE object
st, body = req("DELETE", "/classes/Ghost/" + gid)
check("delete object", st == 200)
st, body = req("GET", "/classes/Ghost/" + gid)
check("deleted object 404", st == 404)

# 13. DELETE file
st, body = req("DELETE", "/files/" + up["name"])
check("delete file", st == 200)
st, body = req("GET", "/files/" + up["name"])
check("deleted file 404", st == 404)

# 14. Empty query on unknown class
st, body = req("GET", "/classes/Nonexistent")
check("empty results for unknown class", st == 200 and json.loads(body) == {"results": []})

# 15. Profile_Steam fallback (original behavior)
st, body = req("GET", "/classes/Profile_Steam")
check("Profile_Steam fallback", st == 200 and b"RiffApiLocalProfile" in body)

print()
print("PASSED: %d, FAILED: %d" % (passed, failed))
if failed:
    raise SystemExit(1)
"""Patch 2: adds Parse auth endpoints (/login, /users/me, /logout),
normalizes __op ops on create, and upserts _Installation."""
import io

PATH = r"C:\Games\Riff Racer\riffapi-local\riffapi-local.py"

with io.open(PATH, "r", encoding="utf-8") as f:
    src = f.read()

count = 0

# ---------------------------------------------------------------------------
# 1. Auth endpoints after AppOpened
# ---------------------------------------------------------------------------
anchor_appopened = '''@app.route("/events/AppOpened", methods=["GET", "POST"])
def AppOpened():
    return "{}"
'''

auth_routes = '''

# ---------------------------------------------------------------------------
# Auth API (the game logs in with credentials derived from its Steam ID)
# ---------------------------------------------------------------------------

def gen_session_token():
    alphabet = string.ascii_letters + string.digits
    return "r:" + "".join(random.choice(alphabet) for _ in range(28))


@app.route("/login", methods=["GET"])
def parse_login():
    username = request.args.get("username", "")
    password = request.args.get("password", "")
    ts = now_iso()
    with STORAGE_LOCK:
        user = None
        for o in load_all_objects("_User"):
            if o.get("username") == username:
                user = o
                break
        if user is None:
            # Local server: auto-register unknown users (per Steam account).
            object_id = gen_object_id()
            while os.path.isfile(object_path("_User", object_id)):
                object_id = gen_object_id()
            user = {
                "objectId": object_id,
                "createdAt": ts,
                "updatedAt": ts,
                "username": username,
                "password": password,
            }
            log_line("  created user %r -> %s" % (username, object_id))
        user["sessionToken"] = gen_session_token()
        user["updatedAt"] = ts
        store_object("_User", user)
    resp = dict(user)
    resp.pop("password", None)  # never echo the password back
    log_line("  login ok for %r (session %s)" % (username, resp.get("sessionToken")))
    return jsonify(resp)


@app.route("/users/me", methods=["GET"])
def users_me():
    token = request.headers.get("X-Parse-Session-Token")
    if token:
        with STORAGE_LOCK:
            for o in load_all_objects("_User"):
                if o.get("sessionToken") == token:
                    resp = dict(o)
                    resp.pop("password", None)
                    return jsonify(resp)
    return error_response(404, 101, "Session not found")


@app.route("/logout", methods=["POST"])
def logout():
    return jsonify({})
'''

if '"/login"' not in src:
    assert anchor_appopened in src, "AppOpened anchor not found"
    src = src.replace(anchor_appopened, anchor_appopened + auth_routes, 1)
    count += 1

# ---------------------------------------------------------------------------
# 2. Normalize __op operations on create + upsert _Installation
# ---------------------------------------------------------------------------
old_create = '''    ts = now_iso()
    with STORAGE_LOCK:
        object_id = gen_object_id()
        while os.path.isfile(object_path(cls, object_id)):
            object_id = gen_object_id()
        obj = dict(body)
        obj["objectId"] = object_id'''
new_create = '''    ts = now_iso()
    with STORAGE_LOCK:
        # Upsert installations keyed by their installationId (Parse semantics).
        if cls == "_Installation" and isinstance(body.get("installationId"), str):
            for existing in load_all_objects(cls):
                if existing.get("installationId") == body["installationId"]:
                    merge_update(existing, body)
                    existing["updatedAt"] = ts
                    store_object(cls, existing)
                    return jsonify({"objectId": existing["objectId"], "createdAt": existing["createdAt"]})
        object_id = gen_object_id()
        while os.path.isfile(object_path(cls, object_id)):
            object_id = gen_object_id()
        obj = {}
        merge_update(obj, body)
        obj["objectId"] = object_id'''

if old_create in src:
    src = src.replace(old_create, new_create, 1)
    count += 1

old_batch_create = '''                        obj = dict(inner_body)
                        obj["objectId"] = new_id'''
new_batch_create = '''                        obj = {}
                        merge_update(obj, inner_body)
                        obj["objectId"] = new_id'''

if old_batch_create in src:
    src = src.replace(old_batch_create, new_batch_create, 1)
    count += 1

with io.open(PATH, "w", encoding="utf-8", newline="") as f:
    f.write(src)

print("applied patches:", count)
assert count == 3, "expected 3 patches, got %d" % count
print("OK")
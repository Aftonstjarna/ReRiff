"""Patch 3: adds the web admin GUI endpoints to riffapi-local.py.

Run from the folder that contains riffapi-local.py. This is the exact
transformation that turned the working game server into a manageable one:
a /admin page plus JSON endpoints for browsing classes/objects/files/log.

Idempotent-ish: skips parts that are already applied.
"""
import io

PATH = "riffapi-local.py"
src = io.open(PATH, encoding="utf-8").read()
count = 0

# 1) flask import gains send_from_directory
old_imp = "from flask import Flask, Response, jsonify, request"
new_imp = "from flask import Flask, Response, jsonify, request, send_from_directory"
if old_imp in src and "send_from_directory" not in src:
    src = src.replace(old_imp, new_imp, 1)
    count += 1

# 2) admin routes inserted right before the __main__ block
anchor = 'if __name__ == "__main__":\n    ensure_dirs()'
assert anchor in src, "main anchor not found"

admin_routes = """
# ---------------------------------------------------------------------------
# Admin GUI (web interface for managing stored data)
# ---------------------------------------------------------------------------

ADMIN_HTML = os.path.join(BASE_DIR, "admin.html")


@app.route("/admin")
@app.route("/admin/")
def admin_ui():
    return send_from_directory(BASE_DIR, "admin.html")


def _class_counts():
    counts = {}
    if os.path.isdir(CLASSES_DIR):
        for d in os.listdir(CLASSES_DIR):
            p = os.path.join(CLASSES_DIR, d)
            if os.path.isdir(p):
                counts[d] = len([f for f in os.listdir(p) if f.endswith(".json")])
    return counts


@app.route("/admin/api/overview")
def admin_overview():
    classes = _class_counts()
    file_count = file_bytes = log_lines = 0
    if os.path.isdir(FILES_DIR):
        for fn in os.listdir(FILES_DIR):
            fp = os.path.join(FILES_DIR, fn)
            if os.path.isfile(fp):
                file_count += 1
                file_bytes += os.path.getsize(fp)
    if os.path.isfile(LOG_FILE):
        with open(LOG_FILE, "r", encoding="utf-8", errors="replace") as f:
            log_lines = sum(1 for _ in f)
    return jsonify({"classes": classes, "fileCount": file_count,
                    "fileBytes": file_bytes, "logLines": log_lines})


@app.route("/admin/api/classes")
def admin_classes():
    return jsonify({"classes": _class_counts()})


@app.route("/admin/api/classes/<cls>", methods=["GET"])
def admin_class_objects(cls):
    with STORAGE_LOCK:
        objs = load_all_objects(cls)
    return jsonify({"class": cls, "count": len(objs), "results": objs})


@app.route("/admin/api/classes/<cls>", methods=["POST"])
def admin_create_object(cls):
    # Create an object from JSON; honors a custom objectId if provided.
    try:
        body = parse_json_body(request.get_data(cache=True))
    except ValueError as ex:
        return error_response(400, 107, str(ex))
    ts = now_iso()
    with STORAGE_LOCK:
        object_id = safe_name(body.get("objectId") or gen_object_id())
        if os.path.isfile(object_path(cls, object_id)):
            return error_response(400, 107, "objectId already exists: %s" % object_id)
        obj = {}
        merge_update(obj, body)
        obj["objectId"] = object_id
        obj["createdAt"] = ts
        obj["updatedAt"] = ts
        store_object(cls, obj)
    return jsonify({"objectId": object_id, "createdAt": ts})


@app.route("/admin/api/classes/<cls>/<object_id>", methods=["PUT"])
def admin_replace_object(cls, object_id):
    # Replace all fields of an object (keeps objectId/createdAt).
    try:
        body = parse_json_body(request.get_data(cache=True))
    except ValueError as ex:
        return error_response(400, 107, str(ex))
    ts = now_iso()
    with STORAGE_LOCK:
        path = object_path(cls, object_id)
        if not os.path.isfile(path):
            return error_response(404, 101, "Object not found.")
        with open(path, "r", encoding="utf-8") as f:
            old = json.load(f)
        obj = {}
        merge_update(obj, body)
        obj["objectId"] = object_id
        obj["createdAt"] = old.get("createdAt", ts)
        obj["updatedAt"] = ts
        store_object(cls, obj)
    return jsonify({"ok": True, "updatedAt": ts})


@app.route("/admin/api/files")
def admin_files():
    files = []
    if os.path.isdir(FILES_DIR):
        for fn in sorted(os.listdir(FILES_DIR)):
            fp = os.path.join(FILES_DIR, fn)
            if os.path.isfile(fp):
                st_ = os.stat(fp)
                files.append({"name": fn, "size": st_.st_size,
                              "modified": datetime.datetime.fromtimestamp(
                                  st_.st_mtime, datetime.timezone.utc
                              ).strftime("%Y-%m-%dT%H:%M:%SZ")})
    return jsonify({"files": files})


@app.route("/admin/api/log")
def admin_log():
    try:
        lines_n = max(1, min(5000, int(request.args.get("lines", 400))))
    except ValueError:
        lines_n = 400
    out = []
    if os.path.isfile(LOG_FILE):
        with open(LOG_FILE, "r", encoding="utf-8", errors="replace") as f:
            out = f.readlines()[-lines_n:]
    return jsonify({"lines": [l.rstrip("\n") for l in out]})


"""

src = src.replace(anchor, admin_routes + anchor, 1)
count += 1

io.open(PATH, "w", encoding="utf-8", newline="\n").write(src)
print("applied patches:", count)

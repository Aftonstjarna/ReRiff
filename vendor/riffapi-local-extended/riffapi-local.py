#!/usr/bin/env python3
"""RiffApi Local - local server for Riff Racer.

Based on rebane2001's riffapi-local (https://github.com/rebane2001/riffapi-local),
extended with persistent Parse-style object storage and file storage so that
ghost uploads/downloads (and shared tracks/scores) work again:

  * POST   /files/<name>          - stores the uploaded bytes, returns name+url
  * GET    /files/<name>          - serves previously stored bytes
  * DELETE /files/<name>          - deletes a stored file
  * POST   /classes/<class>       - creates an object (persisted to disk)
  * GET    /classes/<class>       - queries objects (where/order/limit/skip/count/keys)
  * GET    /classes/<class>/<id>  - fetches a single object
  * PUT    /classes/<class>/<id>  - updates an object
  * DELETE /classes/<class>/<id>  - deletes an object
  * POST   /batch                 - batched operations

All data is stored under ./data next to this script.
Every request is logged to ./data/requests.log and to the console.
"""

import datetime
import importlib.util
import json
import mimetypes
import os
import random
import re
import string
import threading
import urllib.parse

from flask import Flask, Response, jsonify, request, send_from_directory

app = Flask(__name__)

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
DATA_DIR = os.path.join(BASE_DIR, "data")
CLASSES_DIR = os.path.join(DATA_DIR, "classes")
FILES_DIR = os.path.join(DATA_DIR, "files")
LOG_FILE = os.path.join(DATA_DIR, "requests.log")

STORAGE_LOCK = threading.RLock()

news = {
    "title": "RiffApiLocal",
    "msg": "You're connected to RiffApiLocal.\nScores and ghosts are saved locally.",
}
username = "RiffApiLocal"

# Canned responses kept from the original riffapi-local: the game needs these
# exact configuration values to function.
responses = {
    "Configuration": '{"results":[{"objectId":"ITz5K79wcQ","updatedAt":"2015-10-06T15:37:10.761Z","RegionGroup":"Default","createdAt":"2015-08-27T16:48:23.993Z","OnTrackCoinValue":10,"TrackConversionCost":300,"ShareScoreCoinReward":200,"WatchAdCoinReward":200,"TrackCreatorCoinReward":100}]}',
    "News": f'{{"results":[{{"objectId":"RiffApiLocalNews","updatedAt":"2000-01-01T00:00:00.000Z","de":"{news["msg"]}","createdAt":"2000-01-01T00:00:00.000Z","ru":"{news["msg"]}","en":"{news["msg"]}","esTitle":"{news["title"]}","it":"{news["msg"]}","fr":"{news["msg"]}","frTitle":"{news["title"]}","itTitle":"{news["title"]}","es":"{news["msg"]}","enTitle":"{news["title"]}","Status":"Active","ruTitle":"{news["title"]}","deTitle":"{news["title"]}","Pinned":true,"ID":16}}]}}',
    "Profile_Steam": f'{{"results":[{{"objectId":"{username}Profile","SystemID":"1","FavouriteCar":1,"FavouriteSong":"null","FavouriteGenre":"pop","DriverLevel":0,"Platform":"PC","Username":"{username}","createdAt":"2000-01-01T00:00:00.000Z","updatedAt":"2000-01-01T00:00:00.000Z"}}]}}',
    "Version": '{"results":[{"objectId":"BoJMZ1TQJZ","updatedAt":"2015-02-10T21:05:53.214Z","Ver":100,"createdAt":"2015-02-10T20:50:24.164Z"}]}',
}

# Classes that always return the canned response above.
CANNED_CLASSES = ("Configuration", "News", "Version")

LOG_LOCK = threading.Lock()


def ensure_dirs():
    os.makedirs(CLASSES_DIR, exist_ok=True)
    os.makedirs(FILES_DIR, exist_ok=True)


def log_line(text):
    line = str(text)
    with LOG_LOCK:
        try:
            print(line)
        except Exception:
            pass
        try:
            with open(LOG_FILE, "a", encoding="utf-8") as f:
                f.write(line + "\n")
        except Exception:
            pass


def now_iso():
    return datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%S.%f")[:-3] + "Z"


def gen_object_id():
    alphabet = string.ascii_letters + string.digits
    return "".join(random.choice(alphabet) for _ in range(10))


def safe_name(name):
    """Sanitize a class/file name so it is safe to use as a path component."""
    name = os.path.basename(str(name))
    name = re.sub(r"[^A-Za-z0-9._\-]", "_", name)
    return name or "_"


def class_dir(cls):
    return os.path.join(CLASSES_DIR, safe_name(cls))


def object_path(cls, object_id):
    return os.path.join(class_dir(cls), safe_name(object_id) + ".json")


def load_all_objects(cls):
    """Return list of stored objects (dicts) for a class."""
    d = class_dir(cls)
    objs = []
    if not os.path.isdir(d):
        return objs
    for fn in sorted(os.listdir(d)):
        if not fn.endswith(".json"):
            continue
        try:
            with open(os.path.join(d, fn), "r", encoding="utf-8") as f:
                objs.append(json.load(f))
        except Exception:
            pass
    return objs


def store_object(cls, obj):
    d = class_dir(cls)
    os.makedirs(d, exist_ok=True)
    path = object_path(cls, obj["objectId"])
    tmp = path + ".tmp"
    with open(tmp, "w", encoding="utf-8") as f:
        json.dump(obj, f, ensure_ascii=False)
    os.replace(tmp, path)


def delete_stored_object(cls, object_id):
    path = object_path(cls, object_id)
    if os.path.isfile(path):
        os.remove(path)
        return True
    return False


def scalarize(value):
    """Reduce Parse-typed values to plain scalars for comparison/sorting."""
    if isinstance(value, dict):
        t = value.get("__type")
        if t == "Pointer":
            return value.get("objectId")
        if t == "File":
            return value.get("name")
        if t == "Date":
            return value.get("iso")
    return value


def get_field(obj, key):
    """Fetch a (possibly dotted) field; returns (value, present)."""
    cur = obj
    for part in key.split("."):
        if isinstance(cur, dict) and part in cur:
            cur = cur[part]
        else:
            return None, False
    return cur, True


def compare_values(a, b):
    """Compare two scalars; returns -1/0/1. Never raises."""
    a_num = isinstance(a, (int, float)) and not isinstance(a, bool)
    b_num = isinstance(b, (int, float)) and not isinstance(b, bool)
    if a_num and b_num:
        return (a > b) - (a < b)
    sa, sb = str(a), str(b)
    return (sa > sb) - (sa < sb)


def condition_matches(value, present, cond):
    """Evaluate a where-condition against one field value."""
    if isinstance(cond, dict) and cond and all(isinstance(k, str) and k.startswith("$") for k in cond.keys()):
        for op, operand in cond.items():
            if op == "$ne":
                if present and scalarize(value) == scalarize(operand):
                    return False
            elif op == "$in":
                wanted = [scalarize(x) for x in operand] if isinstance(operand, list) else []
                if (not present) or scalarize(value) not in wanted:
                    return False
            elif op == "$nin":
                unwanted = [scalarize(x) for x in operand] if isinstance(operand, list) else []
                if present and scalarize(value) in unwanted:
                    return False
            elif op == "$exists":
                if bool(present and value is not None) != bool(operand):
                    return False
            elif op in ("$lt", "$lte", "$gt", "$gte"):
                if not present or value is None:
                    return False
                c = compare_values(scalarize(value), scalarize(operand))
                if op == "$lt" and not c < 0:
                    return False
                if op == "$lte" and not c <= 0:
                    return False
                if op == "$gt" and not c > 0:
                    return False
                if op == "$gte" and not c >= 0:
                    return False
            elif op == "$regex":
                if not present or not isinstance(value, str):
                    return False
                flags = re.IGNORECASE if "i" in str(cond.get("$options") or "") else 0
                try:
                    if not re.search(operand, value, flags):
                        return False
                except re.error:
                    return False
            elif op == "$options":
                continue
            elif op == "$all":
                if not present or not isinstance(value, list):
                    return False
                wanted = [scalarize(x) for x in operand] if isinstance(operand, list) else []
                have = [scalarize(x) for x in value]
                if not all(w in have for w in wanted):
                    return False
            else:
                # Unsupported operator -> be conservative, no match.
                return False
        return True
    # Plain equality
    if not present:
        return False
    return scalarize(value) == scalarize(cond)


def matches_where(obj, where):
    if not isinstance(where, dict):
        return True
    for key, cond in where.items():
        value, present = get_field(obj, key)
        if not condition_matches(value, present, cond):
            return False
    return True


def song_aliases():
    """Use registered source hashes, retaining legacy explicit groups."""
    aliases = dict(catalog_aliases())
    path = os.path.join(DATA_DIR, "song-aliases.json")
    if not os.path.isfile(path):
        return aliases
    with open(path, encoding="utf-8") as stream:
        groups = json.load(stream)
    if not isinstance(groups, list):
        raise ValueError("song-aliases.json must contain a list of ID groups")
    seen = set()
    for group in groups:
        if (not isinstance(group, list) or len(group) < 2
                or any(not isinstance(x, str) or not re.fullmatch(r"[A-Za-z0-9_-]+", x) for x in group)
                or len(set(group)) != len(group) or any(x in seen for x in group)):
            raise ValueError("Invalid or overlapping song alias group")
        seen.update(group)
        roots = {aliases.get(x, x) for x in group}
        canonical = min(roots)
        aliases = {key: canonical if value in roots else value for key, value in aliases.items()}
        aliases.update((x, canonical) for x in group)
    return aliases


def song_query_view(cls, objs, where):
    """Compare cached song IDs canonically without modifying stored records.

    Object IDs, file names and URLs remain intact so updates and ghost downloads
    still refer to their original records. Only song identity is translated.
    """
    field = ("SongID" if cls == "Score" else "song" if cls.startswith("ghost_v")
             else "name" if cls == "FileData" else None)
    if field is None or not isinstance(where, dict) or field not in where:
        return objs, where
    aliases = song_aliases()

    def canonical(value):
        if isinstance(value, str):
            if cls == "FileData":
                parts = song_file_parts(value)
                return aliases.get(parts[0], parts[0]) + parts[1] if parts else value
            return aliases.get(value, value)
        if isinstance(value, list):
            return [canonical(v) for v in value]
        if isinstance(value, dict):
            return {k: canonical(v) if k in ("$in", "$nin", "$ne") else v for k, v in value.items()}
        return value

    return [dict(o, **{field: canonical(o[field])}) if field in o else o for o in objs], dict(where, **{field: canonical(where[field])})


def song_file_parts(name):
    match = re.fullmatch(r"([A-Za-z0-9_-]+?)(-[0-9]+\.ghost|_v[0-9]+\.bin)", name)
    return match.groups() if match else None


def sort_objects(objs, order_spec):
    keys = [k.strip() for k in str(order_spec).split(",") if k.strip()]
    result = list(objs)

    def sort_key_for(field):
        def key_fn(o):
            value, present = get_field(o, field)
            v = scalarize(value)
            if not present or v is None:
                return (0, 0, 0)  # missing sorts first (like undefined in Parse)
            if isinstance(v, bool):
                return (1, 0, int(v))
            if isinstance(v, (int, float)):
                return (1, 1, float(v))
            return (1, 2, str(v))
        return key_fn

    # Stable multi-key sort: apply from last key to first.
    for field in reversed(keys):
        descending = field.startswith("-")
        fname = field[1:] if descending else field
        result.sort(key=sort_key_for(fname), reverse=descending)
    return result


def project_keys(obj, keys_param):
    if not keys_param:
        return obj
    wanted = [k.strip() for k in str(keys_param).split(",") if k.strip()]
    out = {}
    for k in wanted:
        value, present = get_field(obj, k)
        if present:
            out[k] = value
    # Always keep identity fields.
    for meta in ("objectId", "createdAt", "updatedAt"):
        if meta in obj:
            out[meta] = obj[meta]
    return out


def include_pointers(obj, include_param):
    if not include_param:
        return obj
    fields = [f.strip() for f in str(include_param).split(",") if f.strip()]
    out = dict(obj)
    for field in fields:
        value = out.get(field)
        if isinstance(value, dict) and value.get("__type") == "Pointer":
            ref_cls = value.get("className")
            ref_id = value.get("objectId")
            if ref_cls and ref_id:
                path = object_path(ref_cls, ref_id)
                if os.path.isfile(path):
                    try:
                        with open(path, "r", encoding="utf-8") as f:
                            out[field] = json.load(f)
                    except Exception:
                        pass
    return out


def parse_json_body(raw):
    if raw is None or raw == b"":
        return {}
    try:
        text = raw.decode("utf-8")
    except UnicodeDecodeError:
        raise ValueError("body is not valid UTF-8")
    try:
        parsed = json.loads(text)
    except json.JSONDecodeError:
        raise ValueError("invalid JSON body")
    if not isinstance(parsed, dict):
        raise ValueError("JSON body must be an object")
    return parsed


def error_response(status, code, message):
    resp = jsonify({"code": code, "error": message})
    resp.status_code = status
    return resp


def apply_field_op(current, op):
    """Apply a Parse REST operation ({"__op": ...}) to a current value."""
    kind = op.get("__op")
    if kind == "Increment":
        amount = op.get("amount", 1)
        base = current if isinstance(current, (int, float)) and not isinstance(current, bool) else 0
        return base + amount, False
    if kind == "Add":
        base = list(current) if isinstance(current, list) else []
        return base + list(op.get("objects", [])), False
    if kind == "AddUnique":
        base = list(current) if isinstance(current, list) else []
        for item in op.get("objects", []):
            if item not in base:
                base.append(item)
        return base, False
    if kind == "Remove":
        base = list(current) if isinstance(current, list) else []
        return [x for x in base if x not in op.get("objects", [])], False
    if kind == "Delete":
        return None, True
    # Unknown operation: store verbatim.
    return op, False


def merge_update(obj, updates):
    """Merge REST update fields (including __op operations) into an object."""
    for k, v in updates.items():
        if k in ("objectId", "createdAt"):
            continue
        if isinstance(v, dict) and "__op" in v:
            new_val, del_flag = apply_field_op(obj.get(k), v)
            if del_flag:
                obj.pop(k, None)
            else:
                obj[k] = new_val
        else:
            obj[k] = v


# ---------------------------------------------------------------------------
# Request logging
# ---------------------------------------------------------------------------

@app.before_request
def _log_request():
    body = request.get_data(cache=True)
    try:
        body_text = body.decode("utf-8") if body else ""
    except UnicodeDecodeError:
        body_text = "<%d binary bytes> %s" % (len(body), body[:48].hex())
    if len(body_text) > 4000:
        body_text = body_text[:4000] + "...(truncated)"
    qs = request.query_string.decode("utf-8", "replace")
    log_line("")
    log_line("[%s] %s %s%s" % (now_iso(), request.method, request.path, ("?" + qs) if qs else ""))
    log_line("  from %s | AppId=%r WindowsKey=%r SessionToken=%r" % (
        request.remote_addr,
        request.headers.get("X-Parse-Application-Id"),
        request.headers.get("X-Parse-Windows-Key"),
        request.headers.get("X-Parse-Session-Token"),
    ))
    if body_text:
        log_line("  body: %s" % body_text)


@app.after_request
def _log_response(resp):
    if request.path.startswith("/classes/") or request.path.startswith("/files/") or request.path == "/batch":
        log_line("  -> %d (%d bytes)" % (resp.status_code, resp.calculate_content_length() or 0))
    return resp


# ---------------------------------------------------------------------------
# Files API
# ---------------------------------------------------------------------------

def host_base():
    host = request.host or "127.0.0.1:5144"
    return "http://%s" % host


@app.route("/files/<path:filename>", methods=["POST"])
def upload_file(filename):
    data = request.get_data(cache=False)
    if not data:
        return error_response(400, 107, "Empty file upload")
    name = safe_name(filename)
    os.makedirs(FILES_DIR, exist_ok=True)
    dest = os.path.join(FILES_DIR, name)
    with STORAGE_LOCK:
        with open(dest, "wb") as f:
            f.write(data)
    url = "%s/files/%s" % (host_base(), urllib.parse.quote(name))
    log_line("  stored file %s (%d bytes)" % (name, len(data)))
    resp = jsonify({"url": url, "name": name})
    resp.status_code = 201
    return resp


@app.route("/files/<path:filename>", methods=["GET"])
def download_file(filename):
    name = safe_name(filename)
    path = os.path.join(FILES_DIR, name)
    # The game builds ghost names from its own cached ID plus the selected user.
    # Resolve equivalent IDs on reads; preserve the original upload and URL.
    if not os.path.isfile(path):
        parts = song_file_parts(name)
        if parts:
            with STORAGE_LOCK:
                aliases = song_aliases()
                root = aliases.get(parts[0], parts[0])
                for song_id, canonical in sorted(aliases.items()):
                    candidate = os.path.join(FILES_DIR, song_id + parts[1])
                    if canonical == root and os.path.isfile(candidate):
                        path = candidate
                        break
    if not os.path.isfile(path):
        return error_response(404, 101, "File not found: %s" % name)
    mime = mimetypes.guess_type(name)[0] or "application/octet-stream"
    with open(path, "rb") as f:
        payload = f.read()
    return Response(payload, mimetype=mime)


@app.route("/files/<path:filename>", methods=["DELETE"])
def delete_file(filename):
    name = safe_name(filename)
    path = os.path.join(FILES_DIR, name)
    with STORAGE_LOCK:
        if os.path.isfile(path):
            os.remove(path)
    return jsonify({})


# ---------------------------------------------------------------------------
# Classes API
# ---------------------------------------------------------------------------

@app.route("/classes/<cls>", methods=["GET"])
def query_class(cls):
    args = request.args

    if cls in CANNED_CLASSES:
        return Response(responses[cls], mimetype="application/json")

    if cls == "_Installation":
        return Response('{"opens":1,"updatedAt":"2000-01-01T00:00:00.000Z"}', mimetype="application/json")

    if cls == "Profile_Steam":
        # Fall back to the canned profile until the game stores its own.
        with STORAGE_LOCK:
            stored = load_all_objects(cls)
        if not stored:
            return Response(responses["Profile_Steam"], mimetype="application/json")

    where_raw = args.get("where", "{}")
    try:
        where = json.loads(where_raw) if where_raw else {}
    except json.JSONDecodeError:
        return error_response(400, 107, "Invalid 'where' JSON")

    with STORAGE_LOCK:
        objs = load_all_objects(cls)

    views, match_where = song_query_view(cls, objs, where)
    filtered = [o for o, view in zip(objs, views) if matches_where(view, match_where)]
    # A player may already have records under several equivalent cached IDs.
    # Rank their best score once, while retaining every stored record for undo.
    identity_field = 'SongID' if cls == 'Score' else 'song' if cls.startswith('ghost_v') else None
    if identity_field and isinstance(where, dict) and identity_field in where:
        aliases = song_aliases()
        best = {}
        for obj in filtered:
            owner = obj.get('Owner' if cls == 'Score' else 'user', obj['objectId'])
            song = obj.get(identity_field)
            key = (aliases.get(song, song), str(owner), str(obj.get('Mode')), str(obj.get('Version')))
            score_field = 'Score' if cls == 'Score' else 'score'
            if key not in best or compare_values(obj.get(score_field, 0), best[key].get(score_field, 0)) > 0:
                best[key] = obj
        filtered = list(best.values())
    total = len(filtered)

    order = args.get("order")
    if order:
        filtered = sort_objects(filtered, order)

    try:
        skip = max(0, int(args.get("skip", 0)))
    except ValueError:
        skip = 0
    try:
        limit = int(args.get("limit", 100))
    except ValueError:
        limit = 100
    if limit < 0:
        limit = 1000000

    page = filtered[skip:skip + limit]
    page = [include_pointers(project_keys(o, args.get("keys")), args.get("include")) for o in page]

    result = {"results": page}
    if args.get("count") in ("1", "true"):
        result["count"] = total
    return jsonify(result)


@app.route("/classes/<cls>", methods=["POST"])
def create_class(cls):
    try:
        body = parse_json_body(request.get_data(cache=True))
    except ValueError as ex:
        return error_response(400, 107, str(ex))

    ts = now_iso()
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
        obj["objectId"] = object_id
        obj["createdAt"] = ts
        obj["updatedAt"] = ts
        store_object(cls, obj)
    return jsonify({"objectId": object_id, "createdAt": ts})


@app.route("/classes/<cls>/<object_id>", methods=["GET"])
def get_class_object(cls, object_id):
    with STORAGE_LOCK:
        objs = load_all_objects(cls)
    for o in objs:
        if o.get("objectId") == object_id:
            return jsonify(o)
    return error_response(404, 101, "Object not found for get.")


@app.route("/classes/<cls>/<object_id>", methods=["PUT"])
def update_class_object(cls, object_id):
    try:
        body = parse_json_body(request.get_data(cache=True))
    except ValueError as ex:
        return error_response(400, 107, str(ex))

    ts = now_iso()
    with STORAGE_LOCK:
        path = object_path(cls, object_id)
        if not os.path.isfile(path):
            return error_response(404, 101, "Object not found for update.")
        try:
            with open(path, "r", encoding="utf-8") as f:
                obj = json.load(f)
        except Exception:
            obj = {"objectId": object_id}
        merge_update(obj, body)
        obj["updatedAt"] = ts
        store_object(cls, obj)
    return jsonify({"updatedAt": ts})


@app.route("/classes/<cls>/<object_id>", methods=["DELETE"])
def delete_class_object(cls, object_id):
    with STORAGE_LOCK:
        deleted = delete_stored_object(cls, object_id)
    if not deleted:
        return error_response(404, 101, "Object not found for delete.")
    return jsonify({})


# ---------------------------------------------------------------------------
# Batch API
# ---------------------------------------------------------------------------

@app.route("/batch", methods=["POST"])
def batch():
    try:
        body = parse_json_body(request.get_data(cache=True))
    except ValueError as ex:
        return error_response(400, 107, str(ex))

    requests_list = body.get("requests")
    if not isinstance(requests_list, list):
        return error_response(400, 107, "Missing 'requests' array")

    results = []
    for item in requests_list:
        method = str(item.get("method", "")).upper()
        path = str(item.get("path", ""))
        inner_body = item.get("body") or {}
        try:
            parts = [p for p in path.split("/") if p]
            if len(parts) >= 2 and parts[0] == "classes":
                cls = parts[1]
                oid = parts[2] if len(parts) > 2 else None
                if method == "POST" and oid is None:
                    ts = now_iso()
                    with STORAGE_LOCK:
                        new_id = gen_object_id()
                        while os.path.isfile(object_path(cls, new_id)):
                            new_id = gen_object_id()
                        obj = {}
                        merge_update(obj, inner_body)
                        obj["objectId"] = new_id
                        obj["createdAt"] = ts
                        obj["updatedAt"] = ts
                        store_object(cls, obj)
                    results.append({"success": {"objectId": new_id, "createdAt": ts}})
                elif method in ("PUT", "POST") and oid is not None:
                    ts = now_iso()
                    with STORAGE_LOCK:
                        p = object_path(cls, oid)
                        if not os.path.isfile(p):
                            raise LookupError("Object not found.")
                        with open(p, "r", encoding="utf-8") as f:
                            obj = json.load(f)
                        merge_update(obj, inner_body)
                        obj["updatedAt"] = ts
                        store_object(cls, obj)
                    results.append({"success": {"updatedAt": ts}})
                elif method == "DELETE" and oid is not None:
                    with STORAGE_LOCK:
                        ok = delete_stored_object(cls, oid)
                    if not ok:
                        raise LookupError("Object not found.")
                    results.append({"success": {}})
                elif method == "GET":
                    with STORAGE_LOCK:
                        objs = load_all_objects(cls)
                    if oid is not None:
                        found = [o for o in objs if o.get("objectId") == oid]
                        if not found:
                            raise LookupError("Object not found.")
                        results.append({"success": found[0]})
                    else:
                        results.append({"success": {"results": objs}})
                else:
                    results.append({"error": {"code": 107, "error": "Unsupported batch operation"}})
            else:
                results.append({"error": {"code": 107, "error": "Unsupported batch path: %s" % path}})
        except LookupError as ex:
            results.append({"error": {"code": 101, "error": str(ex)}})
        except Exception as ex:
            results.append({"error": {"code": 1, "error": "Internal error: %s" % ex}})
    return jsonify(results)


# ---------------------------------------------------------------------------
# Misc endpoints kept from the original server
# ---------------------------------------------------------------------------

@app.route("/events/AppOpened", methods=["GET", "POST"])
def AppOpened():
    return "{}"


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


@app.route("/", methods=["GET"])
def index():
    return jsonify({
        "server": "RiffApiLocal",
        "note": "Parse-compatible local server for Riff Racer with persistent ghosts.",
        "dataDir": DATA_DIR,
    })



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
    file_count = 0
    file_bytes = 0
    if os.path.isdir(FILES_DIR):
        for fn in os.listdir(FILES_DIR):
            fp = os.path.join(FILES_DIR, fn)
            if os.path.isfile(fp):
                file_count += 1
                file_bytes += os.path.getsize(fp)
    log_lines = 0
    if os.path.isfile(LOG_FILE):
        with open(LOG_FILE, "r", encoding="utf-8", errors="replace") as f:
            log_lines = sum(1 for _ in f)
    return jsonify({
        "classes": classes,
        "fileCount": file_count,
        "fileBytes": file_bytes,
        "logLines": log_lines,
    })


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
    """Create an object from JSON; honors a custom objectId if provided."""
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
    log_line("  admin created %s/%s" % (cls, object_id))
    return jsonify({"objectId": object_id, "createdAt": ts})


@app.route("/admin/api/classes/<cls>/<object_id>", methods=["PUT"])
def admin_replace_object(cls, object_id):
    """Replace all fields of an object (keeps objectId/createdAt)."""
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
    log_line("  admin replaced %s/%s" % (cls, object_id))
    return jsonify({"ok": True, "updatedAt": ts})


@app.route("/admin/api/files")
def admin_files():
    files = []
    if os.path.isdir(FILES_DIR):
        for fn in sorted(os.listdir(FILES_DIR)):
            fp = os.path.join(FILES_DIR, fn)
            if os.path.isfile(fp):
                st_ = os.stat(fp)
                files.append({
                    "name": fn,
                    "size": st_.st_size,
                    "modified": datetime.datetime.fromtimestamp(
                        st_.st_mtime, datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
                })
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


_catalog_spec = importlib.util.spec_from_file_location(
    'reriff_song_catalog', os.path.join(BASE_DIR, 'song_catalog.py'))
_catalog_module = importlib.util.module_from_spec(_catalog_spec)
_catalog_spec.loader.exec_module(_catalog_module)
_catalog_module.install(globals())

if __name__ == "__main__":
    ensure_dirs()
    log_line("=" * 60)
    log_line("RiffApiLocal starting on http://127.0.0.1:5144")
    log_line("Data dir: %s" % DATA_DIR)
    log_line("=" * 60)
    app.run(host="127.0.0.1", port=5144, debug=False, threaded=True)

# sed -i "s/https:\/\/parseapi.back4app.com/http:\/\/127.000000000.0.1:5144/g" ParseOctane.dll

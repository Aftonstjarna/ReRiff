"""One-shot patcher: adds Parse __op support to riffapi-local.py."""
import io

PATH = r"C:\Games\Riff Racer\riffapi-local\riffapi-local.py"

with io.open(PATH, "r", encoding="utf-8") as f:
    src = f.read()

anchor1 = '''def error_response(status, code, message):
    resp = jsonify({"code": code, "error": message})
    resp.status_code = status
    return resp
'''

addition = '''

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
'''

old_put = '''        except Exception:
            obj = {"objectId": object_id}
        for k, v in body.items():
            if k in ("objectId", "createdAt"):
                continue
            obj[k] = v
        obj["updatedAt"] = ts'''

new_put = '''        except Exception:
            obj = {"objectId": object_id}
        merge_update(obj, body)
        obj["updatedAt"] = ts'''

old_batch = '''                        with open(p, "r", encoding="utf-8") as f:
                            obj = json.load(f)
                        for k, v in inner_body.items():
                            if k in ("objectId", "createdAt"):
                                continue
                            obj[k] = v
                        obj["updatedAt"] = ts'''

new_batch = '''                        with open(p, "r", encoding="utf-8") as f:
                            obj = json.load(f)
                        merge_update(obj, inner_body)
                        obj["updatedAt"] = ts'''

count = 0
if anchor1 in src and "def merge_update" not in src:
    src = src.replace(anchor1, anchor1 + addition, 1)
    count += 1
if old_put in src:
    src = src.replace(old_put, new_put, 1)
    count += 1
if old_batch in src:
    src = src.replace(old_batch, new_batch, 1)
    count += 1

with io.open(PATH, "w", encoding="utf-8", newline="") as f:
    f.write(src)

print("applied patches:", count)
assert count == 3, "expected 3 patches"
print("OK")
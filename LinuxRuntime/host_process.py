"""Small native-host primitives shared by the optional server controllers."""
from contextlib import contextmanager
import os
from pathlib import Path
import time


def windows_identity(pid):
    import ctypes
    from ctypes import wintypes as w
    kernel = ctypes.WinDLL('kernel32', use_last_error=True)
    kernel.OpenProcess.argtypes = (w.DWORD, w.BOOL, w.DWORD)
    kernel.OpenProcess.restype = w.HANDLE
    kernel.GetProcessTimes.argtypes = (w.HANDLE, *([ctypes.POINTER(w.FILETIME)]*4))
    kernel.QueryFullProcessImageNameW.argtypes = (w.HANDLE, w.DWORD, w.LPWSTR, ctypes.POINTER(w.DWORD))
    kernel.WaitForSingleObject.argtypes = (w.HANDLE, w.DWORD)
    kernel.CloseHandle.argtypes = (w.HANDLE,)
    handle = kernel.OpenProcess(0x1000 | 0x100000, False, pid)
    if not handle:
        return None
    try:
        created, exited, cpu, user = (w.FILETIME() for _ in range(4))
        name, length = ctypes.create_unicode_buffer(32768), w.DWORD(32768)
        if (kernel.WaitForSingleObject(handle, 0) != 258
                or not kernel.GetProcessTimes(handle, created, exited, cpu, user)
                or not kernel.QueryFullProcessImageNameW(handle, 0, name, length)):
            return None
        return str((created.dwHighDateTime << 32) | created.dwLowDateTime), name.value
    finally:
        kernel.CloseHandle(handle)


def stamp(pid):
    if os.name == 'nt':
        identity = windows_identity(pid)
        return identity[0] if identity else None
    try:
        fields = Path(f'/proc/{pid}/stat').read_text().rsplit(')', 1)[1].split()
        return fields[19] if fields[0] != 'Z' else None
    except (OSError, IndexError):
        return None


@contextmanager
def file_lock(path, blocking=True):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open('a+b') as stream:
        if os.name != 'nt':
            import fcntl
            fcntl.flock(stream, fcntl.LOCK_EX | (0 if blocking else fcntl.LOCK_NB))
            yield stream
            return
        import msvcrt
        stream.seek(0, 2)
        if not stream.tell():
            stream.write(b'\0');stream.flush()
        deadline = time.monotonic() + (30 if blocking else 0)
        while True:
            try:
                stream.seek(0);msvcrt.locking(stream.fileno(), msvcrt.LK_NBLCK, 1)
                break
            except OSError:
                if time.monotonic() >= deadline:
                    raise RuntimeError('Another local-server operation is still running.')
                time.sleep(.1)
        try:
            yield stream
        finally:
            stream.seek(0);msvcrt.locking(stream.fileno(), msvcrt.LK_UNLCK, 1)

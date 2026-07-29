import os
from config import ALLOWED_EXTENSIONS, VERSION_TAG_REGEX, LAUNCHER_VERSION

def safe_join(root_abs: str, rel: str):
    if not rel or os.path.isabs(rel) or "\x00" in rel:
        return None
    forbidden = {"CON","PRN","AUX","NUL","COM1","COM2","COM3","LPT1","LPT2"}
    for part in rel.replace("\\", "/").split("/"):
        if part.split(".")[0].upper() in forbidden:
            return None
    target = os.path.normpath(os.path.join(root_abs, rel))
    try:
        if os.path.commonpath([root_abs, os.path.abspath(target)]) != root_abs:
            return None
    except ValueError:
        return None
    if os.path.islink(target):
        real = os.path.realpath(target)
        try:
            if os.path.commonpath([root_abs, real]) != root_abs:
                return None
        except ValueError:
            return None
    return target

def check_extension(arcname: str) -> bool:
    _, ext = os.path.splitext(arcname.lower())
    return ext in ALLOWED_EXTENSIONS

def inject_version(content: str) -> str:
    return VERSION_TAG_REGEX.sub(LAUNCHER_VERSION, content)
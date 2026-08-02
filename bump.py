#!/usr/bin/env python3
import sys
import re
from pathlib import Path

def bump_version(new_version: str):
    if not re.match(r"^\d+\.\d+\.\d+(-[a-zA-Z0-9.]+)?$", new_version):
        print(f"Error: Invalid version format '{new_version}'. Expected format like '0.0.0'.")
        sys.exit(1)

    root_dir = Path(__file__).parent.resolve()
    
    props_path = root_dir / "library.properties"
    if props_path.exists():
        content = props_path.read_text(encoding="utf-8")
        updated_content = re.sub(r"^version=.*$", f"version={new_version}", content, flags=re.MULTILINE)
        props_path.write_text(updated_content, encoding="utf-8")
        print(f"[OK] Updated library.properties -> version={new_version}")
    else:
        print("[!] Warning: library.properties not found.")

    json_path = root_dir / "library.json"
    if json_path.exists():
        content = json_path.read_text(encoding="utf-8")
        updated_content = re.sub(r'("version"\s*:\s*")[^"]+(")', f'\\g<1>{new_version}\\g<2>', content)
        json_path.write_text(updated_content, encoding="utf-8")
        print(f"[OK] Updated library.json -> \"version\": \"{new_version}\"")
    else:
        print("[!] Warning: library.json not found.")

    print(f"\nSuccessfully bumped project version to {new_version}!")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python bump.py <new_version>")
        print("Example: python bump.py 1.4.1")
        sys.exit(1)
        
    bump_version(sys.argv[1].strip())

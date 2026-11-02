#!/usr/bin/env python3
"""Download the Apple open-source projects listed in projects.txt, pinned by the macOS release manifest.

    python3 -I userland/fetch-sources.py OUTDIR [PROJECT...]

The tags come from apple-oss-distributions/distribution-macOS (tag RELEASE_TAG, release.json), so every
project is the version that shipped with macOS 13.5. Each project is unpacked to OUTDIR/<project>;
OUTDIR/MANIFEST lists "<project> <tag>" for what was fetched. Projects absent from the manifest are an
error: the closed ones are handled by stubs (docs/userland-plan.md), not downloaded.
"""
import io
import json
import os
import sys
import tarfile
import time
import urllib.request

RELEASE_TAG = "macos-135"
MANIFEST_URL = f"https://raw.githubusercontent.com/apple-oss-distributions/distribution-macOS/{RELEASE_TAG}/release.json"
TARBALL_URL = "https://github.com/apple-oss-distributions/{project}/archive/refs/tags/{tag}.tar.gz"
HERE = os.path.dirname(os.path.abspath(__file__))


def get(url, tries=4):
    for i in range(tries):
        try:
            with urllib.request.urlopen(url, timeout=120) as r:
                return r.read()
        except OSError as e:
            if i == tries - 1:
                raise
            print(f"  retry {url}: {e}", file=sys.stderr)
            time.sleep(2 ** i)


def project_list():
    with open(os.path.join(HERE, "projects.txt")) as f:
        return [l.split("#")[0].strip() for l in f if l.split("#")[0].strip()]


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    out = sys.argv[1]
    wanted = sys.argv[2:] or project_list()
    tags = {p["project"]: p["tag"] for p in json.loads(get(MANIFEST_URL))["projects"]}
    missing = [p for p in wanted if p not in tags]
    if missing:
        sys.exit(f"not in the {RELEASE_TAG} manifest: {' '.join(missing)}")
    os.makedirs(out, exist_ok=True)
    done = []
    for p in wanted:
        dest = os.path.join(out, p)
        if os.path.isdir(dest):
            print(f"have {p} {tags[p]}")
        else:
            print(f"fetch {p} {tags[p]}")
            data = get(TARBALL_URL.format(project=p, tag=tags[p]))
            tmp = dest + ".tmp"
            with tarfile.open(fileobj=io.BytesIO(data), mode="r:gz") as t:
                # the tarball has a single top-level directory "<project>-<tag>"
                t.extractall(tmp, filter="data")
            (top,) = os.listdir(tmp)
            os.rename(os.path.join(tmp, top), dest)
            os.rmdir(tmp)
        done.append(f"{p} {tags[p]}")
    with open(os.path.join(out, "MANIFEST"), "w") as f:
        f.write("\n".join(done) + "\n")


if __name__ == "__main__":
    main()

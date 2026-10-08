#!/usr/bin/env python3
"""Install the declared MOS SDK; optionally include TCC and its example."""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import sys

ROOT = Path(__file__).resolve().parents[1]
# Retain the existing ledger name so earlier installs migrate safely.
STATE = '.tcc-install.json'


def digest(data):
    return hashlib.sha256(data).hexdigest()


def relative_path(name):
    p = PurePosixPath(name)
    if p.is_absolute() or not p.parts or any(x in ('.', '..') for x in p.parts):
        raise ValueError('Invalid relative path: ' + name)
    if '\\' in name or ':' in name:
        raise ValueError('Invalid relative path: ' + name)
    return p


def within(root, name):
    p = root.joinpath(*relative_path(name).parts)
    if not p.resolve().is_relative_to(root.resolve()):
        raise ValueError('Path escapes destination: ' + name)
    return p


def owned_name(name):
    relative_path(name)
    return (name in ('bin/tcc', 'lib/libmos.a', 'src/hello.c')
            or name.startswith('include/')
            or name.startswith('src/tcc/'))


def install(root, dest, dry_run=False, with_tcc=False):
    entries = []
    manifests = ['api/install.json']
    if with_tcc:
        manifests.append('apps/tcc/install.json')
    for manifest in manifests:
        spec = json.loads((root / manifest).read_text(encoding='utf-8'))
        if spec.get('schema') != 1:
            raise ValueError('Unsupported installation manifest schema')
        entries.extend(spec['files'])
    def selected(name):
        return with_tcc or name.startswith('include/') or name == 'lib/libmos.a'
    # Read and validate every source before changing the destination.
    payload = {}
    folded = set()
    for entry in entries:
        name = entry['target']
        if not owned_name(name) or name.casefold() in folded:
            raise ValueError('Invalid or duplicate target: ' + name)
        folded.add(name.casefold())
        payload[name] = within(root, entry['source']).read_bytes()
    state_path = within(dest, STATE)
    old = {}
    if state_path.exists():
        state = json.loads(state_path.read_text(encoding='utf-8'))
        if state.get('schema') != 1:
            raise ValueError('Unsupported installed-state schema')
        old = state['files']
    for name, expected in old.items():
        if not owned_name(name):
            raise ValueError('Invalid installed-state path: ' + name)
        if not selected(name):
            continue
        p = within(dest, name)
        if p.exists() and digest(p.read_bytes()) != expected:
            raise ValueError('Locally modified installed file: ' + str(p)
                             + '; move your edited copy before updating')
    actions = []
    for name, data in payload.items():
        p = within(dest, name)
        if not p.exists() or p.read_bytes() != data:
            actions.append(('copy', name))
    for name in old.keys() - payload.keys():
        if selected(name) and within(dest, name).exists():
            actions.append(('remove', name))
    for action, name in actions:
        print(action + ' ' + name)
    if dry_run:
        print('Dry run: no files changed')
        return
    for action, name in actions:
        p = within(dest, name)
        if action == 'remove':
            p.unlink()
        else:
            p.parent.mkdir(parents=True, exist_ok=True)
            p.write_bytes(payload[name])
    dest.mkdir(parents=True, exist_ok=True)
    state_path.write_text(json.dumps({
        'schema': 1, 'files': {**{n: h for n, h in old.items() if not selected(n)},
                              **{n: digest(b) for n, b in payload.items()}}
    }, indent=2) + '\n', encoding='utf-8')
    print('Installed ' + str(len(payload)) + ' declared files')


def main(with_tcc=False):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--dest', required=True, type=Path,
                        help='MOS2 directory on the SD card or in a release tree')
    parser.add_argument('--dry-run', action='store_true')
    parser.add_argument('--with-tcc', action='store_true', default=with_tcc)
    args = parser.parse_args()
    try:
        install(ROOT, args.dest.resolve(), args.dry_run, args.with_tcc)
    except (OSError, ValueError, KeyError, TypeError) as error:
        parser.exit(1, str(error) + '\n')


if __name__ == '__main__':
    main()

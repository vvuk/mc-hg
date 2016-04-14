# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this,
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

from __future__ import unicode_literals

import os
import subprocess

from . import pex

# The logic here is far from robust. Improvements are welcome.

def update_mercurial_repo(hg, repo, path, revision='default',
                          hostfingerprints=None, global_args=None):
    """Ensure a HG repository exists at a path and is up to date."""
    hostfingerprints = hostfingerprints or {}

    args = [hg]
    if global_args:
        args.extend(global_args)

    for host, fingerprint in sorted(hostfingerprints.items()):
        args.extend(['--config', 'hostfingerprints.%s=%s' % (host,
            fingerprint)])

    if os.path.exists(path):
        pex.run_process(args + ['pull', repo], cwd=path, require_unix_environment=True)
    else:
        pex.run_process(args + ['clone', repo, path], require_unix_environment=True)

    pex.run_process([hg, 'update', '-r', revision], cwd=path, require_unix_environment=True)


def update_git_repo(git, repo, path, revision='origin/master'):
    """Ensure a Git repository exists at a path and is up to date."""
    if os.path.exists(path):
        pex.run_process([git, 'fetch', '--all'], cwd=path, require_unix_environment=True)
    else:
        pex.run_process([git, 'clone', repo, path], require_unix_environment=True)

    pex.run_process([git, 'checkout', revision], cwd=path, require_unix_environment=True)

# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

from __future__ import unicode_literals

import string

manifest_template = """# THIS FILE IS AUTOGENERATED BY ${caller} - DO NOT EDIT
[DEFAULT]
support-files =
${supportfiles}

${tests}
"""

reftest_template = """# THIS FILE IS AUTOGENERATED BY ${caller} - DO NOT EDIT

${reftests}
"""



def substManifest(caller, test_files, support_files):
    test_files = [f.lstrip('/') for f in test_files]
    support_files = [f.lstrip('/') for f in support_files]

    return string.Template(manifest_template).substitute({
        'caller': caller,
        'supportfiles': '\n'.join('  %s' % f for f in sorted(support_files)),
        'tests': '\n'.join('[%s]' % f for f in sorted(test_files))
    })


def substReftestList(caller, tests):
    def reftests(tests):
        return "\n".join(" ".join(line) for line in tests)

    return string.Template(reftest_template).substitute({
        "caller": caller,
        "reftests": reftests(tests),
    })


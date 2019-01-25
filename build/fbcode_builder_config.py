#!/usr/bin/env python
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

'fbcode_builder steps to build rsocket'

import specs.rsocket as rsocket


def fbcode_builder_spec(builder):
    return {
        'depends_on': [rsocket],
    }


config = {
    'github_project': 'rsocket/rsocket-cpp',
    'fbcode_builder_spec': fbcode_builder_spec,
}

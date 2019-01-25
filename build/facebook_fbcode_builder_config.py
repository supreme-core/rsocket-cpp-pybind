#!/usr/bin/env python
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals
'Facebook-specific additions to the fbcode_builder spec for rsocket'

config = read_fbcode_builder_config('fbcode_builder_config.py')  # noqa: F821
config['legocastle_opts'] = {
    'alias': 'rsocket-oss',
    'oncall': 'rsocket',
    'build_name': 'Open-source build for rsocket',
    'legocastle_os': 'ubuntu_16.04',
}

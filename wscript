#!/usr/bin/env python

APPNAME = 'WireCellNav'

def options(ctx):
    ctx.load('find_package')

def configure(ctx):
    ctx.load('find_package')


def build(bld):
    # main code library
    bld.shared_library()
#    bld.api_headers()
#    bld.rootcint_dictionary()

#    for testsrc in bld.path.ant_glob('test/test_*.cxx'):
#        bld.test_program([testsrc])



#!/usr/bin/env python
# encoding: utf-8

APPNAME = 'slash'
VERSION = '1.0'

def options(ctx):
    pass

def configure(ctx):
    ctx.check(header_name='termios.h', features='c cprogram', mandatory=False)
    ctx.env.SLASH_ENABLED = True

def build(ctx):
    ctx.objects(
        target   = APPNAME,
        source   = ['src/slash.c', 'src/slash_csp.c', 'src/base16.c'],
        includes = 'include',
        use = 'csp',
        export_includes = 'include')

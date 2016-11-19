#!/usr/bin/env python
# encoding: utf-8

def options(ctx):
    gr = ctx.add_option_group('slash options')
    gr.add_option('--slash-asf', action='store_true')
    gr.add_option('--slash-csp', action='store_true')

def configure(ctx):
    ctx.check(header_name='termios.h', features='c cprogram', mandatory=False)
    ctx.env.SLASH_ENABLED = True
    
    ctx.env.append_unique('FILES_SLASH', 'src/slash.c')
    ctx.env.append_unique('USE_SLASH', 'csp')
    
    if ctx.options.slash_asf:
        ctx.env.append_unique('FILES_SLASH', 'src/slash_asf.c')
        ctx.env.append_unique('USE_SLASH', 'asf')
        ctx.define('SLASH_ASF', '1')
        
    if ctx.options.slash_csp:
        ctx.env.append_unique('FILES_SLASH', 'src/base16.c')
        ctx.env.append_unique('FILES_SLASH', 'src/slash_csp.c')
        ctx.env.append_unique('USE_SLASH', 'csp')
        
    ctx.write_config_header('include/libslash.h')

def build(ctx):
    ctx.objects(
        target   = 'slash',
        source   = ctx.env.FILES_SLASH,
        includes = 'include',
        use = ctx.env.USE_SLASH,
        defines = ctx.env.DEFINES_SLASH,
        export_includes = 'include')

top = '.'
out = 'build'

APPNAME = 'mdns-announce'
VERSION = '0.0.0'

CXXFLAGS = []
CFLAGS = []
LDFLAGS = []

TOOLS = [
    'compiler_cxx',
    'compiler_c',
    'ar',
    'waf_unit_test',
    ]

def options(ctx):
    for tool in TOOLS:
        ctx.load(tool)

    ctx.add_option('--variant', action='store', default='debug',
            help='What variant to build ...')
    ctx.add_option('--compiler', action='store', default='clang',
            help='What compiler to build with.')

def configure(cfg):
    print('→ configuring the project in ' + cfg.path.abspath())

    cfg.env.VERSION = VERSION
    cfg.env.APPNAME = APPNAME

    if cfg.options.compiler == 'clang':
        cfg.env['CC'] = "clang"
        cfg.env['CXX'] = "clang++"

    for tool in TOOLS:
        cfg.load(tool)

    # Add another include path...
    cfg.env.append_value('INCLUDES', ['/usr/local/include'])

    # Check for C++11 support
    cfg.check_cxx_flag('-std=c++11', required=True)
    cfg.check_cxx_flag('-stdlib=libc++', required=False)
    cfg.env.append_value('LINKFLAGS', ['-stdlib=libc++'])

    cfg.check_cxx(
            lib='ev',
            header_name='ev++.h',
            msg='Checking for libev',
            )

    cfg.check_cxx(
            header_name='dns_sd.h',
            msg='Checking for dns_sd.h',
            )

    # This is untested, maybe it works, maybe it doesn't. Test it sometime =)
    cfg.check_cxx(
            lib='dns_sd',
            msg='Checking for dns_sd (Avahi compat layer)',
            define_name='AVAHI',
            mandatory=False
            )

    default_env = cfg.env.derive()
    default_env.detach()

    cfg.env.append_value('CXXFLAGS', CXXFLAGS)
    cfg.env.append_value('LINKFLAGS', LDFLAGS)
    cfg.env.append_value('CFLAGS', CFLAGS)

    cfg.check_cxx_flag('-Wall')
    cfg.check_cxx_flag('-Wextra')
    cfg.check_cxx_flag('-Werror')
    #cfg.check_c_flag('-Wall')
    #cfg.check_c_flag('-Wextra')
    #cfg.check_c_flag('-Werror')

    env = cfg.env
    env.detach()

    cfg.setenv('debug', env)
    cfg.env.append_value('CXXFLAGS', ['-g2'])
    cfg.define('DEBUG', 1)

    cfg.setenv('release', env)
    cfg.env.append_value('CXXFLAGS', ['-O3'])

    # Create a clean default environment
    cfg.setenv('default', default_env)

    cfg.recurse('src tests')


def build(ctx):
    print('→ build from ' + ctx.path.abspath())
    # Just build src for now, we don't have tests... yet
    ctx.recurse('src')

def init(ctx):
    from waflib.Build import BuildContext, CleanContext, InstallContext, UninstallContext

    for x in 'debug release'.split():
        for y in (BuildContext, CleanContext, InstallContext, UninstallContext):
            name = y.__name__.replace('Context','').lower()
            class tmp(y):
                cmd = name + '_' + x
                variant = x

    for y in (BuildContext, CleanContext, InstallContext, UninstallContext):
        class tmp(y):
            @property
            def variant(self):
                return ctx.options.variant

from waflib.Configure import conf
from waflib import Errors

@conf
def check_cxx_flag(ctx, flag, required=False):
    env = ctx.env
    env.stash()
    try:
        env.append_value('CXXFLAGS', flag)
        ctx.check_cxx(
                feature='cxx',
                cxxflags='-Werror',
                msg='Checking C++ compiler for flag: {}'.format(flag)
                )
        return True
    except:
        if required:
            raise Errors.WafError('Compiler flag "{}" is required'.format(flag))
        env.revert()
        return False


@conf
def check_c_flag(ctx, flag, required=False):
    env = ctx.env
    env.stash()
    try:
        env.append_value('CFLAGS', flag)
        ctx.check_cc(
                feature='c',
                cflags='-Werror',
                msg='Checking C compiler for flag: {}'.format(flag)
                )
        return True
    except:
        if required:
            raise Errors.WafError('Compiler flag "{}" is required'.format(flag))
        env.revert()
        return False

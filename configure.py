#!/usr/bin/python3
# File: configure.py
# Part of cards <github.com/rmkrupp/cards>
#
# Copyright (C) 2024 Noah Santer <n.ed.santer@gmail.com>
# Copyright (C) 2024 Rebecca Krupp <beka.krupp@gmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import argparse
import os
import sys
import subprocess
from datetime import datetime
import misc.ninja_syntax as ninja

#
# DIRECTLY INVOKED PROGRAMS
#

# used only if --defer-pkg-config=false
pkgconfig = {
        'release': 'pkg-config',
        'release-compat': 'pkg-config',
        'debug': 'pkg-config',
        'w64': 'x86_64-w64-mingw32-pkg-config'
    }

#
# ARGUMENT PARSING
#

parser = argparse.ArgumentParser(
        prog = 'configure.py',
        description = 'generate the build.ninja file',
        epilog = 'This program is part of cards <github.com/rmkrupp/cards>'
    )

parser.add_argument('--cflags', help='override compiler flags (and CFLAGS)')
parser.add_argument('--ldflags',
                    help='override compiler flags when linking (and LDFLAGS)')

parser.add_argument('--cc', help='override cc (and CC)')
parser.add_argument('--gperf', help='override gperf (and GPERF)')
parser.add_argument('--pkg-config',
                    help='override pkg-config (and PKG_CONFIG)')

parser.add_argument('--build',
                    choices=['release', 'standalone', 'debug', 'w64'],
                    default='debug',
                    help='set the build type (default: debug)')
parser.add_argument('--enable-compatible', action='store_true',
                    help='enable compatibility mode for older compilers')
parser.add_argument('--disable-sanitize', action='store_true',
                    help='don\'t enable the sanitizer in debug mode')
parser.add_argument('--build-native',
                    choices=['none', 'mtune', 'march', 'both'], default='none',
                    help='build with mtune=native or march=native')
parser.add_argument('--O3', '--o3', action='store_true',
                    help='build releases with -O3')

parser.add_argument('--disable-test-tool', action='append', default=[],
                    choices=[
                        'gperf_test', 'lex_test', 'hash_test',
                        'sorted_set_test', 'hash_test2', 'lex_test2'
                    ],
                    help='don\'t build a specific test tool')
parser.add_argument('--disable-tool', action='append', default=[],
                    choices=[
                        'cards_compile', 'cards_inspect',
                        'save_create', 'save_inspect'
                    ],
                    help='don\'t build a specific tool')
parser.add_argument('--disable-client', action='append', default=[],
                    choices=[
                        'cli', 'rlcli'
                    ],
                    help='don\'t build a specific client')
parser.add_argument('--disable-server', action='store_true',
                    help='don\'t build the server')

parser.add_argument('--lua-backend',
                    choices=['lua51', 'luajit', 'none'], default='luajit',
                    help='set the lua backend (default: luajit)')
parser.add_argument('--disable-argp', action='store_true',
                    help='fall back to getopt for argument parsing')

parser.add_argument('--disable-verbose-lexer', action='store_true',
                    help='don\'t enable the verbose lexer')

parser.add_argument('--force-version', metavar='STRING',
                    help='override the version string')
parser.add_argument('--add-version-suffix', metavar='SUFFIX',
                    help='append the version string')

# n.b. we aren't using BooleanOptionalAction for Debian reasons
parser.add_argument('--defer-git-describe', action='store_true', default=True,
                    help='run git describe when ninja is run, not configure.py (default)')
parser.add_argument('--no-defer-git-describe', action='store_false', dest='defer_git_describe',
                    help='run git describe when configure.py is run, not ninja')
parser.add_argument('--defer-pkg-config', action='store_true', default=True,
                    help='run pkg-config when ninja is run, not configure.py (default)')
parser.add_argument('--no-defer-pkg-config', action='store_false', dest='defer_pkg_config',
                    help='run pkg-config when configure.py is run, not ninja')

hash_opts = parser.add_argument_group('hash library options')
hash_opts.add_argument('--enable-hash-statistics', action = 'store_true',
                    help = 'compile with -DHASH_STATISTICS')
hash_opts.add_argument('--disable-hash-warnings', action = 'store_true',
                    help = 'compile with -DHASH_NO_WARNINGS')
hash_opts.add_argument('--enable-hash-simulate-failure', action = 'store_true',
                    help = 'compile with -DHASH_SIMULATE_FAILURE')

args = parser.parse_args()

#
# HELPER FUNCTIONS
#

def package(name,
            alias = None,
            pkg_config = True,
            libs = {},
            cflags = {},
            comment = True):
    libs = (libs.get(args.build, '') + ' ' + libs.get('all', '')).strip()
    if pkg_config and len(libs) > 0:
        libs = ' ' + libs

    cflags = (cflags.get(args.build, '') + ' ' + cflags.get('all', '')).strip()
    if pkg_config and len(cflags) > 0:
        cflags = ' ' + cflags

    if not alias:
        alias = name

    if comment:
        w.comment('package ' + name) 
    if pkg_config:
        if args.defer_pkg_config:
            w.variable(alias + '_cflags', '$$($pkgconfig --cflags ' + name + ')' + cflags)
            w.variable(alias + '_libs', '$$($pkgconfig --libs ' + name + ')' + libs)
        else:
            if args.build not in pkgconfig:
                print('ERROR: --defer-pkg-config is false but there is no pkg-config for build type', args.build, '(have:', pkgconfig, ')', file=sys.stderr)
                sys.exit(1)

            pc = pkgconfig.get(args.build)
            pc_cflags = subprocess.run([pc, '--cflags', name],
                                       capture_output = True)
            pc_libs = subprocess.run([pc, '--libs', name],
                                     capture_output = True)
            if pc_cflags.returncode != 0 or pc_libs.returncode != 0:
                w.comment(
                        'WARNING: ' + pc +
                        ' exited non-zero for library ' + name
                    )
                print('WARNING:', pc, 'exited non-zero for library',
                      name, file = sys.stderr)
            w.variable(alias + '_cflags',
                       pc_cflags.stdout.decode().strip() + cflags)
            w.variable(alias + '_libs',
                       pc_libs.stdout.decode().strip() + libs)
    else:
        w.variable(alias + '_cflags', cflags)
        w.variable(alias + '_libs', libs)
    w.newline()

def transformer(source, rule):
    if rule == 'cc':
        if source[-2:] == '.c':
            return source[:-2] + '.o'
    elif rule == 'gperf':
        if source[-6:] == '.gperf':
            return source[:-6] + '.c'
    return source

def build(source,
          rule = 'cc',
          input_prefix = 'src/',
          output_prefix = '$builddir/',
          packages = [],
          cflags = '$cflags',
          includes = '$includes'):

    variables = []
    cflags = ' '.join([cflags] + ['$' + name + '_cflags' for name in packages])
    if cflags != '$cflags':
        variables += [('cflags', cflags)]

    if includes != '$includes':
        variables += [('includes', includes)]

    w.build(
            output_prefix + transformer(source, rule),
            rule,
            input_prefix + source,
            variables = variables
        )

def exesuffix(root, enabled):
    if enabled:
        return root + '.exe'
    else:
        return root

def enable_debug():
    if args.enable_compatible:
        w.variable(key = 'std', value = '-std=gnu2x')
        w.variable(key = 'cflags', value = '$cflags $sanflags -g -Og')
        w.comment('adding compatibility defines because we were generated with --enable-compatible')
        w.variable(key = 'defines',
                   value = '$defines '+
                   '-Dconstexpr=const ' +
                   '"-Dstatic_assert(x)=" ' +
                   '-DENABLE_COMPAT')
    else:
        w.variable(key = 'std', value = '-std=gnu23')
        w.variable(key = 'cflags', value = '$cflags $sanflags -g -Og')

    if not args.force_version:
        w.variable(key = 'version', value = '"$version"-debug')
    else:
        w.comment('not appending -debug because we were generated with --force-version=')

def enable_release():
    if args.O3:
        w.comment('setting -O3 because we were generated with --O3')
        o = '-O3'
    else:
        o = '-O2'

    if args.enable_compatible:
        w.variable(key = 'std', value = '-std=gnu2x')
        w.variable(key = 'cflags', value = '$cflags ' + o)
        w.comment('adding compatibility defines because we were generated with --enable-compatible')
        w.variable(key = 'defines',
                   value = '$defines '+
                   '-Dconstexpr=const ' +
                   '"-Dstatic_assert(x)=" ' +
                   '-DENABLE_COMPAT')
    else:
        w.variable(key = 'std', value = '-std=gnu23')
        w.variable(key = 'cflags', value = '$cflags ' + o)

    w.variable(key = 'defines', value = '$defines -DNDEBUG')

def enable_w64():
    args.disable_argp = True
    w.variable(key = 'std', value = '-std=gnu2x')
    w.variable(key = 'cflags', value = '$cflags -O2 -static')
    if args.enable_compatible:
        w.comment('adding compatibility defines because we were generated with --enable-compatible')
        w.variable(key = 'defines',
                   value = '$defines '+
                   '-Dconstexpr=const ' +
                   '"-Dstatic_assert(x)=" ' +
                   '-DENABLE_COMPAT')
    w.variable(key = 'includes',
               value = '$includes -I/usr/x86_64-w64-mingw32/include')
    w.variable(key = 'defines', value = '$defines -DNDEBUG')

#
# THE WRITER
#

w = ninja.Writer(open('build.ninja', 'w'))

#
# PREAMBLE
#

w.comment('we were generated by configure.py on ' +
          datetime.now().strftime('%d-%m-%y at %H:%M:%S'))
w.comment('arguments: ' + str(sys.argv[1:]))
w.newline()

#
# BASE VERSION
#

if args.force_version:
    # this option also disables the -debug suffix in enable_debug()
    # but it does not disable --add_version_suffix
    w.comment('the following version was set at generation by --force-version='
              + args.force_version)
    w.variable(key = 'version', value = args.force_version)
else:
    if args.defer_git_describe:
        w.variable(key = 'version',
                   value = '$$(git describe --always --dirty)')
    else:
        git_describe = subprocess.run(
                ['git', 'describe', '--always', '--dirty'],
                capture_output = True
            )
        if git_describe.returncode != 0:
            w.comment('WARNING: git describe exited non-zero')
            print('WARNING: git describe exited non-zero', file = sys.stderr)
            w.variable(key = 'version', value = 'unknown')
        else:
            w.variable(key = 'version', value = git_describe.stdout.decode())

w.variable(key = 'builddir', value = 'out')

#
# TOOLS TO INVOKE
#

def warn_environment(key, argsval, flagname):
    if key in os.environ and argsval:
        print('WARNING:', key,
              'environment variable is set but will be ignored because',
              flagname, 'was passed', file = sys.stderr)
        w.comment('WARNING: ' + key +
                  ' environment variable is set but will be ignored because ' +
                  flagname + ' was passed')

warn_environment('CC', args.cc, '--cc=')
warn_environment('GPERF', args.cc, '--gperf=')
# TODO: is there a normal environment variable for pkg-config?
#warn_environment('PKG_CONFIG', args.cc, '--pkg-config=')
warn_environment('CFLAGS', args.cc, '--cflags=')
warn_environment('LDFLAGS', args.cc, '--ldflags=')

if args.cc:
    if (args.build == 'w64' and args.cc != 'x86_64-w64-mingw32-gcc') \
            or (args.build != 'w64' and args.cc != 'gcc'):
        w.comment('using this cc because we were generated with --cc=' +
                  args.cc)
    w.variable(key = 'cc', value = args.cc)
elif 'CC' in os.environ:
    w.comment('using this cc because CC was set')
    w.variable(key = 'cc', value = os.environ['cc'])
elif args.build == 'w64':
    w.variable(key = 'cc', value = 'x86_64-w64-mingw32-gcc')
else:
    w.variable(key = 'cc', value = 'gcc')

if args.gperf:
    if args.gperf != 'gperf':
        w.comment('using this gperf because we were generated with --gperf=' +
                  args.gperf)
    w.variable(key = 'gperf', value = args.gperf)
elif 'GPERF' in os.environ:
    w.comment('using this gperf because GPERF was set')
else:
    w.variable(key = 'gperf', value = 'gperf')

if args.pkg_config:
    w.comment('using this pkg-config because we were generated with --pkg-config=' +
              args.pkg_config)
    w.variable(key = 'pkgconfig', value = args.pkg_config)
elif args.build == 'w64':
    w.variable(key = 'pkgconfig', value = 'x86_64-w64-mingw32-pkg-config')
else:
    w.variable(key = 'pkgconfig', value = 'pkg-config')

#
# CFLAGS/LDFLAGS DEFAULTS
#

if args.cflags:
    w.comment('these are overriden below because we were generated with --cflags=' +
              args.cflags)
elif 'CFLAGS' in os.environ:
    w.comment('these are overriden below because CFLAGS was set')

w.variable(key = 'cflags',
           value = '-Wall -Wextra -Werror -fdiagnostics-color -flto')

if args.ldflags:
    w.comment('these are overriden below because we were generated with --ldflags=' +
              args.ldflags)
elif 'LDFLAGS' in os.environ:
    w.comment('these are overriden below because LDFLAGS was set')

w.variable(key = 'ldflags', value = '')

#
# MTUNE/MARCH SETTINGS
#

if args.build_native == 'none':
    pass
elif args.build_native == 'mtune':
    w.comment('# adding cflags for --build-native=mtune')
    w.variable(key = 'cflags', value = '$cflags -mtune=native')
elif args.build_native == 'march':
    w.comment('# adding cflags for --build-native=march')
    w.variable(key = 'cflags', value = '$cflags -march=native')
elif args.build_native == 'both':
    w.comment('# adding cflags for --build-native=both')
    w.variable(key = 'cflags', value = '$cflags -march=native -mtune=native')
else:
    print('WARNING: unhandled build-native mode "' + args.build_native + '"',
          file = sys.stderr)
    w.comment('WARNING: unhandled build-native mode "' +
              args.build_native + '"')

#
# SANITIZER
#

if args.build == 'w64':
    w.comment('-fsanitize disabled for w64 builds')
    w.variable(key = 'sanflags', value = '')
elif args.disable_sanitize:
    w.comment('-fsanitize disabled because we were generated with --disable-sanitize')
    w.variable(key = 'sanflags', value = '')
else:
    w.variable(key = 'sanflags', value = '-fsanitize=address,undefined')

#
# INCLUDES
#

w.variable(key = 'includes', value = '-Iinclude -Ilibs/hash/include')
w.newline()

#
# BUILD MODE
#

if args.build == 'debug':
    if args.O3:
        print('WARNING: ignoring option --O3 for debug build',
              file = sys.stderr)
        w.comment('WARNING: ignoring option --O3 for debug build')
    w.comment('build mode: debug')
    enable_debug()
elif args.build == 'release':
    w.comment('build mode: release')
    enable_release()
elif args.build == 'w64':
    w.comment('build mode: w64')
    w.comment('(this implies --disable-argp)')
    enable_w64()
else:
    print('WARNING: unhandled build mode "' + args.build + '"',
          file = sys.stderr)
    w.comment('WARNING: unhandled build mode "' + args.build +'"')
w.newline()

#
# THE VERBOSE LEXER
#

if args.disable_verbose_lexer:
    w.comment('the verbose lexer is not enabled because we were generated with --disable-verbose-lexer')
else:
    w.comment('the verbose lexer is enabled')
    w.variable(key = 'defines', value = '$defines -DVERBOSE_LEXER=1')
w.newline()

#
# --enable-hash-statistics
#
if args.enable_hash_statistics:
    w.comment('-DHASH_STATISTICS because we were generated with --enable-hash-statistics')
    w.variable('defines', '$defines -DHASH_STATISTICS')

#
# --disable-hash-warnings
#
if args.disable_hash_warnings:
    w.comment('-DHASH_NO_WARNINGS because we were generated with --disable-hash-warnings')
    w.variable('defines', '$defines -DHASH_NO_WARNINGS')

#
# --enable-hash-simulate-failure
#
if args.enable_hash_simulate_failure:
    w.comment('-DHASH_SIMULATE_FAILURE because we were generated with --enable-hash-simulate-failure')
    w.variable('defines', '$defines -DHASH_SIMULATE_FAILURE')

#
# CFLAGS/LDFLAGS OVERRIDES
#

needs_newline = False

if args.cflags:
    w.variable(key = 'cflags', value = args.cflags)
    needs_newline = True
elif 'CFLAGS' in os.environ:
    w.variable(key = 'cflags', value = os.environ['CFLAGS'])

if args.ldflags:
    w.variable(key = 'ldflags', value = args.ldflags)
    needs_newline = True
elif 'LDFLAGS' in os.environ:
    w.variable(key = 'ldflags', value = os.environ['LDFLAGS'])

#
# OPTIONAL VERSION SUFFIX
#

if args.add_version_suffix:
    w.variable(key = 'version',
               value = '"$version"-' + args.add_version_suffix)
    needs_newline = True

if needs_newline:
    w.newline()

#
# THE VERSION DEFINE
#

w.variable(key = 'defines', value = '$defines -DVERSION="\\"$version\\""')
w.newline()

#
# PACKAGES
#

package('sqlite3')
package('libevent', libs = {"w64": "-lws2_32 -liphlpapi"})
package('jansson')
package('unistring', pkg_config = False, libs = {
    'debug': '-lunistring',
    'release': '-lunistring',
    'release-compat': '-lunistring',
    'w64': '-lunistring -liconv'
})

#
# LUA BACKEND
#

if args.lua_backend == 'luajit':
    w.comment('lua is luajit')
    package('luajit', alias = 'lua',
            cflags = {'all': '-DUSE_LUAJIT=1'}, comment = False)
elif args.lua_backend == 'lua51':
    w.comment('lua is lua51')
    package('lua5.1', alias = 'lua',
            cflags = {'all': '-DUSE_LUAJIT=0'}, comment = False)
else:
    w.comment('lua is disabled')
    w.newline()

#
# NINJA RULES
#

w.rule(
        name = 'cc',
        deps = 'gcc',
        depfile = '$out.d',
        command = '$cc $std $includes -MMD -MF $out.d $defines ' +
                  '$cflags $in -c -o $out'
    )
w.newline()

w.rule(
        name = 'bin',
        deps = 'gcc',
        depfile = '$out.d',
        command = '$cc $std $includes -MMD -MF $out.d $defines ' +
                  '$cflags $in -o $out $ldflags $libs'
    )
w.newline()

w.rule(
        name = 'gperf',
        command = '$gperf --output-file=$out $in'
    )
w.newline()

#
# SOURCES
#

w.comment('source files')

build('bundle.c', packages = ['sqlite3'])
build('card.c', packages = ['lua'])
build('config_loader.c', packages = ['lua'])
build('game.c')
build('name_set.c', packages = ['unistring'])
build('main.c', packages = ['unistring', 'libevent'])
build('networker.c', packages = ['unistring'])
build('server.c')
w.newline()

build('util/log.c', packages = ['unistring'])
build('util/refstring.c', packages = ['unistring'])
build('util/sorted_set.c')
build('util/strdup.c')
build('util/checksum.c')
w.newline()

build('command/keyword.c', input_prefix = '$builddir/',
      cflags = '$cflags -Wno-missing-field-initializers -Wno-unused-parameter')
build('command/lex.c', packages = ['unistring'])
build('command/parse.c', packages = ['unistring'])
w.newline()

build('test/gperf_test.c')
build('test/hash_test.c')
build('test/lex_test.c', packages = ['unistring'])
build('test/sorted_set_test.c')
build('test/hash_test2.c')
build('test/lex_test2.c', packages = ['unistring'])
w.newline()

build('tools/cards_compile/cards_compile.c', packages = ['sqlite3'])
build('tools/cards_compile/args_getopt.c')
build('tools/cards_compile/args_argp.c',
      cflags = '$cflags -Wno-missing-field-initializers')
w.newline()

build('tools/cards_inspect/cards_inspect.c', packages = ['lua', 'sqlite3'])
build('tools/cards_inspect/args_getopt.c')
build('tools/cards_inspect/args_argp.c',
      cflags = '$cflags -Wno-missing-field-initializers')
w.newline()

build('tools/save_create/save_create.c', packages = ['sqlite3', 'jansson'])
build('tools/save_create/args_getopt.c')
build('tools/save_create/args_argp.c',
      cflags = '$cflags -Wno-missing-field-initializers')
w.newline()

build('tools/save_inspect/save_inspect.c', packages = ['sqlite3', 'jansson'])
build('tools/save_inspect/args_getopt.c')
build('tools/save_inspect/args_argp.c',
      cflags = '$cflags -Wno-missing-field-initializers')
w.newline()

build('client/cli/cli.c', packages = ['libevent'])
build('client/cli/args_getopt.c')
build('client/cli/args_argp.c',
      cflags = '$cflags -Wno-missing-field-initializers')
w.newline()

build('client/rlcli/rlcli.c', packages = ['libevent'],
      includes = '$includes -Ilibs/linenoise')
build('client/rlcli/args_getopt.c')
build('client/rlcli/args_argp.c',
      cflags = '$cflags -Wno-missing-field-initializers')
w.newline()

build('command/keyword.gperf', rule = 'gperf')
w.newline()

build('hash.c',
      input_prefix = 'libs/hash/src/', output_prefix = '$builddir/libs/hash/')
w.newline()

build('libs/linenoise/linenoise.c', input_prefix = '')
w.newline()

#
# OUTPUTS
#

w.comment('output products')

all_targets = []
tools_targets = []

def bin_target(name,
               inputs,
               argp_inputs = [],
               getopt_inputs = [],
               targets = [],
               variables = [],
               is_disabled = False,
               why_disabled = ''):
    fullname = exesuffix(name, args.build == 'w64')

    if type(is_disabled) == bool:
        is_disabled = [is_disabled]
        why_disabled = [why_disabled]

    assert(len(is_disabled) == len(why_disabled))

    if True not in is_disabled:
        if args.disable_argp and (len(argp_inputs) > 0 \
                or len(getopt_inputs) > 0):
            w.comment('# building ' + name + ' with getopt because we were generated with --disable-argp')
            inputs += getopt_inputs
        else:
            inputs += argp_inputs

        for group in targets:
            group += [fullname]
        w.build(fullname, 'bin', inputs, variables = variables)
    else:
        if sum([1 for disabled in is_disabled if disabled]) > 1:
            w.comment(fullname + ' is disabled because:')
            for disabled, why in zip(is_disabled, why_disabled):
                if disabled:
                    w.comment(' - ' + why)
        else:
            w.comment(fullname + ' is disabled because ' +
                      [why_disabled[x] for x in range(len(why_disabled))
                       if is_disabled[x]][0])
    w.newline()

bin_target(
        name = 'cards',
        inputs = [
            '$builddir/bundle.o',
            '$builddir/command/keyword.o',
            '$builddir/command/lex.o',
            '$builddir/command/parse.o',
            '$builddir/config_loader.o',
            '$builddir/name_set.o',
            '$builddir/main.o',
            '$builddir/networker.o',
            '$builddir/card.o',
            '$builddir/game.o',
            '$builddir/server.o',
            '$builddir/util/log.o',
            '$builddir/util/refstring.o',
            '$builddir/util/sorted_set.o',
            '$builddir/util/strdup.o',
            '$builddir/libs/hash/hash.o'
        ],
        variables = [
            ('libs', '$libevent_libs $lua_libs $unistring_libs $sqlite3_libs')
        ],
        is_disabled = [
            args.disable_server,
            args.lua_backend == 'none'
        ],
        why_disabled = [
            'we were generated with --disable-server',
            'we were generated with --lua-backend=none'
        ],
        targets = [all_targets]
    )

bin_target(
        name = 'test/gperf_test',
        inputs = [
            '$builddir/command/keyword.o',
            '$builddir/test/gperf_test.o'
        ],
        is_disabled = 'gperf_test' in args.disable_test_tool,
        why_disabled = 'we were generated with --disable-test-tool=gperf_test',
        targets = [all_targets, tools_targets]
    )

bin_target(
        name = 'test/lex_test',
        inputs = [
            '$builddir/command/lex.o',
            '$builddir/command/keyword.o',
            '$builddir/command/parse.o',
            '$builddir/name_set.o',
            '$builddir/bundle.o',
            '$builddir/game.o',
            '$builddir/card.o',
            '$builddir/test/lex_test.o',
            '$builddir/util/refstring.o',
            '$builddir/util/strdup.o',
            '$builddir/util/sorted_set.o',
            '$builddir/util/log.o',
            '$builddir/libs/hash/hash.o'
        ],
        variables = [('libs', '$sqlite3_libs $lua_libs $unistring_libs')],
        is_disabled = [
            args.lua_backend == 'none',
            'lex_test' in args.disable_test_tool
        ],
        why_disabled = [
            'we were generated with --lua-backend=none',
            'we were generated with --disable-test-tool=lex_test'
        ],
        targets = [all_targets, tools_targets]
    )

bin_target(
        name = 'clients/cli',
        inputs = [
            '$builddir/client/cli/cli.o',
            '$builddir/util/strdup.o'
        ],
        argp_inputs = [
            '$builddir/client/cli/args_argp.o'
        ],
        getopt_inputs = [
            '$builddir/client/cli/args_getopt.o'
        ],
        variables = [('libs', '$libevent_libs')],
        is_disabled = 'cli' in args.disable_client,
        why_disabled = 'we were generated with --disable-client=cli',
        targets = [all_targets, tools_targets]
    )

bin_target(
        name = 'clients/rlcli',
        inputs = [
            '$builddir/client/rlcli/rlcli.o',
            '$builddir/util/strdup.o',
            '$builddir/libs/linenoise/linenoise.o'
        ],
        argp_inputs = [
            '$builddir/client/rlcli/args_argp.o'
        ],
        getopt_inputs = [
            '$builddir/client/rlcli/args_getopt.o'
        ],
        variables = [('libs', '$libevent_libs')],
        is_disabled = [
            'rlcli' in args.disable_client,
            args.build == 'w64'
        ],
        why_disabled = [
            'we were generated with --disable-client=rlcli',
            'we were generated with --build=w64',
        ],
        targets = [all_targets, tools_targets]
    )

bin_target(
        name = 'test/hash_test',
        inputs = [
            '$builddir/test/hash_test.o',
            '$builddir/libs/hash/hash.o'
        ],
        is_disabled = 'hash_test' in args.disable_test_tool,
        why_disabled = 'we were generated with --disable-test-tool=hash_test',
        targets = [all_targets, tools_targets]
    )

bin_target(
        name = 'test/sorted_set_test',
        inputs = [
            '$builddir/test/sorted_set_test.o',
            '$builddir/util/sorted_set.o'
        ],
        is_disabled = 'sorted_set_test' in args.disable_test_tool,
        why_disabled =
            'we were generated with --disable-test-tool=sorted_set_test',
        targets = [all_targets, tools_targets]
    )

bin_target(
        name = 'test/lex_test2',
        inputs = [
            '$builddir/test/lex_test2.o',
            '$builddir/command/lex.o',
            '$builddir/command/keyword.o',
            '$builddir/name_set.o',
            '$builddir/card.o',
            '$builddir/libs/hash/hash.o',
            '$builddir/util/sorted_set.o',
            '$builddir/util/refstring.o',
            '$builddir/util/log.o'
        ],
        variables = [('libs', '$unistring_libs $lua_libs')],
        is_disabled = [
            'lex_test2' in args.disable_test_tool,
            args.lua_backend == 'none'
        ],
        why_disabled = [
            'we were generated with --disable-test-tool=lex_test2',
            'we were generated with --lua-backend=none'
        ],
        targets = [all_targets, tools_targets]
    )


bin_target(
        name = 'test/hash_test2',
        inputs = [
            '$builddir/test/hash_test2.o',
            '$builddir/libs/hash/hash.o',
            '$builddir/util/strdup.o'
        ],
        is_disabled = 'hash_test2' in args.disable_test_tool,
        why_disabled =
            'we were generated with --disable-test-tool=hash_test2',
        targets = [all_targets, tools_targets]
    )

bin_target(
        name = 'tools/cards_compile',
        inputs = [
            '$builddir/tools/cards_compile/cards_compile.o',
            '$builddir/util/strdup.o'
        ],
        argp_inputs = [
            '$builddir/tools/cards_compile/args_argp.o'
        ],
        getopt_inputs = [
            '$builddir/tools/cards_compile/args_getopt.o'
        ],
        variables = [('libs', '$sqlite3_libs')],
        is_disabled = 'cards_compile' in args.disable_tool,
        why_disabled = 'we were generated with --disable-tool=cards_compile',
        targets = [all_targets, tools_targets]
    )

bin_target(
        name = 'tools/cards_inspect',
        inputs = [
            '$builddir/tools/cards_inspect/cards_inspect.o',
            '$builddir/util/strdup.o'
        ],
        argp_inputs = [
            '$builddir/tools/cards_inspect/args_argp.o'
        ],
        getopt_inputs = [
            '$builddir/tools/cards_inspect/args_getopt.o'
        ],
        variables = [('libs', '$sqlite3_libs $lua_libs')],
        is_disabled = [
            args.lua_backend == 'none',
            'cards_inspect' in args.disable_tool
        ],
        why_disabled = [
            'we were generated with --lua-backend=none',
            'we were generated with --disable-tool=cards_inspect'
        ],
        targets = [all_targets, tools_targets]
    )

bin_target(
        name = 'tools/save_create',
        inputs = [
            '$builddir/tools/save_create/save_create.o',
            '$builddir/util/strdup.o',
            '$builddir/util/checksum.o',
            '$builddir/util/sorted_set.o'
        ],
        argp_inputs = [
            '$builddir/tools/save_create/args_argp.o'
        ],
        getopt_inputs = [
            '$builddir/tools/save_create/args_getopt.o'
        ],
        variables = [('libs', '$sqlite3_libs $jansson_libs')],
        is_disabled = 'save_create' in args.disable_tool,
        why_disabled = 'we were generated with --disable-tool=save_create',
        targets = [all_targets, tools_targets]
    )

bin_target(
        name = 'tools/save_inspect',
        inputs = [
            '$builddir/tools/save_inspect/save_inspect.o',
            '$builddir/util/strdup.o',
            '$builddir/util/checksum.o',
            '$builddir/util/sorted_set.o'
        ],
        argp_inputs = [
            '$builddir/tools/save_inspect/args_argp.o'
        ],
        getopt_inputs = [
            '$builddir/tools/save_inspect/args_getopt.o'
        ],
        variables = [('libs', '$sqlite3_libs $jansson_libs')],
        is_disabled = 'save_inspect' in args.disable_tool,
        why_disabled = 'we were generated with --disable-tool=save_inspect',
        targets = [all_targets, tools_targets]
    )
#
# ALL, TOOLS, AND DEFAULT
#

if len(tools_targets) > 0:
    w.build('tools', 'phony', tools_targets)
else:
    w.comment('NOTE: no tools target because there are no enabled tools')
w.newline()

w.build('all', 'phony', all_targets)
w.newline()

w.default('all')

#
# DONE
#

w.close()

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
# ARGUMENT PARSING
#

parser = argparse.ArgumentParser(
        prog = 'configure.py',
        description = 'generate the build.ninja file',
        epilog = 'This program is part of cards <github.com/rmkrupp/cards>'
    )

parser.add_argument('--cflags', help='override compiler flags')
parser.add_argument('--cc', help='override cc')
parser.add_argument('--gperf', help='override gperf')
parser.add_argument('--ldflags', help='override compiler flags when linking')
#parser.add_argument('--prefix')
#parser.add_argument('--destdir')
parser.add_argument('--build',
                    choices=['release', 'debug', 'w64'], default='debug',
                    help='set the build type (default: debug)')
parser.add_argument('--build-native',
                    choices=['none', 'mtune', 'march', 'both'], default='none',
                    help='build with mtune=native or march=native')
parser.add_argument('--O3', '--o3', action='store_true',
                    help='build releases with -O3')
parser.add_argument('--lua-backend',
                    choices=['lua51', 'luajit', 'none'], default='luajit',
                    help='set the lua backend (default: luajit)')
parser.add_argument('--disable-test-tool', action='append', default=[],
                    choices=[
                        'gperf_test', 'lex_test', 'hash_test'
                    ],
                    help='don\'t build a specific test tool')
parser.add_argument('--disable-tool', action='append', default=[],
                    choices=[
                        'cards_compile', 'cards_inspect'
                    ],
                    help='don\'t build a specific tool')
parser.add_argument('--disable-client', action='append', default=[],
                    choices=[
                        'cli', 'rlcli'
                    ],
                    help='don\'t build a specific client')
parser.add_argument('--disable-server', action='store_true',
                    help='don\'t build the server')
parser.add_argument('--disable-readline', action='store_true',
                    help='don\'t build anything dependent on readline')
parser.add_argument('--disable-argp', action='store_true',
                    help='fall back to getopt for argument parsing')
parser.add_argument('--disable-sanitize', action='store_true',
                    help='don\'t enable the sanitizer in debug mode')
parser.add_argument('--disable-verbose-lexer', action='store_true',
                    help='don\'t enable the verbose lexer')
parser.add_argument('--force-version', metavar='STRING',
                    help='override the version string')
parser.add_argument('--add-version-suffix', metavar='SUFFIX',
                    help='append the version string')

hash_opts = parser.add_argument_group('hash library options')
hash_opts.add_argument('--enable-hash-statistics', action='store_true',
                    help='compile with -DHASH_STATISTICS')
hash_opts.add_argument('--disable-hash-warnings', action='store_true',
                    help='compile with -DHASH_NO_WARNINGS')
hash_opts.add_argument('--enable-hash-simulate-failure', action='store_true',
                    help='compile with -DHASH_SIMULATE_FAILURE')

args = parser.parse_args()

#
# HELPER FUNCTIONS
#

def exesuffix(root, enabled):
    if enabled:
        return root + '.exe'
    else:
        return root

def enable_debug():
    w.variable(key = 'std', value = '-std=gnu23')
    w.variable(key = 'cflags', value = '$cflags $sanflags -g -Og')
    if not args.force_version:
        w.variable(key = 'version', value = '"$version"-debug')
    else:
        w.comment('not appending -debug because we were generated with --force-version=')

def enable_release():
    w.variable(key = 'std', value = '-std=gnu23')
    if (args.O3):
        w.comment('setting -O3 because we were generated with --O3')
        w.variable(key = 'cflags', value = '$cflags -O3')
    else:
        w.variable(key = 'cflags', value = '$cflags -O2')
    w.variable(key = 'defines', value = '$defines -DNDEBUG')

def enable_w64():
    args.disable_argp = True
    w.variable(key = 'std', value = '-std=gnu2x')
    w.variable(key = 'cflags', value = '$cflags -O2 -static -I/usr/x86_64-w64-mingw32/include')
    w.variable(key = 'w64netlibs', value = '-lws2_32 -liphlpapi')
    w.variable(key = 'w64curses', value = '-lcurses')
    w.variable(key = 'ldflags', value = '$ldflags -L/usr/x86_64-w64-mingw32/lib')
    w.variable(key = 'defines', value = '$defines -DNDEBUG')

def enable_luajit():
    w.variable(key = 'defines', value = '$defines -DUSE_LUAJIT=1')
    if args.build == 'w64':
        w.variable(key = 'lualib', value = '-lluajit')
    else:
        w.variable(key = 'lualib', value = '-lluajit-5.1')

def enable_lua51():
    w.variable(key = 'defines', value = '$defines -DUSE_LUAJIT=0')
    w.variable(key = 'lualib', value = '-llua5.1')

def enable_verbose_lexer():
    w.variable(key = 'defines', value = '$defines -DVERBOSE_LEXER=1')

#
# THE WRITER
#

w = ninja.Writer(open('build.ninja', 'w'))

#
# PREAMBLE
#

w.comment('we were generated by configure.py on ' + datetime.now().strftime('%d-%m-%y %H:%M:%S'))
w.comment('arguments: ' + str(sys.argv[1:]))
w.newline()

#
# BASE VERSION
#

if args.force_version:
    # this option also disables the -debug suffix in enable_debug()
    # but it does not disable --add_version_suffix
    w.comment('the following version was set at generation by --force-version=' + args.force_version)
    w.variable(key = 'version', value = args.force_version)
else:
    w.variable(key = 'version', value = '$$(git describe --always --dirty)')

w.variable(key = 'builddir', value = 'out')

#
# TOOLS TO INVOKE
#

if 'CC' in os.environ:
    print('WARNING: CC environment variable is set but will be ignored (did you mean --cc=?)',
          file=sys.stderr)

if args.cc:
    if (args.build == 'w64' and args.cc != 'x86_64-w64-mingw32-gcc') or (args.build != 'w64' and args.cc != 'gcc'):
        w.comment('using this cc because we were generated with --cc=' + args.cc)
    w.variable(key = 'cc', value = args.cc)
elif args.build == 'w64':
    w.variable(key = 'cc', value = 'x86_64-w64-mingw32-gcc')
else:
    w.variable(key = 'cc', value = 'gcc')

if args.gperf:
    if args.gperf != 'gperf':
        w.comment('using this gperf because we were generated with --gperf=' + args.gperf)
    w.variable(key = 'gperf', value = args.gperf)
else:
    w.variable(key = 'gperf', value = 'gperf')

#
# CFLAGS/LDFLAGS DEFAULTS
#

if args.cflags:
    w.comment('these are overriden below because we were generated with --cflags=' + args.cflags)
w.variable(key = 'cflags', value = '-Wall -Wextra -Werror -fdiagnostics-color -flto')

if args.ldflags:
    w.comment('these are overriden below because we were generated with --ldflags=' + args.ldflags)
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
    print('WARNING: unhandled build-native mode "' + args.build_native + '"', file=sys.stderr)
    w.comment('WARNING: unhandled build-native mode "' + args.build_native +'"')

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
        print('WARNING: ignoring option --O3 for debug build', file=sys.stderr)
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
    print('WARNING: unhandled build mode "' + args.build + '"', file=sys.stderr)
    w.comment('WARNING: unhandled build mode "' + args.build +'"')
w.newline()

#
# LUA MODE
#

if args.lua_backend == 'luajit':
    w.comment('lua is luajit')
    enable_luajit()
elif args.lua_backend == 'lua51':
    w.comment('lua is lua51')
    enable_lua51()
else:
    w.comment('lua is disabled')
    w.comment('(this implies --disable-server and --disable-tool=cards_inspect)')
    args.disable_server = True
w.newline()

#
# THE VERBOSE LEXER
#

if args.disable_verbose_lexer:
    w.comment('the verbose lexer is not enabled because we were generated with --disable-verbose-lexer')
else:
    w.comment('the verbose lexer is enabled')
    enable_verbose_lexer()
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

if args.ldflags:
    w.variable(key = 'ldflags', value = args.ldflags)
    needs_newline = True

#
# OPTIONAL VERSION SUFFIX
#

if args.add_version_suffix:
    w.variable(key = 'version', value = '"$version"-' + args.add_version_suffix)
    needs_newline = True

if needs_newline:
    w.newline()

#
# THE VERSION DEFINE
#

w.variable(key = 'defines', value = '$defines -DVERSION="\\"$version\\""')
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

w.build('$builddir/config_loader.o', 'cc', 'src/config_loader.c')
w.build('$builddir/main.o', 'cc', 'src/main.c')
w.build('$builddir/networker.o', 'cc', 'src/networker.c')
w.build('$builddir/server.o', 'cc', 'src/server.c')
w.build('$builddir/loader.o', 'cc', 'src/loader.c')
w.newline()

w.build('$builddir/util/log.o', 'cc', 'src/util/log.c')
w.build('$builddir/util/refstring.o', 'cc', 'src/util/refstring.c')
w.build('$builddir/util/strdup.o', 'cc', 'src/util/strdup.c')
w.build('$builddir/util/sorted_set.o', 'cc', 'src/util/sorted_set.c')
w.newline()

w.build('$builddir/command/keyword.o', 'cc', 'out/command/keyword.c',
        variables=[
            ('cflags',
             '$cflags -Wno-missing-field-initializers ' +
             '-Wno-unused-parameter')
        ])

w.build('$builddir/command/lex.o', 'cc', 'src/command/lex.c')
w.build('$builddir/command/parse.o', 'cc', 'src/command/parse.c')
w.newline()

w.build('$builddir/test/gperf_test.o', 'cc', 'src/test/gperf_test.c')
w.build('$builddir/test/lex_test.o', 'cc', 'src/test/lex_test.c')
w.build('$builddir/test/hash_test.o', 'cc', 'src/test/hash_test.c')

w.build('$builddir/tools/cards_compile/cards_compile.o', 'cc',
        'src/tools/cards_compile/cards_compile.c')
w.build('$builddir/tools/cards_compile/args_getopt.o', 'cc',
        'src/tools/cards_compile/args_getopt.c')
w.build('$builddir/tools/cards_compile/args_argp.o', 'cc',
        'src/tools/cards_compile/args_argp.c',
        variables=[('cflags', '$cflags -Wno-missing-field-initializers')])

w.build('$builddir/tools/cards_inspect/cards_inspect.o', 'cc',
        'src/tools/cards_inspect/cards_inspect.c')
w.build('$builddir/tools/cards_inspect/args_getopt.o', 'cc',
        'src/tools/cards_inspect/args_getopt.c')
w.build('$builddir/tools/cards_inspect/args_argp.o', 'cc',
        'src/tools/cards_inspect/args_argp.c',
        variables=[('cflags', '$cflags -Wno-missing-field-initializers')])

w.build('$builddir/client/cli/cli.o', 'cc', 'src/client/cli/cli.c')
w.build('$builddir/client/cli/args_getopt.o', 'cc',
        'src/client/cli/args_getopt.c')
w.build('$builddir/client/cli/args_argp.o', 'cc', 'src/client/cli/args_argp.c',
        variables=[('cflags', '$cflags -Wno-missing-field-initializers')])
w.newline()

w.build('$builddir/client/rlcli/rlcli.o', 'cc', 'src/client/rlcli/rlcli.c')
w.build('$builddir/client/rlcli/args_getopt.o', 'cc',
        'src/client/rlcli/args_getopt.c')
w.build('$builddir/client/rlcli/args_argp.o', 'cc',
        'src/client/rlcli/args_argp.c',
        variables=[('cflags', '$cflags -Wno-missing-field-initializers')])
w.newline()

w.build('$builddir/command/keyword.c', 'gperf', 'src/command/keyword.gperf')
w.newline()

w.build('$builddir/libs/hash/hash.o', 'cc', 'libs/hash/src/hash.c')
w.newline()
#
# OUTPUTS
#

all_targets = []
tools_targets = []

def bin_target(name,
               inputs,
               argp_inputs=[],
               getopt_inputs=[],
               targets=[],
               variables=[],
               is_disabled=False,
               why_disabled=''):
    fullname = exesuffix(name, args.build == 'w64')

    if type(is_disabled) == bool:
        is_disabled = [is_disabled]
        why_disabled = [why_disabled]

    assert(len(is_disabled) == len(why_disabled))

    if True not in is_disabled:
        if args.disable_argp and (len(argp_inputs) > 0 or len(getopt_inputs) > 0):
            w.comment('# building ' + name + ' with getopt because we were generated with --disable-argp')
            inputs += getopt_inputs
        else:
            inputs += argp_inputs

        for group in targets:
            group += [fullname]
        w.build(fullname, 'bin', inputs, variables=variables)
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
            '$builddir/command/keyword.o',
            '$builddir/command/lex.o',
            '$builddir/command/parse.o',
            '$builddir/config_loader.o',
            '$builddir/main.o',
            '$builddir/networker.o',
            '$builddir/server.o',
            '$builddir/loader.o',
            '$builddir/util/log.o',
            '$builddir/util/refstring.o',
            '$builddir/util/strdup.o',
            '$builddir/util/sorted_set.o',
            '$builddir/libs/hash/hash.o'
        ],
        variables = [('libs', '-levent $lualib $w64netlibs')],
        is_disabled = args.disable_server,
        why_disabled = 'we were generated with --disable-server',
        targets = [all_targets]
    )

bin_target(
        name = 'test/gperf_test',
        inputs = [
            '$builddir/command/keyword.o',
            '$builddir/test/gperf_test.o'
        ],
        variables = [('libs', '')],
        is_disabled = 'gperf_test' in args.disable_test_tool,
        why_disabled = 'we were generated with --disable-test-tool=gperf_test',
        targets = [all_targets, tools_targets]
    )

bin_target(
        name = 'test/lex_test',
        inputs = [
            '$builddir/command/lex.o',
            '$builddir/command/keyword.o',
            '$builddir/loader.o',
            '$builddir/test/lex_test.o',
            '$builddir/util/refstring.o',
            '$builddir/util/strdup.o',
            '$builddir/util/sorted_set.o',
            '$builddir/libs/hash/hash.o'
        ],
        variables = [('libs', '')],
        is_disabled = 'lex_test' in args.disable_test_tool,
        why_disabled = 'we were generated with --disable-test-tool=lex_test',
        targets = [all_targets, tools_targets]
    )

if args.disable_argp:
    w.comment('# building cli with getopt because we were generated with --disable-argp')
    cli_args_input = [ '$builddir/client/cli/args_getopt.o' ]
else:
    cli_args_input = [ '$builddir/client/cli/args_argp.o' ]

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
        variables = [('libs', '-levent $w64netlibs')],
        is_disabled = 'cli' in args.disable_client,
        why_disabled = 'we were generated with --disable-client=cli',
        targets = [all_targets, tools_targets]
    )

bin_target(
        name = 'clients/rlcli',
        inputs = [
            '$builddir/client/rlcli/rlcli.o',
            '$builddir/util/strdup.o'
        ],
        argp_inputs = [
            '$builddir/client/rlcli/args_argp.o'
        ],
        getopt_inputs = [
            '$builddir/client/rlcli/args_getopt.o'
        ],
        variables = [
            ('libs', '-levent $w64netlibs -lreadline $w64curses'),
            ('cflags', '$cflags -pthread')
        ],
        is_disabled = [
            'rlcli' in args.disable_client,
            args.disable_readline
        ],
        why_disabled = [
            'we were generated with --disable-client=rlcli',
            'we were generated with --disable-readline'
        ],
        targets = [all_targets, tools_targets]
    )

bin_target(
        name = 'test/hash_test',
        inputs = [
            '$builddir/test/hash_test.o',
            '$builddir/libs/hash/hash.o'
        ],
        variables = [
            ('libs', '')
        ],
        is_disabled = 'hash_test' in args.disable_test_tool,
        why_disabled = 'we were generated with --disable-test-tool=hash_test',
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
        variables = [
            ('libs', '-lsqlite3')
        ],
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
        variables = [
            ('libs', '-lsqlite3 $lualib')
        ],
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

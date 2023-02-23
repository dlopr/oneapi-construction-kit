#!/usr/bin/env python

# Copyright (C) Codeplay Software Limited. All Rights Reserved.

# Imports.
import argparse


def setParseArguments():
    """ Defines then returns an arguments parser for the coverage script.

        Returns:
            arg_parser (namespace): arguments parser containing all the flags.
    """

    arg_parser = argparse.ArgumentParser(description='Code Coverage Checker')
    requ_group = arg_parser.add_mutually_exclusive_group(required=True)

    arg_parser.add_argument(
        '--run-lcov',
        action='store_true',
        default=False,
        dest='run_lcov',
        help='Run lcov coverage')

    arg_parser.add_argument(
        '--no-branches',
        action='store_true',
        default=False,
        dest='no_branches',
        help='Disable branches coverage checking')

    arg_parser.add_argument(
        '--no-functions',
        action='store_true',
        default=False,
        dest='no_functions',
        help='Disable functions coverage checking')

    arg_parser.add_argument(
        '--no-module-reporting',
        action='store_true',
        default=False,
        dest='no_module_reporting',
        help='Disable reporting of modules on stdout')

    arg_parser.add_argument(
        '--no-test-suite-reporting',
        action='store_true',
        default=False,
        dest='no_test_suite_reporting',
        help='Disable reporting of test suites on stdout')

    arg_parser.add_argument(
        '-j'
        '--threads',
        default=None,
        dest='threads',
        type=int,
        metavar='N',
        help='Specify the number of threads to involve')

    arg_parser.add_argument(
        '-c',
        '--check-regression',
        dest='check_regression',
        default='',
        metavar='FILE|DIRECTORY',
        help='Specify a file/folder to check regression')

    arg_parser.add_argument(
        '-e',
        '--exclude-modules',
        nargs='+',
        default=[],
        dest='exclude_modules',
        metavar=('MODULE'),
        help='Modules or submodules to exclude')

    arg_parser.add_argument(
        '-f',
        '--cover-filter',
        nargs='+',
        default=[],
        dest='cover_filter',
        metavar=('REGEXP'),
        help='Coverage tests cases to ignore')

    arg_parser.add_argument(
        '-i',
        '--no-intermediate-files',
        action='store_true',
        default=False,
        dest='no_intermediate_files',
        help='Disable intermediate output files')

    arg_parser.add_argument(
        '--junit-xml-output',
        default='',
        dest='junit_xml_output',
        metavar='PATH_TO_FILE',
        help='Generate a junit xml summary of coverage')

    arg_parser.add_argument(
        '-l',
        '--lcov-html-output',
        dest='lcov_html_output',
        action='store_true',
        default=False,
        help='Generate an html summary of coverage')

    requ_group.add_argument(
        '-m',
        '--module',
        nargs=2,
        dest='modules',
        action='append',
        default=[],
        metavar=('PATH_TO_SOURCES', 'PATH_TO_OBJECTS'),
        help='Specify a module to cover')

    arg_parser.add_argument(
        '-o',
        '--output-directory',
        dest='output_directory',
        default=None,
        type=str,
        metavar='PATH_TO_DIRECTORY',
        help='Directory to output total output files')

    arg_parser.add_argument(
        '-p',
        '--percentage',
        dest='percentage',
        default='100',
        type=int,
        metavar='N',
        help='Minimum for a test to be tagged as passed')

    arg_parser.add_argument(
        '-q',
        '--quiet',
        dest='quiet',
        action='store_true',
        default=False,
        help='Display nothing on stdout')

    arg_parser.add_argument(
        '-r',
        '--recursive',
        dest='recursive',
        action='store_true',
        default=False,
        help='Recursively perform test submodules')

    arg_parser.add_argument(
        '-s',
        '--csv-output',
        dest='csv_output',
        action='store',
        default='',
        type=str,
        metavar='PATH_TO_FILE',
        help='Generate a csv file summary of coverage')

    requ_group.add_argument(
        '-t',
        '--test-suite',
        help='Path to the test suite then flags',
        nargs='+',
        action='append',
        dest='test_suites',
        default=[],
        metavar=('PATH_TO_TEST_SUITE', 'FLAG'))

    arg_parser.add_argument(
        '-v',
        '--verbose',
        dest='verbose',
        action='store_true',
        default=False,
        help='More information displayed')

    requ_group.add_argument(
        '-x',
        '--xml-input',
        dest='xml_input',
        default='',
        metavar='PATH_TO_FILE',
        help='Specify an xml input')

    return arg_parser

#!/bin/python3

import unittest
from unittest.mock import patch

import os
import xml.etree.ElementTree as ET
import argparse
from enum import Enum

class ErrorType(Enum):
    ERROR = 1
    WARNING = 2
    STYLE = 3
    PERFORMANCE = 4
    PORTABILITY = 5
    INFORMATION = 6
    UNKNOWN = 7

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("-f", "--file", help="cppcheck xml file", default="cppcheck.xml")
    parser.add_argument("--error-threshold", type=int, help="error threshold", default=4)
    parser.add_argument("--warning-threshold", type=int, help="warning threshold", default=23)
    args = parser.parse_args()
    return args

def classify_error(error: ET.Element):
    severity = error.attrib['severity']
    if severity == 'error':
        return ErrorType.ERROR
    elif severity == 'warning':
        return ErrorType.WARNING
    elif severity == 'style':
        return ErrorType.STYLE
    elif severity == 'performance':
        return ErrorType.PERFORMANCE
    elif severity == 'portability':
        return ErrorType.PORTABILITY
    elif severity == 'information':
        return ErrorType.INFORMATION
    else:
        return ErrorType.UNKNOWN

def count_error_types(root: ET.Element):
    # Count types of errors
    error_types = {ErrorType.ERROR: 0, ErrorType.WARNING: 0, ErrorType.STYLE: 0, ErrorType.PERFORMANCE: 0, ErrorType.PORTABILITY: 0, ErrorType.INFORMATION: 0, ErrorType.UNKNOWN: 0}
    for error in root.findall('errors/error'):
        classified_error = classify_error(error)
        error_types[classified_error] += 1
    return error_types

def main():
    args = parse_args()

    if not os.path.exists(args.file):
        print("File not found: {}".format(args.file))
        return

    tree = ET.parse(args.file)
    root = tree.getroot()

    error_types = count_error_types(root)
    
    # Quality gate
    exit_code = 0
    if error_types[ErrorType.ERROR] > args.error_threshold:
        print("Too many errors: {}".format(error_types[ErrorType.ERROR]))
        exit_code = 1
    if error_types[ErrorType.WARNING] > args.warning_threshold:
        print("Too many warnings: {}".format(error_types[ErrorType.WARNING]))
        exit_code = 1
    print("--------------------")
    print("Errors: {}".format(error_types[ErrorType.ERROR]))
    print("Warnings: {}".format(error_types[ErrorType.WARNING]))
    print("Style: {}".format(error_types[ErrorType.STYLE]))
    print("Performance: {}".format(error_types[ErrorType.PERFORMANCE]))
    print("Portability: {}".format(error_types[ErrorType.PORTABILITY]))
    print("Information: {}".format(error_types[ErrorType.INFORMATION]))
    print("Unknown: {}".format(error_types[ErrorType.UNKNOWN]))
    print("--------------------")
    if exit_code == 0:
        print("Quality gate passed")
    return exit_code

if __name__ == "__main__":
    _exit_code = main()
    exit(_exit_code)

# Testing
class TestParsingCPPCheck(unittest.TestCase):
    def test_parse_cppcheck(self):
        tree = ET.parse('test_asset/cppcheck-result.xml')
        root = tree.getroot()
        self.assertEqual(root.tag, 'results')
        self.assertEqual(root.attrib['version'], '2')
        self.assertEqual(len(root.findall('errors/error')), 141)

    def test_classify_error(self):
        tree = ET.parse('test_asset/cppcheck-result.xml')
        root = tree.getroot()
        error = root.findall('errors/error')[0]
        print(error)
        self.assertEqual(classify_error(error), ErrorType.WARNING)

    @patch('argparse.ArgumentParser.parse_args', return_value=argparse.Namespace(file='test_asset/cppcheck-result.xml'))
    def test_main(self, mock_args):
        mock_args.return_value = argparse.Namespace(file='test_asset/cppcheck-result.xml', error_threshold=4, warning_threshold=23)
        exit_code = main()
        self.assertEqual(exit_code, 0)

    def test_count_error_types(self):
        tree = ET.parse('test_asset/cppcheck-result.xml')
        root = tree.getroot()
        error_types = count_error_types(root)
        self.assertEqual(error_types[ErrorType.ERROR], 4)
        self.assertEqual(error_types[ErrorType.WARNING], 23)
        self.assertEqual(error_types[ErrorType.STYLE], 86)
        self.assertEqual(error_types[ErrorType.PERFORMANCE], 20)
        self.assertEqual(error_types[ErrorType.PORTABILITY], 2)
        self.assertEqual(error_types[ErrorType.INFORMATION], 6)
        self.assertEqual(error_types[ErrorType.UNKNOWN], 0) 





    

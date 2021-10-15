#!/usr/bin/env python3

import json
import sys

def run_compare(report):
    with open(report) as f:
        doc = json.load(f)

    for testcase in doc:
        measurements = testcase['measurements']
        time = float(measurements[0]["time"])
        if time < 0:
            continue
        if time > 0.05:
            print("More than 5% performance decrease, considering it a failure")
            sys.exit(2)


def main(argv):
    if len(argv) != 2:
        print(f'Usage: {argv[0]} <path-to-compare.json>')
        sys.exit(1)
    run_compare(argv[1])

if __name__ == '__main__':
    main(sys.argv)

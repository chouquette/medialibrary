#!/usr/bin/env python3

import json
import sys

def run_compare(report):
    with open(report) as f:
        doc = json.load(f)

    decrease = 0
    increase = 0

    for testcase in doc:
        if len(testcase['aggregate_name']) == 0:
            if len(testcase['measurements']) > 1:
                # More than a single measurement, waiting for the aggregate results
                continue
            # fall through, measurements describes a single repetition test
        elif testcase['aggregate_name'] != "mean":
            # We are interested in the mean aggregate, ignore the others
            continue

        # Now we either have an aggregated mean result, or a single rep measurement
        m = testcase['measurements'][0]
        time = float(m["time"])
        if time < -0.05:
            print(f"Performance increase detected for test {testcase['name']}", file=sys.stderr)
            increase += 1
        elif time > 0.05:
            print(f"Performance decrease detected for test {testcase['name']}", file=sys.stderr)
            decrease += 1
        else:
            print(f"No major performance change detected for test {testcase['name']}", file=sys.stderr)

    if decrease > 0 or increase > 0:
        print(f"{increase} tests with improved performance ; {decrease} with performance decrease")
        if decrease > 0:
            sys.exit(2)
    else:
        print("No performance change")

def main(argv):
    if len(argv) != 2:
        print(f'Usage: {argv[0]} <path-to-compare.json>')
        sys.exit(1)
    run_compare(argv[1])

if __name__ == '__main__':
    main(sys.argv)

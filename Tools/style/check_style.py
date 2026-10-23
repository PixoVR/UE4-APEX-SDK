#!/usr/bin/env python3
"""Checks the style rules clang-format and clang-tidy cannot express.

    check_style.py [--fix] [FILE...]     (default: every C and C++ file we own)

Rules (see AGENTS.md):
  1. After the closing brace of a conditional or loop block, a blank line comes
     before any further code. Exceptions: the next line closes the enclosing
     block, or continues the statement (else, catch, the while of a do-while).
  2. Indentation uses tabs, never spaces.
  3. File names are PascalCase, named after the class or contents they hold
     (TelemetryBuffer.cpp, not telemetry_buffer.cpp).

Exits non-zero when a rule is broken; --fix inserts the missing blank lines.
"""

import argparse
import os
import re
import subprocess
import sys

REPOSITORY_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
OWNED_DIRECTORIES = ("Source",)
# Vendored apex-sdk-core, copied in by Tools/update_core.py, keeps its own checks.
VENDORED_PREFIX = "Source/ThirdParty/"
SOURCE_EXTENSIONS = (".c", ".cpp", ".h", ".hpp")
PASCAL_CASE_FILE_NAME = re.compile(r"^[A-Z][A-Za-z0-9]*\.")
CONTROL_KEYWORDS = re.compile(r"^(if|else|for|while|switch|do|try|catch)\b")
CONTINUATIONS = re.compile(r"^(}|else\b|catch\b|while\b|#)")


def owned_source_files():
    listed = subprocess.run(["git", "ls-files", *OWNED_DIRECTORIES], cwd=REPOSITORY_ROOT, check=True,
                            capture_output=True, text=True).stdout.split()
    return [os.path.join(REPOSITORY_ROOT, path) for path in listed
            if path.endswith(SOURCE_EXTENSIONS) and not path.startswith(VENDORED_PREFIX)]


def leading_whitespace(line):
    return line[:len(line) - len(line.lstrip(" \t"))]


def find_block_opener(lines, brace_line_number):
    """Returns the first line of the statement that owns the '{' on brace_line_number.

    With braces on their own line, the brace sits at the same indentation as the
    start of its statement, so the opener is the nearest earlier line at that
    indentation.
    """
    indentation = leading_whitespace(lines[brace_line_number])
    for candidate_line_number in range(brace_line_number - 1, -1, -1):
        candidate = lines[candidate_line_number]
        if candidate.strip() == "" or candidate.lstrip().startswith("#"):
            continue
        if leading_whitespace(candidate) == indentation:
            return candidate.strip()
    return ""


def check_file(path, fix):
    with open(path, encoding="utf-8") as source:
        lines = source.read().split("\n")

    problems = []
    if not PASCAL_CASE_FILE_NAME.match(os.path.basename(path)):
        problems.append((0, "name the file in PascalCase after the class or contents it holds"))

    open_blocks = []  # True for each open block that is a conditional or a loop
    missing_blank_lines = []
    for line_number, line in enumerate(lines):
        stripped = line.strip()
        if line.startswith(" ") and stripped and not stripped.startswith("*"):
            problems.append((line_number, "indent with tabs, not spaces"))

        if stripped == "{":
            open_blocks.append(bool(CONTROL_KEYWORDS.match(find_block_opener(lines, line_number))))
        elif stripped.endswith("{") and not stripped.startswith(("//", "/*", "*")):
            open_blocks.append(False)  # a braced initializer or similar opened on a code line
        if stripped in ("}", "};") and open_blocks:
            closes_control_block = open_blocks.pop()
            next_line = lines[line_number + 1].strip() if line_number + 1 < len(lines) else ""
            if closes_control_block and next_line != "" and not CONTINUATIONS.match(next_line):
                problems.append((line_number, "leave a blank line after the closing brace of a conditional or loop"))
                missing_blank_lines.append(line_number)
        elif stripped.startswith("}") and open_blocks and stripped not in ("}", "};"):
            open_blocks.pop()  # e.g. "});" closing a lambda or "} while (...);"

    if fix and missing_blank_lines:
        for line_number in reversed(missing_blank_lines):
            lines.insert(line_number + 1, "")
        with open(path, "w", encoding="utf-8") as source:
            source.write("\n".join(lines))
        problems = [problem for problem in problems if problem[0] not in missing_blank_lines]

    relative_path = os.path.relpath(path, REPOSITORY_ROOT)
    for line_number, message in problems:
        print("%s:%d: %s" % (relative_path, line_number + 1, message))
    return len(problems)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--fix", action="store_true", help="insert the missing blank lines")
    parser.add_argument("files", nargs="*", help="files to check (default: every C and C++ file we own)")
    arguments = parser.parse_args()

    paths = [os.path.abspath(path) for path in arguments.files] or owned_source_files()
    problem_count = sum(check_file(path, arguments.fix) for path in paths)
    if problem_count:
        print("%d style problem(s)" % problem_count)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())

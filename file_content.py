#!/usr/bin/env python3

import os
import sys
import argparse

# Directories we generally want to skip in a CMake C/C++ project
EXCLUDED_DIRECTORIES = {
    'build',
    'bin',
    '.git',
    'out',
    'logs',
    'CMakeFiles',  # CMake's build-related directory
    '.vscode',
    '.idea',
    '.clangd',
    'debug',
    'release',
    '.pio',
}

# Common text-based file extensions for CMake, C, and C++ projects
VALID_FILE_EXTENSIONS = {
    '.c',       # C source
    '.cpp',     # C++ source
    '.h',       # C/C++ headers
    '.hpp',     # C++ headers
    '.cc',      # Alternative C++ source
    '.hh',      # Alternative C++ headers
    '.cxx',     # Alternative C++ source
    '.hxx',     # Alternative C++ headers
    '.cmake',   # CMake scripts
    'CMakeLists.txt',  # Main CMake configuration file
    '.json',    # JSON files (e.g., configuration)
    '.yml',
    '.properties',  # Optional configuration files
    '.ino',
    '.ini'
}

def should_skip_directory(dir_name: str) -> bool:
    """
    Returns True if we should skip this directory:
      - if it contains 'test' (in any case)
      - if it is in EXCLUDED_DIRECTORIES
    """
    dir_lower = dir_name.lower()
    if 'test' in dir_lower:
        return True
    if dir_name in EXCLUDED_DIRECTORIES:
        return True
    return False

def should_process_file(file_name: str) -> bool:
    """
    Returns True if we should read this file.
    We skip:
      - any file whose lowercase name contains 'test'
      - any file whose extension is not in VALID_FILE_EXTENSIONS
    """
    file_lower = file_name.lower()
    if 'test' in file_lower:
        return False

    if file_name in VALID_FILE_EXTENSIONS:  # For exact matches like 'CMakeLists.txt'
        return True
    
    _, ext = os.path.splitext(file_name)
    if ext.lower() in VALID_FILE_EXTENSIONS:
        return True
    
    return False

def extract_text_from_files(input_dir, output_file):
    """
    Recursively walk through `input_dir`, skipping generated/binary/log directories
    and skipping any file or directory that contains 'test' in the name,
    writing text-based contents of valid files to `output_file`.
    """
    with open(output_file, 'w', encoding='utf-8') as out_f:
        for root, dirs, files in os.walk(input_dir):
            # Filter out directories we don't want to descend into
            dirs[:] = [d for d in dirs if not should_skip_directory(d)]

            for file_name in files:
                if not should_process_file(file_name):
                    continue

                full_path = os.path.join(root, file_name)
                try:
                    # Attempt to read each file as text
                    with open(full_path, 'r', encoding='utf-8', errors='replace') as f:
                        text_content = f.read()

                    # Write the relative path + content to the output
                    rel_path = os.path.relpath(full_path, input_dir)
                    out_f.write(f"--- {rel_path} ---\n")
                    out_f.write(text_content + "\n\n")

                except Exception as e:
                    print(f"Could not read file {full_path}: {e}", file=sys.stderr)

def main():
    parser = argparse.ArgumentParser(
        description=(
            "Recursively extract text from typical source/CMake files in a "
            "CMake C/C++ project, skipping generated dirs/files, logs, "
            "or anything containing 'test'."
        )
    )
    parser.add_argument("--root", required=True, help="Root directory of the CMake C/C++ project.")
    parser.add_argument("--output", required=True, help="Output file to write extracted contents.")

    args = parser.parse_args()

    if not os.path.isdir(args.root):
        parser.error(f"The specified root '{args.root}' is not a directory or does not exist.")

    extract_text_from_files(args.root, args.output)
    print(f"Extraction complete. Output written to {args.output}")

if __name__ == "__main__":
    main()

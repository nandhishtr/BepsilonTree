#!/bin/bash

# Check if at least two arguments are provided
if [ "$#" -lt 2 ]; then
    echo "Usage: $0 output_file path [path ...] [-h] [-e extensions] [-f files]"
    exit 1
fi

output_file="$1"
shift
paths=()

# Collect paths until we encounter options
while [ "$#" -gt 0 ] && [[ "$1" != "-"* ]]; do
    paths+=("$1")
    shift
done

extension=".*"
include_hidden_dirs=false
exclude_extensions=()
exclude_files=()

# Function to print usage
usage() {
    echo "Usage: $0 output_file path [path ...] [-h] [-e extensions] [-f files]"
    echo "  -h  Include hidden directories"
    echo "  -e  Exclude file extensions (comma-separated, no spaces)"
    echo "  -f  Exclude specific files (comma-separated, no spaces)"
    exit 1
}

# Parse command-line options
while getopts ":he:f:" opt; do
    case ${opt} in
        h )
            include_hidden_dirs=true
            ;;
        e )
            IFS=',' read -r -a exclude_extensions <<< "$OPTARG"
            ;;
        f )
            IFS=',' read -r -a exclude_files <<< "$OPTARG"
            ;;
        \? )
            usage
            ;;
    esac
done

# Empty the output file if it exists, or create a new one
> "$output_file"

# Build find command for each path
for path in "${paths[@]}"; do
    find_cmd="find \"$path\" -type f -name \"*$extension\""

    # Include hidden directories if specified
    if [ "$include_hidden_dirs" = false ]; then
        find_cmd="$find_cmd -not -path '*/\.*'"
    fi

    # Execute find command and iterate over the files
    eval $find_cmd | while read -r file; do
        # Exclude files with specific extensions
        for ext in "${exclude_extensions[@]}"; do
            if [[ "$file" == *"$ext" ]]; then
                continue 2
            fi
        done

        # Exclude specific files
        for excl_file in "${exclude_files[@]}"; do
            if [[ "$(basename "$file")" == "$excl_file" ]]; then
                continue 2
            fi
        done

        echo "--- Start of $file ---" >> "$output_file"
        cat "$file" >> "$output_file"
        echo -e "\n--- End of $file ---\n" >> "$output_file"
    done
done

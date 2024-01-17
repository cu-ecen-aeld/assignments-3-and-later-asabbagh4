#!/bin/bash

# Assign arguments to variables
filesdir="$1"
searchstr="$2"

# Function to exit with an error message
exit_with_error() {
    echo "$1" >&2
    exit 1
}

# Check if both arguments are provided
if [ -z "$filesdir" ] || [ -z "$searchstr" ]; then
    exit_with_error "Error: Both directory path and search string must be provided."
fi

# Check if the directory exists
if [ ! -d "$filesdir" ]; then
    exit_with_error "Error: Provided path '$filesdir' is not a directory."
fi

# Count the number of files in the directory and subdirectories
file_count=$(find "$filesdir" -type f | wc -l)

# Count the number of matching lines
matching_lines_count=$(grep -r "$searchstr" "$filesdir" | wc -l)

# Print the result
echo "The number of files are $file_count and the number of matching lines are $matching_lines_count"

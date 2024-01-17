#!/bin/bash

# Assign arguments to variables
writefile="$1"
writestr="$2"

# Function to exit with an error message
exit_with_error() {
    echo "$1" >&2
    exit 1
}

# Check if both arguments are provided
if [ -z "$writefile" ] || [ -z "$writestr" ]; then
    exit_with_error "Error: Both file path and write string must be provided."
fi

# Create the directory path if it does not exist
dirpath=$(dirname "$writefile")
mkdir -p "$dirpath" || exit_with_error "Error: Failed to create directory path."

# Write the string to the file, creating or overwriting the file
echo "$writestr" > "$writefile" || exit_with_error "Error: Failed to write to file."

# Success message (optional)
echo "File '$writefile' created successfully with the provided content."

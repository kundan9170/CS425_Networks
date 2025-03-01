#!/bin/bash

export DISPLAY=:0
export XDG_SESSION_TYPE=x11

TEST_FILE="test_files.txt"
CLIENT_BINARY="./client_test"

if [[ ! -f "$TEST_FILE" ]]; then
    echo "Error: $TEST_FILE not found!"
    exit 1
fi

# Debugging: print out the content of the test file
echo "Reading from $TEST_FILE..."
line_count=0
while IFS= read -r arg; do
    echo "Processing line: $arg"
    line_count=$((line_count + 1))
    # sleep 0.1  # Adjust sleep if necessary
    gnome-terminal -- bash -c "$CLIENT_BINARY \"$arg\"; exec bash" &
done < "$TEST_FILE"

# Wait for all background processes to finish
wait

echo "Finished processing $line_count lines."

#!/bin/bash
#
# Version Forge Setup Script
#
# This script automates dependency installation, cleanup, and compilation
# for the Version Forge project.

PROJECT_NAME="Version Forge"
SUCCESS_EMOJI="✅"
FAIL_EMOJI="❌"

# --- 1. Core Functions ---

# Function to check if a command exists
check_cmd() {
    command -v "$1" >/dev/null 2>&1
}

# Function to run make and check status
run_make() {
    echo "--- Compiling targets: version_forge and vf_server ---"
    
    # Use the Makefile to build both targets
    if make; then
        echo -e "\n$SUCCESS_EMOJI Compilation Successful!"
        echo "Executables created: ./version_forge and ./vf_server"
        return 0
    else
        echo -e "\n$FAIL_EMOJI Compilation FAILED."
        echo "Please check the errors above and ensure all source files are present."
        return 1
    fi
}

# --- 2. Main Script Logic ---

case "$1" in
    "deps")
        echo "--- Installing Required Dependencies ---"
        if check_cmd apt-get; then
            echo "Installing build-essential, libssl-dev (for SHA-1), and zlib1g-dev (for compression)..."
            sudo apt-get update
            sudo apt-get install build-essential libssl-dev zlib1g-dev -y
            echo -e "\n$SUCCESS_EMOJI Dependencies installed."
        else
            echo "$FAIL_EMOJI Error: This script requires 'apt-get' (Debian/Ubuntu)."
            exit 1
        fi
        ;;

    "build")
        echo "--- Cleaning Old Build Files ---"
        make clean
        run_make
        ;;
        
    "clean")
        echo "--- Cleaning Project Directories ---"
        make clean
        rm -rf .minivcs .minivcs_fork >/dev/null 2>&1
        echo -e "\n$SUCCESS_EMOJI Clean successful. Object files and test repos removed."
        ;;

    *)
        echo "Usage: ./setup.sh [command]"
        echo "Commands:"
        echo "  deps    - Install required libraries (libssl-dev, zlib1g-dev)."
        echo "  build   - Clean old files and compile the project (default)."
        echo "  clean   - Remove all compiled binaries (.o, version_forge, vf_server) and test repos (.minivcs*)."
        ;;
esac

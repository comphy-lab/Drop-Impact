#!/bin/bash
# runSimulation.sh - Run single drop impact simulation from root directory
# Creates case folder in SimulationCases/<CaseNo>/ and runs simulation there

set -e  # Exit on error

# ============================================================
# Configuration
# ============================================================
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Source project configuration
if [ -f "${SCRIPT_DIR}/.project_config" ]; then
    source "${SCRIPT_DIR}/.project_config"
else
    echo "ERROR: .project_config not found" >&2
    exit 1
fi

# Source parameter parsing library
if [ -f "${SCRIPT_DIR}/src-local/parse_params.sh" ]; then
    source "${SCRIPT_DIR}/src-local/parse_params.sh"
else
    echo "ERROR: src-local/parse_params.sh not found" >&2
    exit 1
fi

# ============================================================
# Usage Information
# ============================================================
usage() {
    cat <<EOF
Usage: $0 [OPTIONS] [params_file]

Run single drop impact simulation from root directory.
Creates case folder in SimulationCases/<CaseNo>/ based on parameter file.

Options:
    -c, --compile-only    Compile but don't run simulation
    -d, --debug           Compile with debug flags (-g -DTRASH=1)
    -v, --verbose         Verbose output
    -h, --help           Show this help message

Parameter file mode (default):
    $0 default.params

If no parameter file specified, uses default.params from current directory.

Environment variables:
    QCC_FLAGS     Additional qcc compiler flags

Examples:
    # Run with default parameters
    $0

    # Run with custom parameter file
    $0 default.params

    # Compile only (check for errors)
    $0 --compile-only

    # Debug mode with memory checking
    $0 --debug default.params

For more information, see CLAUDE.md
EOF
}

# ============================================================
# Parse Command Line Options
# ============================================================
COMPILE_ONLY=0
DEBUG_FLAGS=""
VERBOSE=0

while [[ $# -gt 0 ]]; do
    case $1 in
        -c|--compile-only)
            COMPILE_ONLY=1
            shift
            ;;
        -d|--debug)
            DEBUG_FLAGS="-g -DTRASH=1"
            shift
            ;;
        -v|--verbose)
            VERBOSE=1
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        -*)
            echo "ERROR: Unknown option: $1" >&2
            usage
            exit 1
            ;;
        *)
            break
            ;;
    esac
done

# ============================================================
# Determine Parameter File
# ============================================================
PARAM_FILE="${1:-default.params}"

if [ ! -f "$PARAM_FILE" ]; then
    echo "ERROR: Parameter file not found: $PARAM_FILE" >&2
    exit 1
fi

[ $VERBOSE -eq 1 ] && echo "Parameter file: $PARAM_FILE"

# ============================================================
# Parse Parameters to Get CaseNo
# ============================================================
parse_param_file "$PARAM_FILE"

CASE_NO=$(get_param "CaseNo")

if [ -z "$CASE_NO" ]; then
    echo "ERROR: CaseNo not found in parameter file" >&2
    exit 1
fi

# Validate CaseNo is 4 digits
if ! [[ "$CASE_NO" =~ ^[0-9]{4}$ ]] || [ "$CASE_NO" -lt 1000 ] || [ "$CASE_NO" -gt 9999 ]; then
    echo "ERROR: CaseNo must be 4-digit (1000-9999), got: $CASE_NO" >&2
    exit 1
fi

CASE_DIR="SimulationCases/${CASE_NO}"

echo "========================================="
echo "Drop Impact Simulation - Single Case"
echo "========================================="
echo "Case Number: $CASE_NO"
echo "Case Directory: $CASE_DIR"
echo "Parameter File: $PARAM_FILE"
echo ""

# ============================================================
# Create Case Directory
# ============================================================
if [ ! -d "$CASE_DIR" ]; then
    echo "Creating case directory: $CASE_DIR"
    mkdir -p "$CASE_DIR"
else
    echo "Case directory exists (will use restart if available)"
fi

# Copy parameter file to case directory for record keeping
cp "$PARAM_FILE" "$CASE_DIR/case.params"

# Change to case directory
cd "$CASE_DIR"
[ $VERBOSE -eq 1 ] && echo "Working directory: $(pwd)"

# ============================================================
# Compilation
# ============================================================
SRC_FILE_ORIG="../dropImpact.c"
SRC_FILE_LOCAL="dropImpact.c"
EXECUTABLE="dropImpact"

echo ""
echo "========================================="
echo "Compilation"
echo "========================================="

# Check if source file exists
if [ ! -f "$SRC_FILE_ORIG" ]; then
    echo "ERROR: Source file $SRC_FILE_ORIG not found" >&2
    exit 1
fi

# Copy source file to case directory for compilation
# This avoids qcc issues with relative paths and keeps a record
cp "$SRC_FILE_ORIG" "$SRC_FILE_LOCAL"
echo "Copied source file to case directory"

# Compile
echo "Compiling $SRC_FILE_LOCAL..."
[ $VERBOSE -eq 1 ] && echo "Compiler: qcc"
[ $VERBOSE -eq 1 ] && echo "Include paths: -I../../src-local"
[ $VERBOSE -eq 1 ] && echo "Flags: -O2 -Wall -disable-dimensions $DEBUG_FLAGS $QCC_FLAGS"

qcc -I../../src-local \
    -O2 -Wall -disable-dimensions \
    $DEBUG_FLAGS $QCC_FLAGS \
    "$SRC_FILE_LOCAL" -o "$EXECUTABLE" -lm

if [ $? -ne 0 ]; then
    echo "ERROR: Compilation failed" >&2
    exit 1
fi

echo "Compilation successful: $EXECUTABLE"

# Exit if compile-only mode
if [ $COMPILE_ONLY -eq 1 ]; then
    echo ""
    echo "Compile-only mode: Stopping here"
    cd ../..
    exit 0
fi

# ============================================================
# Execution
# ============================================================
echo ""
echo "========================================="
echo "Execution"
echo "========================================="

# Use the copied parameter file
echo "Running simulation with case.params"

if [ -f "restart" ]; then
    echo "Restart file found - simulation will resume from checkpoint"
fi

echo ""
echo "Starting simulation..."
echo "========================================="

# Run simulation
./dropImpact case.params

EXIT_CODE=$?

echo "========================================="
if [ $EXIT_CODE -eq 0 ]; then
    echo "Simulation completed successfully"
    echo "Output location: $CASE_DIR"
else
    echo "Simulation failed with exit code $EXIT_CODE"
fi
echo "========================================="

# Return to root directory
cd ../..

exit $EXIT_CODE

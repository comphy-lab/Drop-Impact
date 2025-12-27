#!/bin/bash
# runPostProcess-Ncases.sh - Run post-processing pipeline on multiple simulation cases
# Author: Vatsal Sanjay
# vatsal.sanjay@comphy-lab.org
# CoMPhy Lab — Durham University
# Last updated: Dec 2025

set -e  # Exit on error

# ============================================================
# Configuration
# ============================================================
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Post-processing script paths
VIDEO_SCRIPT="${SCRIPT_DIR}/postProcess/Video-generic.py"
FOOTPRINT_SCRIPT="${SCRIPT_DIR}/postProcess/getFootPrint.py"
PLOT_SCRIPT="${SCRIPT_DIR}/postProcess/plotFootPrint.py"

# C helper executables
HELPER_GETFACET="${SCRIPT_DIR}/postProcess/getFacet"
HELPER_GETDATA="${SCRIPT_DIR}/postProcess/getData-generic"
HELPER_GETFOOTPRINT="${SCRIPT_DIR}/postProcess/getFootPrint"

# Case directory root
CASES_DIR="${SCRIPT_DIR}/simulationCases"

# ============================================================
# Usage Information
# ============================================================
usage() {
    cat <<EOF
Usage: $0 [OPTIONS] CASE_NO [CASE_NO ...]

Run post-processing pipeline on multiple simulation cases.
For each case, executes: Video-generic.py → getFootPrint.py → plotFootPrint.py

Options:
    --CPUs N            Number of parallel workers (default: 4)
    --nGFS N            Number of snapshots to process (default: 4000)
    --tsnap F           Time interval between snapshots (default: 0.01)
    --GridsPerR N       Radial grid resolution for video (default: 256)
    --ZMAX F            Maximum Z value for video (default: 4.0)
    --RMAX F            Maximum R value for video (default: 4.0)
    --ZMIN F            Minimum Z value for video (default: 0.0)

    --skip-video        Skip Video-generic.py (frame generation)
    --skip-footprint    Skip getFootPrint.py (footprint extraction)
    --skip-plot         Skip plotFootPrint.py (PDF generation)

    -n, --dry-run       Show what would run without executing
    -v, --verbose       Verbose output
    -h, --help          Show this help message

Arguments:
    CASE_NO             4-digit case numbers (1000-9999)

Examples:
    # Process multiple cases with default settings
    $0 1000 1001 1002 1003 1004 1005 1006

    # Process with more CPUs and fewer snapshots
    $0 --CPUs 8 --nGFS 500 1000 1001

    # Skip video generation (faster processing)
    $0 --skip-video 1000 1001 1002

    # Only generate footprint plots (skip video and extraction)
    $0 --skip-video --skip-footprint 1000 1001

    # Dry run to preview commands
    $0 --dry-run 1000 1001

Output locations:
    simulationCases/<CaseNo>/results/Video/              # PNG frames
    simulationCases/<CaseNo>/results/rFootvsTime_*.csv   # Footprint data
    simulationCases/<CaseNo>/results/footprint_evolution.pdf  # Plot

For more information, see CLAUDE.md
EOF
}

# ============================================================
# Parse Command Line Options
# ============================================================
CPUS=4
NGFS=4000
TSNAP=0.01
GRIDS_PER_R=256
ZMAX=4.0
RMAX=4.0
ZMIN=0.0

SKIP_VIDEO=0
SKIP_FOOTPRINT=0
SKIP_PLOT=0
DRY_RUN=0
VERBOSE=0

CASE_NUMBERS=()

while [[ $# -gt 0 ]]; do
    case $1 in
        --CPUs)
            CPUS="$2"
            if ! [[ "$CPUS" =~ ^[0-9]+$ ]] || [ "$CPUS" -lt 1 ]; then
                echo "ERROR: --CPUs requires a positive integer, got: $CPUS" >&2
                exit 1
            fi
            shift 2
            ;;
        --nGFS)
            NGFS="$2"
            if ! [[ "$NGFS" =~ ^[0-9]+$ ]] || [ "$NGFS" -lt 1 ]; then
                echo "ERROR: --nGFS requires a positive integer, got: $NGFS" >&2
                exit 1
            fi
            shift 2
            ;;
        --tsnap)
            TSNAP="$2"
            shift 2
            ;;
        --GridsPerR)
            GRIDS_PER_R="$2"
            if ! [[ "$GRIDS_PER_R" =~ ^[0-9]+$ ]] || [ "$GRIDS_PER_R" -lt 1 ]; then
                echo "ERROR: --GridsPerR requires a positive integer, got: $GRIDS_PER_R" >&2
                exit 1
            fi
            shift 2
            ;;
        --ZMAX)
            ZMAX="$2"
            shift 2
            ;;
        --RMAX)
            RMAX="$2"
            shift 2
            ;;
        --ZMIN)
            ZMIN="$2"
            shift 2
            ;;
        --skip-video)
            SKIP_VIDEO=1
            shift
            ;;
        --skip-footprint)
            SKIP_FOOTPRINT=1
            shift
            ;;
        --skip-plot)
            SKIP_PLOT=1
            shift
            ;;
        -n|--dry-run)
            DRY_RUN=1
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
            # Collect case numbers
            CASE_NUMBERS+=("$1")
            shift
            ;;
    esac
done

# ============================================================
# Validation
# ============================================================

# Check at least one case number provided
if [ ${#CASE_NUMBERS[@]} -eq 0 ]; then
    echo "ERROR: No case numbers provided" >&2
    usage
    exit 1
fi

# Validate case numbers are 4-digit
for case_no in "${CASE_NUMBERS[@]}"; do
    if ! [[ "$case_no" =~ ^[0-9]{4}$ ]]; then
        echo "ERROR: Case number must be 4 digits (1000-9999), got: $case_no" >&2
        exit 1
    fi
    if [ "$case_no" -lt 1000 ] || [ "$case_no" -gt 9999 ]; then
        echo "ERROR: Case number out of range (1000-9999), got: $case_no" >&2
        exit 1
    fi
done

# Check if all processing steps are skipped
if [ $SKIP_VIDEO -eq 1 ] && [ $SKIP_FOOTPRINT -eq 1 ] && [ $SKIP_PLOT -eq 1 ]; then
    echo "ERROR: All processing steps are skipped (--skip-video, --skip-footprint, --skip-plot)" >&2
    echo "       At least one processing step must be enabled." >&2
    exit 1
fi

# Check Python availability
if ! command -v python &> /dev/null; then
    echo "ERROR: python not found in PATH" >&2
    exit 1
fi

# Check Python scripts exist
for script in "$VIDEO_SCRIPT" "$FOOTPRINT_SCRIPT" "$PLOT_SCRIPT"; do
    if [ ! -f "$script" ]; then
        echo "ERROR: Python script not found: $script" >&2
        exit 1
    fi
done

# Check C helpers exist (only if needed)
if [ $SKIP_VIDEO -eq 0 ]; then
    if [ ! -x "$HELPER_GETFACET" ]; then
        echo "ERROR: Compiled helper not found or not executable: $HELPER_GETFACET" >&2
        echo "       Compile with: qcc -autolink postProcess/getFacet.c -o postProcess/getFacet -lm" >&2
        exit 1
    fi
    if [ ! -x "$HELPER_GETDATA" ]; then
        echo "ERROR: Compiled helper not found or not executable: $HELPER_GETDATA" >&2
        echo "       Compile with: qcc -autolink postProcess/getData-generic.c -o postProcess/getData-generic -lm" >&2
        exit 1
    fi
fi

if [ $SKIP_FOOTPRINT -eq 0 ]; then
    if [ ! -x "$HELPER_GETFOOTPRINT" ]; then
        echo "ERROR: Compiled helper not found or not executable: $HELPER_GETFOOTPRINT" >&2
        echo "       Compile with: qcc -autolink postProcess/getFootPrint.c -o postProcess/getFootPrint -lm" >&2
        exit 1
    fi
fi

# ============================================================
# Display Configuration
# ============================================================
echo "========================================="
echo "Drop Impact - Post-Processing Pipeline"
echo "========================================="
echo "Cases to process: ${CASE_NUMBERS[*]}"
echo "Total cases: ${#CASE_NUMBERS[@]}"
echo ""
echo "Settings:"
echo "  CPUs:       $CPUS"
echo "  nGFS:       $NGFS"
echo "  tsnap:      $TSNAP"
echo "  GridsPerR:  $GRIDS_PER_R"
echo "  Domain:     R=[0,$RMAX], Z=[$ZMIN,$ZMAX]"
echo ""
echo "Steps to run:"
[ $SKIP_VIDEO -eq 0 ] && echo "  [1] Video-generic.py (frame generation)" || echo "  [1] Video-generic.py (SKIPPED)"
[ $SKIP_FOOTPRINT -eq 0 ] && echo "  [2] getFootPrint.py (footprint extraction)" || echo "  [2] getFootPrint.py (SKIPPED)"
[ $SKIP_PLOT -eq 0 ] && echo "  [3] plotFootPrint.py (PDF generation)" || echo "  [3] plotFootPrint.py (SKIPPED)"
echo ""
[ $DRY_RUN -eq 1 ] && echo "Mode: DRY RUN (no execution)"
echo ""

# ============================================================
# Processing Functions
# ============================================================

run_video() {
    local case_no="$1"
    local results_dir="${CASES_DIR}/${case_no}/results"
    local video_dir="${results_dir}/Video"

    local cmd="python ${VIDEO_SCRIPT} \
        --caseToProcess ${results_dir} \
        --folderToSave ${video_dir} \
        --CPUs ${CPUS} \
        --nGFS ${NGFS} \
        --tsnap ${TSNAP} \
        --GridsPerR ${GRIDS_PER_R} \
        --ZMAX ${ZMAX} \
        --RMAX ${RMAX} \
        --ZMIN ${ZMIN}"

    if [ $VERBOSE -eq 1 ] || [ $DRY_RUN -eq 1 ]; then
        echo "  CMD: $cmd"
    fi

    if [ $DRY_RUN -eq 0 ]; then
        eval "$cmd"
    fi
}

run_footprint() {
    local case_no="$1"
    local results_dir="${CASES_DIR}/${case_no}/results"

    local cmd="python ${FOOTPRINT_SCRIPT} \
        --caseToProcess ${results_dir} \
        --CPUs ${CPUS} \
        --nGFS ${NGFS} \
        --tsnap ${TSNAP}"

    if [ $VERBOSE -eq 1 ] || [ $DRY_RUN -eq 1 ]; then
        echo "  CMD: $cmd"
    fi

    if [ $DRY_RUN -eq 0 ]; then
        eval "$cmd"
    fi
}

run_plot() {
    local case_no="$1"
    local results_dir="${CASES_DIR}/${case_no}/results"

    local cmd="python ${PLOT_SCRIPT} --resultsDir ${results_dir}"

    if [ $VERBOSE -eq 1 ] || [ $DRY_RUN -eq 1 ]; then
        echo "  CMD: $cmd"
    fi

    if [ $DRY_RUN -eq 0 ]; then
        eval "$cmd"
    fi
}

# ============================================================
# Main Processing Loop
# ============================================================
echo "========================================="
echo "Processing Cases"
echo "========================================="

SUCCESSFUL_CASES=()
FAILED_CASES=()
FAILURE_REASONS=()

for case_no in "${CASE_NUMBERS[@]}"; do
    echo ""
    echo "-----------------------------------------"
    echo "Case $case_no"
    echo "-----------------------------------------"

    case_dir="${CASES_DIR}/${case_no}"
    results_dir="${case_dir}/results"
    intermediate_dir="${results_dir}/intermediate"

    # Validate case directory exists
    if [ ! -d "$case_dir" ]; then
        echo "  ERROR: Case directory not found: $case_dir"
        FAILED_CASES+=("$case_no")
        FAILURE_REASONS+=("Case directory not found")
        continue
    fi

    if [ ! -d "$results_dir" ]; then
        echo "  ERROR: Results directory not found: $results_dir"
        FAILED_CASES+=("$case_no")
        FAILURE_REASONS+=("Results directory not found")
        continue
    fi

    if [ ! -d "$intermediate_dir" ]; then
        echo "  ERROR: Intermediate directory not found: $intermediate_dir"
        FAILED_CASES+=("$case_no")
        FAILURE_REASONS+=("No intermediate/ snapshots")
        continue
    fi

    # Count snapshots
    snapshot_count=$(find "$intermediate_dir" -name "snapshot-*" 2>/dev/null | wc -l | tr -d ' ')
    echo "  Found $snapshot_count snapshots in intermediate/"

    if [ "$snapshot_count" -eq 0 ]; then
        echo "  ERROR: No snapshots found"
        FAILED_CASES+=("$case_no")
        FAILURE_REASONS+=("No snapshots in intermediate/")
        continue
    fi

    # Track step failures
    step_failed=0

    # Step 1: Video generation
    if [ $SKIP_VIDEO -eq 0 ]; then
        echo ""
        echo "  [1/3] Running Video-generic.py..."
        if ! run_video "$case_no"; then
            echo "  ERROR: Video generation failed"
            step_failed=1
        else
            [ $DRY_RUN -eq 0 ] && echo "  [1/3] Video generation complete"
        fi
    else
        echo "  [1/3] Video generation (skipped)"
    fi

    # Step 2: Footprint extraction
    if [ $SKIP_FOOTPRINT -eq 0 ] && [ $step_failed -eq 0 ]; then
        echo ""
        echo "  [2/3] Running getFootPrint.py..."
        if ! run_footprint "$case_no"; then
            echo "  ERROR: Footprint extraction failed"
            step_failed=1
        else
            [ $DRY_RUN -eq 0 ] && echo "  [2/3] Footprint extraction complete"
        fi
    elif [ $SKIP_FOOTPRINT -eq 1 ]; then
        echo "  [2/3] Footprint extraction (skipped)"
    fi

    # Step 3: Plot generation
    if [ $SKIP_PLOT -eq 0 ] && [ $step_failed -eq 0 ]; then
        echo ""
        echo "  [3/3] Running plotFootPrint.py..."
        if ! run_plot "$case_no"; then
            echo "  ERROR: Plot generation failed"
            step_failed=1
        else
            [ $DRY_RUN -eq 0 ] && echo "  [3/3] Plot generation complete"
        fi
    elif [ $SKIP_PLOT -eq 1 ]; then
        echo "  [3/3] Plot generation (skipped)"
    fi

    # Record result
    if [ $step_failed -eq 0 ]; then
        SUCCESSFUL_CASES+=("$case_no")
        echo ""
        echo "  Case $case_no: SUCCESS"
    else
        FAILED_CASES+=("$case_no")
        FAILURE_REASONS+=("Processing step failed")
        echo ""
        echo "  Case $case_no: FAILED"
    fi
done

# ============================================================
# Summary
# ============================================================
echo ""
echo "========================================="
echo "Post-Processing Complete"
echo "========================================="
echo "Total cases: ${#CASE_NUMBERS[@]}"
echo "Successful:  ${#SUCCESSFUL_CASES[@]}"
echo "Failed:      ${#FAILED_CASES[@]}"

if [ ${#FAILED_CASES[@]} -gt 0 ]; then
    echo ""
    echo "Failed cases:"
    for i in "${!FAILED_CASES[@]}"; do
        echo "  - Case ${FAILED_CASES[$i]}: ${FAILURE_REASONS[$i]}"
    done
fi

if [ ${#SUCCESSFUL_CASES[@]} -gt 0 ]; then
    echo ""
    echo "Output locations:"
    for case_no in "${SUCCESSFUL_CASES[@]}"; do
        echo "  ${CASES_DIR}/${case_no}/results/"
    done
fi

echo ""

# Exit with error if any cases failed
if [ ${#FAILED_CASES[@]} -gt 0 ]; then
    exit 1
fi

exit 0

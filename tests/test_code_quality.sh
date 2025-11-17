#!/bin/bash

echo "╔════════════════════════════════════════════════════════════╗"
echo "║   CRKit SDK - Code Quality Validation                    ║"
echo "║   Version: 1.0.0                                          ║"
echo "║   Date: 2025-01-17                                        ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Test results array
declare -a TEST_RESULTS

record_test() {
    local name="$1"
    local status="$2"
    local message="$3"

    TOTAL_TESTS=$((TOTAL_TESTS + 1))

    if [ "$status" = "PASS" ]; then
        PASSED_TESTS=$((PASSED_TESTS + 1))
        echo "  ✓ $name"
        TEST_RESULTS+=("PASS: $name")
    else
        FAILED_TESTS=$((FAILED_TESTS + 1))
        echo "  ✗ $name: $message"
        TEST_RESULTS+=("FAIL: $name - $message")
    fi
}

# ============================================================
# TC-S001: Source File Count
# ============================================================
echo "=== TC-S001: Source File Count Verification ==="
CPP_COUNT=$(find src -name "*.cpp" | wc -l)
H_COUNT=$(find src include -name "*.h" -o -name "*.hpp" | wc -l)

echo "  C++ source files: $CPP_COUNT"
echo "  Header files: $H_COUNT"

if [ $CPP_COUNT -ge 5 ] && [ $H_COUNT -ge 8 ]; then
    record_test "Source file count" "PASS" ""
else
    record_test "Source file count" "FAIL" "Insufficient source files"
fi

# ============================================================
# TC-S002: Header File Syntax Check
# ============================================================
echo ""
echo "=== TC-S002: Header File Syntax Validation ==="

# Test main API header
if g++ -std=c++11 -fsyntax-only -I./include include/crkit_api.h 2>/dev/null; then
    record_test "API header syntax" "PASS" ""
else
    record_test "API header syntax" "FAIL" "Syntax errors in crkit_api.h"
fi

# Test types.h (contains Detection fix)
if g++ -std=c++11 -fsyntax-only -I./src -I./include src/utils/types.h 2>/dev/null; then
    record_test "Types header syntax" "PASS" ""
else
    record_test "Types header syntax" "FAIL" "Syntax errors in types.h"
fi

# Test logger.h (contains thread-safe fix)
if g++ -std=c++11 -fsyntax-only -I./src src/utils/logger.h 2>/dev/null; then
    record_test "Logger header syntax" "PASS" ""
else
    record_test "Logger header syntax" "FAIL" "Syntax errors in logger.h"
fi

# Test backend interface
if g++ -std=c++11 -fsyntax-only -I./src -I./include src/backend/inference_backend.h 2>/dev/null; then
    record_test "Backend interface syntax" "PASS" ""
else
    record_test "Backend interface syntax" "FAIL" "Syntax errors in inference_backend.h"
fi

# ============================================================
# TC-S003: Detection Constructor Fix Verification
# ============================================================
echo ""
echo "=== TC-S003: Detection Constructor Fix Verification ==="

# Check if the fix is present in types.h
if grep -q "width(w)" src/utils/types.h; then
    record_test "Detection constructor fix" "PASS" ""
else
    record_test "Detection constructor fix" "FAIL" "Fix not found in types.h"
fi

# Check that the bug is not present
if ! grep -q "width(width)" src/utils/types.h; then
    record_test "Detection constructor bug absent" "PASS" ""
else
    record_test "Detection constructor bug absent" "FAIL" "Bug still present in types.h"
fi

# ============================================================
# TC-S004: CUDA Memory Fix Verification
# ============================================================
echo ""
echo "=== TC-S004: CUDA Memory Management Fix Verification ==="

# Check for cudaFreeHost (correct)
if grep -q "cudaFreeHost" src/backend/tensorrt_backend.cpp; then
    record_test "CUDA cudaFreeHost present" "PASS" ""
else
    record_test "CUDA cudaFreeHost present" "FAIL" "cudaFreeHost not found"
fi

# Check CUDA_CHECK macro exists
if grep -q "CUDA_CHECK" src/backend/tensorrt_backend.cpp; then
    record_test "CUDA_CHECK macro present" "PASS" ""
else
    record_test "CUDA_CHECK macro present" "FAIL" "CUDA_CHECK macro not found"
fi

# Verify CUDA_CHECK is used for cudaMalloc
if grep -q "CUDA_CHECK(cudaMalloc" src/backend/tensorrt_backend.cpp; then
    record_test "CUDA_CHECK used for cudaMalloc" "PASS" ""
else
    record_test "CUDA_CHECK used for cudaMalloc" "FAIL" "CUDA_CHECK not applied to cudaMalloc"
fi

# Verify CUDA_CHECK is used for cudaMemcpyAsync
if grep -q "CUDA_CHECK(cudaMemcpyAsync" src/backend/tensorrt_backend.cpp; then
    record_test "CUDA_CHECK used for cudaMemcpyAsync" "PASS" ""
else
    record_test "CUDA_CHECK used for cudaMemcpyAsync" "FAIL" "CUDA_CHECK not applied to cudaMemcpyAsync"
fi

# ============================================================
# TC-S005: Logger Thread Safety Fix Verification
# ============================================================
echo ""
echo "=== TC-S005: Logger Thread Safety Fix Verification ==="

# Check for localtime_r (POSIX thread-safe version)
if grep -q "localtime_r" src/utils/logger.cpp; then
    record_test "Logger localtime_r present" "PASS" ""
else
    record_test "Logger localtime_r present" "FAIL" "localtime_r not found"
fi

# Check that non-thread-safe localtime is not used directly
if ! grep -E "localtime\(&" src/utils/logger.cpp >/dev/null; then
    record_test "Logger non-thread-safe localtime absent" "PASS" ""
else
    record_test "Logger non-thread-safe localtime absent" "FAIL" "Non-thread-safe localtime still used"
fi

# ============================================================
# TC-S006: Preprocessor Optimization Verification
# ============================================================
echo ""
echo "=== TC-S006: Preprocessor Optimization Verification ==="

# Check for cv::subtract (optimized version)
if grep -q "cv::subtract" src/core/preprocessor.cpp; then
    record_test "OpenCV subtract optimization" "PASS" ""
else
    record_test "OpenCV subtract optimization" "FAIL" "cv::subtract not found"
fi

# Check for cv::divide (optimized version)
if grep -q "cv::divide" src/core/preprocessor.cpp; then
    record_test "OpenCV divide optimization" "PASS" ""
else
    record_test "OpenCV divide optimization" "FAIL" "cv::divide not found"
fi

# ============================================================
# TC-S007: Documentation Check
# ============================================================
echo ""
echo "=== TC-S007: Documentation Completeness ==="

# Check for README
if [ -f "README.md" ]; then
    record_test "README.md exists" "PASS" ""
else
    record_test "README.md exists" "FAIL" "README.md not found"
fi

# Check for Chinese README
if [ -f "README_CN.md" ]; then
    record_test "README_CN.md exists" "PASS" ""
else
    record_test "README_CN.md exists" "FAIL" "README_CN.md not found"
fi

# Check for architecture documentation
if [ -f "docs/ARCHITECTURE.md" ]; then
    record_test "Architecture docs exist" "PASS" ""
else
    record_test "Architecture docs exist" "FAIL" "ARCHITECTURE.md not found"
fi

# Check for API usage guide
if [ -f "docs/API_USAGE.md" ]; then
    record_test "API usage docs exist" "PASS" ""
else
    record_test "API usage docs exist" "FAIL" "API_USAGE.md not found"
fi

# Check for bug fix report
if [ -f "docs/BUG_FIX_REPORT.md" ]; then
    record_test "Bug fix report exists" "PASS" ""
else
    record_test "Bug fix report exists" "FAIL" "BUG_FIX_REPORT.md not found"
fi

# Check for test plan
if [ -f "docs/TEST_PLAN.md" ]; then
    record_test "Test plan exists" "PASS" ""
else
    record_test "Test_plan exists" "FAIL" "TEST_PLAN.md not found"
fi

# ============================================================
# TC-S008: Contact Info Update
# ============================================================
echo ""
echo "=== TC-S008: Contact Information Update ==="

# Check README for correct email
if grep -q "wlxinchat@gmail.com" README.md; then
    record_test "README email updated" "PASS" ""
else
    record_test "README email updated" "FAIL" "Email not updated in README.md"
fi

# Check Chinese README for correct email
if grep -q "wlxinchat@gmail.com" README_CN.md; then
    record_test "README_CN email updated" "PASS" ""
else
    record_test "README_CN email updated" "FAIL" "Email not updated in README_CN.md"
fi

# ============================================================
# TC-S009: Build System Check
# ============================================================
echo ""
echo "=== TC-S009: Build System Validation ==="

# Check for CMakeLists.txt
if [ -f "CMakeLists.txt" ]; then
    record_test "CMakeLists.txt exists" "PASS" ""
else
    record_test "CMakeLists.txt exists" "FAIL" "CMakeLists.txt not found"
fi

# Check for build scripts
if [ -f "scripts/build.sh" ]; then
    record_test "Build script exists" "PASS" ""
else
    record_test "Build script exists" "FAIL" "build.sh not found"
fi

# ============================================================
# TC-S010: License and Legal
# ============================================================
echo ""
echo "=== TC-S010: License and Legal Files ==="

# Check for LICENSE
if [ -f "LICENSE" ]; then
    record_test "LICENSE file exists" "PASS" ""
else
    record_test "LICENSE file exists" "FAIL" "LICENSE not found"
fi

# Check for .gitignore
if [ -f ".gitignore" ]; then
    record_test ".gitignore exists" "PASS" ""
else
    record_test ".gitignore exists" "FAIL" ".gitignore not found"
fi

# ============================================================
# Summary
# ============================================================
echo ""
echo "╔════════════════════════════════════════════════════════════╗"
echo "║                    TEST SUMMARY                            ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""
echo "Total Tests:  $TOTAL_TESTS"
echo "Passed:       $PASSED_TESTS"
echo "Failed:       $FAILED_TESTS"

if [ $FAILED_TESTS -eq 0 ]; then
    echo ""
    echo "✓✓✓ ALL CODE QUALITY TESTS PASSED ✓✓✓"
    echo ""
    exit 0
else
    echo ""
    echo "✗✗✗ SOME TESTS FAILED ✗✗✗"
    echo ""
    echo "Failed tests:"
    for result in "${TEST_RESULTS[@]}"; do
        if [[ $result == FAIL:* ]]; then
            echo "  - ${result#FAIL: }"
        fi
    done
    echo ""
    exit 1
fi

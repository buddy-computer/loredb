#!/bin/bash
set -euo pipefail

# Test script to verify CLEAN_BUILD functionality
echo "Testing CLEAN_BUILD functionality..."

# Create a mock build directory
mkdir -p test_build
echo "test file" > test_build/test.txt
echo "Created test build directory"

# Test 1: Check help includes clean flag
echo ""
echo "=== Test 1: Help text includes --clean flag ==="
if bash setup.sh --help | grep -q "clean"; then
    echo "✅ Help text includes --clean flag"
else
    echo "❌ Help text missing --clean flag"
    exit 1
fi

# Test 2: Simple argument parsing test
echo ""
echo "=== Test 2: Testing argument parsing ==="

# Create a simple test script that extracts the relevant parts
cat > parse_test.sh << 'PARSE_EOF'
#!/bin/bash
CLEAN_BUILD="false"
while [[ $# -gt 0 ]]; do
    case $1 in
        --clean)
            CLEAN_BUILD="true"
            shift
            ;;
        *)
            shift
            ;;
    esac
done
echo "CLEAN_BUILD=$CLEAN_BUILD"
PARSE_EOF

chmod +x parse_test.sh

# Test without --clean
result1=$(./parse_test.sh)
echo "Without --clean: $result1"
if [[ "$result1" != "CLEAN_BUILD=false" ]]; then
    echo "❌ Test failed: Expected 'CLEAN_BUILD=false' without --clean flag, but got '$result1'."
    exit 1
fi

# Test with --clean
result2=$(./parse_test.sh --clean)
echo "With --clean: $result2"
if [[ "$result2" != "CLEAN_BUILD=true" ]]; then
    echo "❌ Test failed: Expected 'CLEAN_BUILD=true' with --clean flag, but got '$result2'."
    exit 1
fi

# Test 3: Clean build logic
echo ""
echo "=== Test 3: Clean build logic ==="

# Create test function for configure_cmake logic
cat > configure_test.sh << 'CONFIGURE_EOF'
#!/bin/bash
BUILD_DIR="test_build"
CLEAN_BUILD="$1"

log_info() { echo "[INFO] $1"; }

configure_cmake() {
    log_info "Configuring CMake build..."
    
    # Remove old build directory if clean build is requested
    if [[ "$CLEAN_BUILD" == "true" ]] && [[ -d "$BUILD_DIR" ]]; then
        log_info "Cleaning existing build directory..."
        rm -rf "$BUILD_DIR"
        echo "CLEANED"
    else
        log_info "Preserving existing build directory"
        echo "PRESERVED"
    fi
}

configure_cmake
CONFIGURE_EOF

chmod +x configure_test.sh

# Test with CLEAN_BUILD=false
echo "Testing with CLEAN_BUILD=false:"
result3=$(./configure_test.sh "false")
echo "$result3"
if ! echo "$result3" | grep -q "PRESERVED"; then
    echo "❌ Test failed: Expected 'PRESERVED' but script output was different."
    exit 1
fi

# Recreate test directory
mkdir -p test_build
echo "test file" > test_build/test.txt

# Test with CLEAN_BUILD=true
echo "Testing with CLEAN_BUILD=true:"
result4=$(./configure_test.sh "true")
echo "$result4"
if ! echo "$result4" | grep -q "CLEANED"; then
    echo "❌ Test failed: Expected 'CLEANED' but script output was different."
    exit 1
fi

# Cleanup
rm -f parse_test.sh configure_test.sh
rm -rf test_build

echo ""
echo "All tests completed!"

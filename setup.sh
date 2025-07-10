#!/bin/bash

# LoreDB Setup Script
# Automatically installs dependencies, configures vcpkg, and builds the project
# Works on Ubuntu/Debian, macOS, and other Unix-like systems

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Script configuration
PROJECT_NAME="LoreDB"
BUILD_TYPE="${BUILD_TYPE:-Release}"
VCPKG_ROOT="${VCPKG_ROOT:-$HOME/vcpkg}"
BUILD_DIR="build"
SKIP_TESTS="${SKIP_TESTS:-false}"
CLEAN_BUILD="${CLEAN_BUILD:-false}"
PARALLEL_JOBS="${PARALLEL_JOBS:-$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)}"

# Detect OS using portable methods
detect_os() {
    local uname_s=$(uname -s)
    
    case "$uname_s" in
        Linux*)
            # Try to detect Linux distribution from /etc/os-release (systemd standard)
            if [[ -f /etc/os-release ]]; then
                local distro_id=$(grep '^ID=' /etc/os-release 2>/dev/null | cut -d'=' -f2 | tr -d '"')
                case "$distro_id" in
                    ubuntu|debian)
                        OS="ubuntu"
                        ;;
                    centos|rhel|fedora|rocky|almalinux)
                        OS="centos"
                        ;;
                    *)
                        # For other distributions, try fallback detection
                        if command -v apt-get &> /dev/null; then
                            OS="ubuntu"
                        elif command -v yum &> /dev/null || command -v dnf &> /dev/null; then
                            OS="centos"
                        else
                            OS="linux"
                        fi
                        ;;
                esac
            else
                # Fallback to command detection for older systems
                if command -v apt-get &> /dev/null; then
                    OS="ubuntu"
                elif command -v yum &> /dev/null || command -v dnf &> /dev/null; then
                    OS="centos"
                else
                    OS="linux"
                fi
            fi
            ;;
        Darwin*)
            OS="macos"
            ;;
        CYGWIN*|MINGW*|MSYS*)
            OS="windows"
            ;;
        FreeBSD*)
            OS="freebsd"
            ;;
        *)
            OS="unknown"
            ;;
    esac
}

# Check if command exists
command_exists() {
    command -v "$1" &> /dev/null
}

# Install system dependencies
install_system_deps() {
    log_info "Installing system dependencies..."
    
    case $OS in
        ubuntu)
            sudo apt-get update -qq
            sudo apt-get install -y \
                build-essential \
                cmake \
                pkg-config \
                git \
                curl \
                zip \
                unzip \
                tar \
                libreadline-dev \
                ninja-build
            ;;
        centos)
            # Use dnf if available (modern RHEL/CentOS/Fedora), otherwise fall back to yum
            if command_exists dnf; then
                PACKAGE_MANAGER="dnf"
            elif command_exists yum; then
                PACKAGE_MANAGER="yum"
            else
                log_error "Neither dnf nor yum package manager found"
                exit 1
            fi
            
            sudo $PACKAGE_MANAGER groupinstall -y "Development Tools" || \
            sudo $PACKAGE_MANAGER group install -y "Development Tools" || \
            sudo $PACKAGE_MANAGER install -y gcc gcc-c++ make
            
            sudo $PACKAGE_MANAGER install -y \
                cmake \
                pkgconfig \
                git \
                curl \
                zip \
                unzip \
                tar \
                readline-devel \
                ninja-build
            ;;
        macos)
            if ! command_exists brew; then
                log_error "Homebrew not found. Please install Homebrew first:"
                log_error "  /bin/bash -c \"\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\""
                exit 1
            fi
            # Note: tar, zip, unzip, curl are built into macOS
            brew install cmake git readline ninja
            ;;
        freebsd)
            # FreeBSD package management
            if command_exists pkg; then
                sudo pkg install -y cmake git curl zip unzip tar readline ninja
            else
                log_error "FreeBSD pkg command not found"
                exit 1
            fi
            ;;
        windows)
            log_warning "Windows detected. Please use one of the following approaches:"
            log_warning "  1. WSL (Windows Subsystem for Linux) - recommended"
            log_warning "  2. Visual Studio with vcpkg"
            log_warning "  3. MSYS2/MinGW-w64"
            log_warning "Manual dependencies needed:"
            log_warning "  - CMake 3.20+"
            log_warning "  - C++20 compiler (MSVC 2019+, GCC 11+, Clang 14+)"
            log_warning "  - Git, vcpkg"
            ;;
        *)
            log_warning "Unknown OS ($OS). Please install dependencies manually:"
            log_warning "  - CMake 3.20+"
            log_warning "  - C++20 compiler (GCC 11+, Clang 14+)"
            log_warning "  - Git, curl, zip, unzip, tar"
            log_warning "  - readline (optional)"
            log_warning "  - ninja-build (optional, for faster builds)"
            ;;
    esac
}

# Check compiler requirements
check_compiler() {
    log_info "Checking C++ compiler..."
    
    if command_exists g++; then
        GCC_VERSION=$(g++ -dumpversion | cut -d. -f1)
        if [[ $GCC_VERSION -ge 11 ]]; then
            log_success "GCC $GCC_VERSION found (C++20 compatible)"
            return 0
        else
            log_warning "GCC $GCC_VERSION found, but GCC 11+ required for C++20"
        fi
    fi
    
    if command_exists clang++; then
        # Try to get version using robust methods
        # First try -dumpfullversion (Clang 16+), then fall back to -dumpversion
        CLANG_FULL_VERSION=""
        if CLANG_FULL_VERSION=$(clang++ -dumpfullversion 2>/dev/null); then
            # -dumpfullversion worked (Clang 16+)
            :
        elif CLANG_FULL_VERSION=$(clang++ -dumpversion 2>/dev/null); then
            # -dumpversion worked (older Clang)
            :
        else
            # Both failed, skip this compiler
            log_warning "Could not determine Clang version"
            CLANG_FULL_VERSION=""
        fi
        
        if [[ -n "$CLANG_FULL_VERSION" ]]; then
            # Extract major version by taking everything before the first dot
            CLANG_MAJOR_VERSION=$(echo "$CLANG_FULL_VERSION" | cut -d. -f1)
            
            # Handle case where version might be empty or non-numeric
            if [[ "$CLANG_MAJOR_VERSION" =~ ^[0-9]+$ ]] && [[ $CLANG_MAJOR_VERSION -ge 14 ]]; then
                log_success "Clang $CLANG_FULL_VERSION (major: $CLANG_MAJOR_VERSION) found (C++20 compatible)"
                return 0
            else
                log_warning "Clang $CLANG_FULL_VERSION (major: $CLANG_MAJOR_VERSION) found, but Clang 14+ required for C++20"
            fi
        fi
    fi
    
    log_error "No suitable C++20 compiler found"
    case $OS in
        ubuntu)
            log_error "Install with: sudo apt-get install gcc-11 g++-11"
            ;;
        centos)
            log_error "Install with: sudo yum install gcc-toolset-11"
            ;;
        macos)
            log_error "Install with: xcode-select --install"
            ;;
    esac
    exit 1
}

# Setup vcpkg
setup_vcpkg() {
    log_info "Setting up vcpkg..."
    
    if [[ ! -d "$VCPKG_ROOT" ]]; then
        log_info "Cloning vcpkg to $VCPKG_ROOT..."
        git clone https://github.com/Microsoft/vcpkg.git "$VCPKG_ROOT"
    else
        log_info "vcpkg already exists at $VCPKG_ROOT"
        cd "$VCPKG_ROOT"
        git pull --quiet
        cd - > /dev/null
    fi
    
    cd "$VCPKG_ROOT"
    
    # Bootstrap vcpkg
    if [[ "$OS" == "windows" ]]; then
        ./bootstrap-vcpkg.bat
    else
        ./bootstrap-vcpkg.sh
    fi
    
    # Install dependencies
    log_info "Installing vcpkg dependencies..."
    ./vcpkg install \
        tl-expected \
        fmt \
        spdlog \
        gtest \
        benchmark \
        pegtl \
        tbb
    
    cd - > /dev/null
    log_success "vcpkg setup complete"
}

# Configure CMake
configure_cmake() {
    log_info "Configuring CMake build..."
    
    # Remove old build directory only when explicitly requested
    if [[ "$CLEAN_BUILD" == "true" && -d "$BUILD_DIR" ]]; then
        log_info "Cleaning existing build directory..."
        rm -rf "$BUILD_DIR"
    fi

    CMAKE_ARGS=(
        -B "$BUILD_DIR"
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
        -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
        -DENABLE_STATIC_ANALYSIS=ON
        -DENABLE_WERROR=ON
    )
    
    # Use Ninja if available
    if command_exists ninja; then
        CMAKE_ARGS+=(-GNinja)
    fi
    
    cmake "${CMAKE_ARGS[@]}"
    log_success "CMake configuration complete"
}

# Build project
build_project() {
    log_info "Building $PROJECT_NAME..."
    
    cmake --build "$BUILD_DIR" --config "$BUILD_TYPE" --parallel "$PARALLEL_JOBS"
    
    log_success "$PROJECT_NAME build complete"
}

# Run tests
run_tests() {
    if [[ "$SKIP_TESTS" == "true" ]]; then
        log_info "Skipping tests (SKIP_TESTS=true)"
        return 0
    fi
    
    log_info "Running tests..."
    
    if [[ -f "$BUILD_DIR/tests" ]]; then
        "./$BUILD_DIR/tests"
        log_success "All tests passed"
    else
        log_warning "Test executable not found, skipping tests"
    fi
}

# Verify installation
verify_installation() {
    log_info "Verifying installation..."
    
    # Check if CLI executable exists
    if [[ -f "$BUILD_DIR/loredb-cli" ]]; then
        log_success "CLI executable found: $BUILD_DIR/loredb-cli"
        
        # Test version command
        if "./$BUILD_DIR/loredb-cli" --version &> /dev/null; then
            log_success "CLI version check passed"
        else
            log_warning "CLI version check failed"
        fi
    else
        log_error "CLI executable not found"
        exit 1
    fi
    
    # Check if benchmarks exist
    if [[ -f "$BUILD_DIR/benchmarks" ]]; then
        log_success "Benchmarks executable found: $BUILD_DIR/benchmarks"
    fi
}

# Print usage information
print_usage() {
    cat << EOF
$PROJECT_NAME Setup Script

Usage: $0 [OPTIONS]

Options:
    -h, --help          Show this help message
    -t, --type TYPE     Build type: Debug, Release, RelWithDebInfo (default: Release)
    -j, --jobs N        Number of parallel build jobs (default: auto-detect)
    -s, --skip-tests    Skip running tests
    -v, --vcpkg PATH    Path to vcpkg installation (default: ~/vcpkg)
    --clean             Clean build directory before building

Environment Variables:
    BUILD_TYPE          Build configuration (Debug/Release/RelWithDebInfo)
    VCPKG_ROOT          Path to vcpkg installation
    SKIP_TESTS          Set to 'true' to skip tests
    CLEAN_BUILD         Set to 'true' to clean build directory before building
    PARALLEL_JOBS       Number of build jobs

Examples:
    $0                  # Basic setup with default options
    $0 -t Debug         # Debug build
    $0 -j 8             # Use 8 parallel jobs
    $0 --skip-tests     # Skip running tests
    $0 --clean          # Clean build before setup

EOF
}

# Parse command line arguments
parse_args() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                print_usage
                exit 0
                ;;
            -t|--type)
                BUILD_TYPE="$2"
                shift 2
                ;;
            -j|--jobs)
                PARALLEL_JOBS="$2"
                shift 2
                ;;
            -s|--skip-tests)
                SKIP_TESTS="true"
                shift
                ;;
            -v|--vcpkg)
                VCPKG_ROOT="$2"
                shift 2
                ;;
            --clean)
                CLEAN_BUILD="true"
                shift
                ;;
            *)
                log_error "Unknown option: $1"
                print_usage
                exit 1
                ;;
        esac
    done
}

# Main setup function
main() {
    echo "=================================="
    echo "  $PROJECT_NAME Setup Script"
    echo "=================================="
    echo ""
    
    log_info "Starting setup process..."
    log_info "Build type: $BUILD_TYPE"
    log_info "Parallel jobs: $PARALLEL_JOBS"
    log_info "vcpkg root: $VCPKG_ROOT"
    log_info "Clean build: $CLEAN_BUILD"
    echo ""
    
    # Detect OS and check requirements
    detect_os
    log_info "Detected OS: $OS"
    
    # Install dependencies
    install_system_deps
    check_compiler
    
    # Setup vcpkg and build
    setup_vcpkg
    configure_cmake
    build_project
    
    # Test and verify
    run_tests
    verify_installation
    
    echo ""
    log_success "Setup complete! 🎉"
    echo ""
    echo "Next steps:"
    echo "  1. Run the CLI: ./$BUILD_DIR/loredb-cli"
    echo "  2. Run tests: ./$BUILD_DIR/tests"
    echo "  3. Run benchmarks: ./$BUILD_DIR/benchmarks"
    echo "  4. Read the documentation: README.md"
    echo ""
    echo "Happy coding! 🚀"
}

# Parse arguments and run main
parse_args "$@"
main 
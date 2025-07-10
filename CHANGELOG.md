# Changelog

All notable changes to this project will be documented in this file.

## [Unreleased]

### Changed

- **Improved OS Detection in setup.sh**: Replaced shell-specific `$OSTYPE` variable with portable `uname -s` command for better compatibility across different shells (bash, zsh, dash)
- **Enhanced Linux Distribution Detection**: Added parsing of `/etc/os-release` file (systemd standard) to accurately identify Linux distributions
- **Extended Distribution Support**: Added support for additional RHEL-based distributions (Rocky Linux, AlmaLinux, Fedora)
- **Improved Package Manager Detection**: Enhanced CentOS/RHEL installation to support both `yum` and `dnf` package managers
- **Added FreeBSD Support**: Basic support for FreeBSD with `pkg` package manager
- **Fixed macOS Dependencies**: Removed redundant system tools (`tar`, `zip`, `unzip`, `curl`) from Homebrew installation as they're built into macOS
- **Better Error Handling**: Improved error messages and fallback mechanisms for unknown distributions
- **Robust Clang Version Detection**: Replaced unreliable regex-based version detection with robust methods using `-dumpfullversion` (Clang 16+) and `-dumpversion` fallback for cross-platform compatibility

### Technical Details

- **Portable OS Detection**: Now uses `uname -s` instead of bash-specific `$OSTYPE`
- **Primary Detection**: `/etc/os-release` parsing for accurate Linux distribution identification
- **Fallback Detection**: Command availability (`apt-get`, `yum`, `dnf`) for legacy systems
- **Multi-Shell Compatibility**: Tested with bash, zsh, and designed to work with POSIX-compliant shells
- **Package Manager Support**: Automatic detection of `dnf` vs `yum` for modern RHEL-based systems
- **Clang Version Detection**: Uses `-dumpfullversion` first (Clang 16+), falls back to `-dumpversion` (older Clang), with major version extraction and numeric validation

### Supported Platforms

- **Linux**: Ubuntu, Debian, CentOS, RHEL, Fedora, Rocky Linux, AlmaLinux
- **macOS**: All versions with Homebrew
- **FreeBSD**: Basic support with pkg
- **Windows**: Detection with guidance for WSL/MSYS2
- **Fallback**: Generic Linux and unknown OS handling

## Previous Changes

_Previous changelog entries would go here_

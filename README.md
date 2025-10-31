# Bik - Simple Backup Manager

A lightweight C++ backup manager for code projects (CLI only). basically a poor 
man's git. nothing special

## Features

- **Simple CLI**: Easy-to-use command-line interface for quick backups
- **Automatic Naming**: Auto-generates backup names with incremental numbers
- **Project-Based**: Each project maintains its own backup configuration
- **Zip Compression**: Efficient storage using zip compression

## Building

### Prerequisites

- CMake 3.15 or higher
- C++17 compatible compiler (GCC or Clang)
- ZLIB library
- libzip (preferred) or minizip for zip support

### Linux

Required libraries: CMake, a C++ compiler (gcc or Clang), zlib, and either libzip or minizip.

```bash
# Install dependencies
# Ubuntu/Debian:
sudo apt-get install cmake build-essential zlib1g-dev libzip-dev

# Or with minizip:
sudo apt-get install cmake build-essential zlib1g-dev libminizip-dev

# Fedora/CentOS:
sudo dnf install cmake gcc-c++ zlib-devel libzip-devel

# Arch Linux:
sudo pacman -S cmake gcc zlib libzip

# Build
mkdir build && cd build
cmake ..
make

# Install system-wide (optional)
sudo make install
```

## Usage

### CLI Commands

#### 1. Initialize a Project

```bash
# Basic initialization
bik project -b /path/to/backup

# With custom name
bik project -b /path/to/backup -n my-project
```

This creates a `.bik` directory in your project with configuration.

#### 2. Create a Backup

```bash
# Auto-generated name (project-backup-0, project-backup-1, etc.)
bik backup

# Custom name
bik backup -n working-version-1
```

#### 3. List and Load Backups

```bash
# Interactive selection
bik load

# Load most recent backup
bik load -last
```

#### 4. Clean Backups

```bash
# Delete all backups
bik clean

# Keep only the most recent backup
bik wipeold
```

## How It Works

1. **Project Initialization**: When you run `bik project -b <dir>`, it creates a `.bik/config.txt` file in your current directory storing the backup location.

2. **Creating Backups**: The `bik backup` command zips the entire current directory (excluding `.bik`) and stores it in the backup directory.

3. **Loading Backups**: When loading, bik extracts the zip to a temporary location, clears the current directory (except `.bik`), and copies the backup contents back.

4. **Naming**: Auto-generated names follow the pattern `<project-name>-backup-<number>`.

## Examples

```bash
# Start a new project
cd /home/user/my-project
bik project -b /home/user/backups

# Make some changes, create a backup
bik backup -n before-refactor

# Make more changes, quick backup
bik backup

# Oh no, broke something! Load the previous backup
bik load -last

# Clean up old backups, keep only the latest
bik wipeold
```

## Project Structure

```
bik/
├── CMakeLists.txt
├── README.md
├── src/
│   ├── core/
│   │   ├── BackupManager.h/cpp    # Core backup logic
│   │   ├── ProjectConfig.h/cpp    # Configuration management
│   │   └── ZipUtils.h/cpp         # Zip compression utilities
│   ├── cli/
│   │   ├── main.cpp               # CLI entry point
│   │   └── CommandHandler.h/cpp   # Command parsing and execution
```

## Configuration File

The `.bik/config.txt` file stores:

```
project_dir=/path/to/project
backup_dir=/path/to/backups
project_name=project-name
```

## Notes

- Backups are stored as standard zip files, so they can be extracted manually if needed
- The `.bik` directory is never included in backups
- Uses libzip if available, else minizip for zip operations

## License

MIT License - feel free to use and modify as needed.

## Contributing

This is a simple, focused tool. Contributions are welcome for bug fixes and small improvements that maintain simplicity.

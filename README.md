# Cygpm - Yet another package manager for Cygwin

> **It's under formulation currently.**

## Theories

### `setup.ini` format

`setup.ini` is Cygwin Setup's package configuration. Usually it's a UTF-8 text file, mainly formatted with YAML. More than 11292 available packages are here hitherto, therefore, this file can be >= 16MB.

Each mirror has its own independent `setup.ini`, stored in Cygwin's download path, respectively.

This demo introduces `setup.ini`'s common format:

```yaml
# This is a comment.
# Comments are usually leaded by a hashtag "#".

# setup.ini metadata
release: cygwin
arch: x86_64
setup-timestamp: 1570126036
setup-minimum-version: 2.844
setup-version: 2.897

# Each package's info
@ package-name
sdesc: "Package's short description"
ldesc: "Package's long description. Note that in these two sections:
You should quote the value with quotation marks, and it supports
new line within the domain of quotation mark pair."
category: Demo
requires: pkg1 pkg2 pkg3 ...
install: x86_64/release/package-name/package-name-0.1.1-1.tar.xz 1024 b33704135031a4728b9ecf7fc72d53e661974f147420c4473eafe135d72eb102528bcb4889c645453b216355db4a9e17b6b63eb7578b7e9c9341c5f27a53f4e0
source: x86_64/release/package-name/package-name-0.1.1-1-src.tar.xz 3072 b33704135031a4728b9ecf7fc72d53e661974f147420c4473eafe135d72eb102528bcb4889c645453b216355db4a9e17b6b63eb7578b7e9c9341c5f27a53f4e0
depends2: pkg1, pkg2, pkg3, ...
```

> "requires": A space-separated array for dependency packages.

## Dependencies

### Build

- Cygwin or MSYS2 environment
- GCC/G++ (MinGW is also compatible)
- GNU Flex
- GNU Make

Quickly install them by Pacman in MSYS2:

```bash
pacman -S gcc make flex
```

## Plans

### Subcommands

- `update`: Fetch a fresh copy of setup.ini
- `install`: Install package(s)
- `remove`: Remove package(s)
- `view`: View a package's information
- `search`: Search a package via keywords among package name and introduction. Will support regex.
- `mirror`: Set a primary mirror

### Features Sketch

- Will store its own `setup.ini` copy and config file in `/etc/cygpm/`.
- Invoke `tar` externally.

### Technologies

#### setup.ini manipulation

- Use **Flex** to generate lexer. It's easy and **EXTREMELY FAST**!

- My former thoughts (with C++11, but it's too slow)
  - Powered by C++11 `std::regex`
  - Use "seek" to accelerate reading
    1. Read all package names at first (prefixed with "`@`")
    2. Remember those names' seek position
    3. Store the two above in a `vector<struct>`
  - Lock the file while taking operations

#### Dependency manipulation

- Draw a dependency TREE so that we can:
  - Pick out orphan packages (neither any packages depend to, nor manually installed by user)
- Install dependencies by QUEUE: Find all dependencies recursively, then add them to a pending queue structured by a `vector`.

### Question

- How to check if a package had been installed?

## Known Traps

- Stream objects' `getline()` method will be too slow when reading big files.
- Flex++ is still under beta, which is much more slower than Flex.

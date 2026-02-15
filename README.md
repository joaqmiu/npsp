# npsp v1.20

**npsp** is a high-performance, native PSP game downloader and converter for the Nintendo Switch. It allows users to browse the **NoPayStation (NPS)** library, download titles in `.pkg` format directly to the console, and extract the internal `EBOOT.PBP` using real-time AES-128-CTR decryption.

---

## 🚀 Installation Guide

1.  **Download the tool**: Get the latest version from the [Latest Release](https://github.com/joaqmiu/npsp/releases/latest).
2.  **Extract the archive**: Unzip the downloaded `.zip` file on your computer.
3.  **Transfer to SD Card**: Copy the extracted `switch` folder and paste it into the **root of your microSD card**.
    * *Note: If prompted to merge folders, select "Yes" or "Replace".*
4.  **Launch**: Open the **Homebrew Menu** or **Sphaira** on your Switch and select **npsp** to start.

---

## Features

* **Integrated NPS Database**: Seamlessly browse the PSP library directly from the NoPayStation TSV database.
* **Advanced Filtering**: Quickly find games using search queries, region filters (JP), numeric titles, or alphabetical sorting.
* **On-the-Fly Extraction**: Automatically extracts the decrypted `EBOOT.PBP` from `.pkg` containers using a native C implementation of the AES-128-CTR algorithm.
* **Modern UI**:
    * Vertical alphabetical list for clean, organized navigation.
    * Real-time download speed tracking (updated every 2 seconds) and progress bars.
    * Hold-to-scroll logic for rapid browsing through large lists.
* **Automated Directory Management**: Automatically creates and manages the `/PPSSPP/` output folder on the microSD card.

---

## Controls

| Button | Action |
| :--- | :--- |
| **D-Pad Up/Down** | Navigate through lists (Hold to scroll) |
| **D-Pad Left/Right** | Skip 10 entries or jump pages |
| **A** | Select / Download / Confirm |
| **B** | Back to Menu / Cancel Download |

---

## Project Structure

The project is modularized for better maintainability and performance:

* **`source/main.c`**: Core application loop, UI state management, and user input handling.
* **`source/net.c`**: Network stack, cURL integration, and TSV database parsing.
* **`source/converter.c`**: Low-level decryption logic and PBP extraction.
* **`source/common.h`**: Shared definitions, structures, and utility functions.

---

## Technical Specifications

### Build Requirements
To compile **npsp**, you must have the **devkitPro** environment installed with the following libraries:
* `devkitA64`
* `libnx`
* `switch-curl`
* `switch-mbedtls`
* `switch-zlib`

### Building from Source
1. Clone the repository.
2. Ensure your `icon.png` (256x256) is in the root directory.
3. Run `make` in your terminal.
4. Copy the resulting `npsp.nro` to the `/switch/npsp/` folder on your SD card.

---

## Credits

* **Developer**: [joaqmiu](https://github.com/joaqmiu)
* **Libraries**: [libnx](https://github.com/switchbrew/libnx), [mbedTLS](https://github.com/Mbed-TLS/mbedtls), [cURL](https://curl.se/).

---

## External Resources

* [devkitPro](https://devkitpro.org/) - Toolchain for Switch homebrew development.
* [NoPayStation](https://nopaystation.com/) - Game database and TSV source.
* [PPSSPP](https://www.ppsspp.org/) - The recommended emulator to play the extracted titles.

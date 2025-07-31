# Relay PC - Android Notification Center

A desktop notification center that receives and displays notifications from your Android device in real-time.

## Features

- **Automatic Discovery**: Finds your Android server automatically using mDNS
- **Real-time Notifications**: Displays notifications from your Android device instantly
- **Modern Interface**: Clean, semi-transparent notification panel
- **System Tray Integration**: Access via system tray icon
- **Popup Notifications**: Temporary popup windows for new notifications
- **Interactive Actions**: Reply to messages and trigger notification actions

## Requirements

- Qt6 (Core, Widgets, Network, DBus)
- C++17 compatible compiler
- Linux desktop environment
- Android device with compatible notification server app

## Installation & Building

1. **Install Dependencies**:
   ```bash
   # Ubuntu/Debian
   sudo apt install qt6-base-dev qt6-tools-dev build-essential
   
   # Fedora
   sudo dnf install qt6-qtbase-devel qt6-qttools-devel gcc-c++
   
   # Arch Linux
   sudo pacman -S qt6-base qt6-tools gcc
   ```

2. **Build the Application**:
   ```bash
   git clone https://github.com/SunPodder/relay-pc
   cd relay-pc
   qmake6 relay-pc.pro
   make
   ```

3. **Run**:
   ```bash
   ./relay-pc
   ```

## Usage

### Getting Started

1. **Install the Android App**: Install the compatible notification server app on your Android device
2. **Connect to Same Network**: Ensure both your PC and Android device are on the same WiFi network
3. **Launch Relay PC**: The app will automatically discover and connect to your Android device
4. **Access Panel**: Click the system tray icon or use `Ctrl+Shift+N` to show/hide notifications

### Command Line Options

```bash
./relay-pc                                    # Normal mode with auto-discovery
./relay-pc --direct <ip-address> [port]      # Connect directly to specific server
./relay-pc --verbose                         # Enable debug logging
./relay-pc --help                           # Show help information
```

### Controls

- **System Tray**: Click to toggle notification panel
- **Hotkey**: `Ctrl+Shift+N` to show/hide panel
- **Remove**: Click × on notification cards to dismiss
- **Actions**: Click ⌄ on cards to expand available actions (Reply, Mark as Read, etc.)

## Troubleshooting

### Connection Issues

If the app can't find your Android device:

1. **Check Network**: Ensure both devices are on the same WiFi network
2. **Firewall**: Make sure your firewall allows mDNS traffic (port 5353 UDP)
3. **Direct Connection**: Try connecting directly:
   ```bash
   ./relay-pc --direct <android-device-ip> 8080
   ```
4. **Verbose Logging**: Enable debug output to see connection details:
   ```bash
   ./relay-pc --verbose
   ```

### Common Solutions

- **Restart WiFi** on both devices
- **Check Android app** is running and properly configured
- **Try direct IP connection** if mDNS discovery fails
- **Disable VPN** temporarily as it may interfere with local network discovery

## Android Server Setup

[https://github.com/SunPodder/Relay](https://github.com/SunPodder/Relay)

Check the Android app documentation for specific setup instructions.


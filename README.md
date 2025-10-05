<h1 align="center">Ymir Generic USB Bulk Streaming Tool</h1>
<p align="center">
  <b>Ymir</b> — a universal USB bulk transfer utility designed to send data to and receive data from any USB device supporting bulk endpoints.<br>
  Fully <b>generic</b> and <b>configurable</b> for any use case.
</p>

<p align="center">
  <img src="https://github.com/av2xn/Ymir/blob/main/ymir-logo.png" alt="Ymir Logo" width="512" height="512">
</p>


---

# ⚡ Features

- ✅ **Generic USB Bulk Transfer** – Works with any USB device that supports bulk transfer.  
- ✅ **Flexible Configuration** – Customize VID, PID, EP-out, EP-in, interface, timeout, and chunk size.  
- ✅ **Unix Philosophy Friendly** – Reads from `stdin` and writes to `stdout`, easily scriptable.  
- ✅ **Verbose Logging** – Detailed transfer and error reporting when needed.  
- ✅ **Binary Output Support** – Works seamlessly with binary data streams or files.  

---

# 1.1 🧰 Installation of required packages 

> ⚠️ If your system has **Git**, **GCC**, and **libusb** you dont need to install required packages, they are already installed.

### Debian 
```bash
sudo apt install git gcc libusb-1.0-0-dev
```

### Arch Linux
```bash
sudo yay -S git gcc libusb-1.0-0-dev
```

### Red Hat (RHEL)
```bash
sudo dnf install git gcc libusb1-devel
```

### openSUSE
```bash
sudo zypper install git gcc libusb-1_0-devel
```

### macOS

Be sure about `HomeBrew` is installed on your system.

```bash
xcode-select --install
brew install libusb
```
If not:

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
xcode-select --install
brew install libusb
```

# 1.2 ⚙️ Building

```bash
git clone https://github.com/av2xn/Ymir.git
cd Ymir
gcc ymir.c -o ymir -lusb-1.0
```

## 2. Basic Command Structure

```bash
echo -n "DATA_TO_SEND" | sudo ./ymir --vid 0xVVVV --pid 0xPPPP --ep-out 0xNN --ep-in 0xMM --iface N [options]
```

- `DATA_TO_SEND` → Raw bytes, ASCII text, or hex string.
- `--vid 0xVVVV` → Vendor ID of your device.
- `--pid 0xPPPP` → Product ID of your device.
- `--ep-out 0xNN` → Bulk OUT endpoint (send data).
- `--ep-in 0xMM` → Bulk IN endpoint (read data).
- `--iface N` → Interface number (usually 0).
- `[options]` → Optional flags: `--verbose`, `--timeout`, `--chunk-size`.

---

## 3. Discovering Your Device

### Linux

1. List USB devices:

```bash
lsusb
```

Example:

```
Bus 001 Device 008: ID 04e8:685d Samsung Electronics Co., Ltd Galaxy S II (Download mode)
```

- VID = 04e8  
- PID = 685d  

2. Get endpoints and interface:

```bash
sudo lsusb -v -d 04e8:685d | grep -E "bEndpointAddress|iInterface"
```

Example output:

```
bEndpointAddress     0x02  EP 2 OUT
bEndpointAddress     0x81  EP 1 IN
iInterface           0
```

- EP-out = 0x02  
- EP-in = 0x81  
- Interface = 0

### macOS

```bash
system_profiler SPUSBDataType
```

Look for Vendor ID, Product ID, and endpoints in the descriptor. IN and OUT endpoints will be labeled.

---

## 4. Sending Data

Send a string to device:

```bash
echo -n "DVIF" | sudo ./ymir --vid 0x04e8 --pid 0x685d --ep-out 0x02 --ep-in 0x81 --iface 0 --verbose
```

- `--verbose` prints the number of bytes sent and received.
- Pipe output to `hexdump` for hex view:

```bash
echo -n "DVIF" | sudo ./ymir --vid 0x04e8 --pid 0x685d --ep-out 0x02 --ep-in 0x81 --iface 0 --verbose | hexdump -C
```

---

## 5. Sending Files / Streaming

```bash
cat firmware.bin | sudo ./ymir --vid 0x1234 --pid 0xabcd --ep-out 0x02 --ep-in 0x81 --iface 0 --verbose > response.bin
```

- Handles large files in chunks automatically.
- `response.bin` will contain all responses.
- Chunk size adjustable with `--chunk-size N`.

---

## 6. Advanced Options

- `--timeout MS` → Wait time in milliseconds (default 5000).  
- `--chunk-size N` → Bytes per read/write operation (default 4096).  
- `--verbose` → Shows detailed debug info.  

**Example with custom timeout and chunk size:**

```bash
echo -n "DATA" | sudo ./ymir --vid 0x1234 --pid 0xabcd --ep-out 0x02 --ep-in 0x81 --iface 0 --timeout 10000 --chunk-size 8192 --verbose
```

---

## 7. Common Issues & Troubleshooting

1. **Device not detected**
   - Check cable and port.
   - Verify VID/PID.
   - Use `sudo` on Linux.

2. **Permission denied**
   - Add udev rule on Linux:

```bash
sudo nano /etc/udev/rules.d/99-usb.rules
```

Add:

```
SUBSYSTEM=="usb", ATTR{idVendor}=="04e8", ATTR{idProduct}=="685d", MODE="0666"
```

Reload rules:

```bash
sudo udevadm control --reload-rules
sudo udevadm trigger
```

3. **LIBUSB_ERROR_PIPE**
   - Sending to wrong endpoint.
   - Check endpoint addresses carefully.

4. **Binary output garbled**
   - Redirect to file instead of terminal.

---

## 8. Practical Examples

### Sending hex bytes

```bash
echo -ne "" | sudo ./ymir --vid 0x1234 --pid 0xabcd --ep-out 0x02 --ep-in 0x81 --iface 0 --verbose
```

### Streaming firmware

```bash
dd if=firmware.bin bs=4096 | sudo ./ymir --vid 0x1234 --pid 0xabcd --ep-out 0x02 --ep-in 0x81 --iface 0 --verbose > output.log
```

### Reading device response only

```bash
sudo ./ymir --vid 0x1234 --pid 0xabcd --ep-in 0x81 --iface 0 --timeout 5000
```

---

## 9. Best Practices

- Start with small test data.
- Always verify IN/OUT endpoints.
- Use verbose mode for debugging.
- Backup device before firmware writes.
- Adjust chunk size based on file size.

---

## 10. Notes

- Works with any USB bulk device.
- Reads multi-chunk responses automatically.
- Compatible with Linux and macOS.
- Designed for both development and forensic USB communication.

---

**🔹 Tip:** Always check your device documentation. Sending incorrect data can brick the device. Use Ymir responsibly.

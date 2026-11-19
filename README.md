# mouse-tracker by Ado210

```text
                               \
                                |
 _._     _,-'""`-._            _|_
(,-.`._,'(       |\`-/|       /___\
    `-.-' \ )-`( , o o)      |     |
          `-    \`_`"'-       \___/
```

Windows tool written in C++ that records mouse movements and exports them as a PNG on your Desktop.

## Build
```bash
g++ -O2 main.cpp -o tracker.exe
```

## Usage
- `/start` – start recording
- `/stop` – save `mouse_pattern_X.png` to Desktop
- `/exit` – quit
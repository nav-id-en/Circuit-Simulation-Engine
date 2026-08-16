# ProteusLab SDL

این پروژه یک نرم‌افزار دسکتاپ طراحی و شبیه‌سازی مدار شبیه Proteus است که از ابتدا با `C++20` و `SDL2` پیاده‌سازی شده است. در این نسخه هیچ وابستگی‌ای به Qt، Qt Creator، SDL_ttf یا SDL_image وجود ندارد. تمام پنجره، رویدادها، بوم، متن‌ها، قطعات، سیم‌ها، نمودارها و کنترل‌ها با SDL2 و رندر برداری داخلی برنامه ساخته می‌شوند.

## اجرای سریع در ویندوز

پوشه پروژه را باز کنید و PowerShell را با گزینه `Run as administrator` اجرا کنید:

```powershell
Set-Location -LiteralPath 'E:\uni\programming\Basic Programming\Project-oop\ProteusLabSDL'
powershell -ExecutionPolicy Bypass -File .\scripts\setup_windows.ps1
```

این اسکریپت فقط ابزارهای لازم برای نسخه SDL را نصب می‌کند:

- MSYS2
- GCC/G++ برای C++
- CMake و Ninja
- SDL2

پس از پایان نصب، یک CMD معمولی باز کنید:

```bat
cd /d "E:\uni\programming\Basic Programming\Project-oop\ProteusLabSDL"
scripts\run_windows.bat
```

فایل اجرایی در مسیر زیر ساخته می‌شود:

```text
build-windows\ProteusLabSDL.exe
```

اسکریپت Build پیش از اجرای برنامه، تمام تست‌ها را نیز اجرا می‌کند. اگر تستی ناموفق باشد برنامه اجرا نمی‌شود و خطا در CMD باقی می‌ماند.

اسکریپت Build کامپایلر، Ninja، SDL2 و DLLهای زمان اجرا را همگی به‌صورت
صریح از `C:\msys64\ucrt64` انتخاب می‌کند. بنابراین حتی اگر یک MinGW دیگر در
مسیرهایی مانند `C:\mingw64` نصب باشد، ابزارهای دو زنجیره با یکدیگر مخلوط
نمی‌شوند. گزینهٔ `--fresh` نیز Cache قدیمی CMake را خودکار اصلاح می‌کند و
نیازی به پاک‌کردن دستی `build-windows` نیست.

## امکانات اصلی

- صفحه آغازین با New، Open، Demo و فهرست پروژه‌های اخیر
- کتابخانه دسته‌بندی‌شده قطعات، جست‌وجو و Preview
- قرار دادن قطعه با کلیک یا Drag & Drop
- انتخاب تکی، انتخاب چندگانه و انتخاب ناحیه‌ای
- جابه‌جایی، Grid Snap، چرخش و قرینه‌سازی
- Zoom و Pan و نمایش مختصات لحظه‌ای
- سیم‌کشی خودکار ۹۰ درجه
- Junction Dot واقعی؛ تقاطع سیم بدون Dot اتصال الکتریکی ندارد
- حفظ اتصال و رسم مجدد سیم هنگام حرکت قطعات
- ویرایش تمام ویژگی‌های قطعات در پنل Properties
- Run، Pause، Stop و Step
- رنگ زنده سیم‌ها برای HIGH، LOW، Analog و Undefined
- تعامل زنده با Switch، Push Button، Keypad و Potentiometer
- Voltage Probe و اسیلوسکوپ چندکاناله
- کنترل `Time/Div` و `Volts/Div` اسیلوسکوپ
- Voltmeter و Ammeter
- DRC برای Floating Input، اتصال خروجی‌های متناقض، نبود GND و مدار خراب
- Simulation Log
- Save، Save As، Open و Recent Projects
- ذخیره اختیاری وضعیت اجرای شبیه‌سازی
- Undo و Redo مبتنی بر Snapshot
- خروجی PNG از خود بوم مدار
- بارگذاری و اعتبارسنجی Intel HEX برای MCU

## قطعات

### منابع

- GND
- DC Voltage Source
- Battery با مقاومت داخلی
- Clock Generator

### قطعات آنالوگ و تعاملی

- Resistor
- Capacitor
- Inductor
- Potentiometer
- Switch
- Push Button
- LED
- 7-Segment

### دیجیتال

- AND
- OR
- NOT
- XOR
- NAND
- D Flip-Flop لبه‌بالارونده

### پیشرفته

- ADC چندبیتی
- DAC چندبیتی
- MCU با Flash، RAM، Register، PC و Port A/B
- External Memory با Address/Data Bus
- LCD 16x2
- Keypad 4x4

### ابزار اندازه‌گیری

- Voltmeter
- Ammeter
- Voltage Probe
- Oscilloscope

## کلیدهای میانبر

| کلید | عملکرد |
|---|---|
| `Ctrl+N` | پروژه جدید |
| `Ctrl+O` | باز کردن پروژه |
| `Ctrl+S` | ذخیره |
| `Ctrl+Shift+S` | ذخیره در مسیر جدید |
| `Ctrl+E` | خروجی PNG |
| `Ctrl+Z` | Undo |
| `Ctrl+Y` | Redo |
| `Ctrl+A` | انتخاب همه |
| `W` | ابزار سیم |
| `J` | ابزار Junction |
| `P` | ابزار Probe |
| `R` | چرخش ۹۰ درجه |
| `H` / `V` | قرینه افقی / عمودی |
| `Delete` | حذف انتخاب |
| `+` / `-` | Zoom |
| دکمه وسط یا راست ماوس | Pan |
| `Esc` | لغو ابزار، بستن منو یا اسیلوسکوپ |

## فایل‌های نمونه

- `examples/mixed_signal_demo.oopproteus.json`
- `examples/mcu_led_demo.oopproteus.json`
- `firmware/port_pattern.hex`

از صفحه آغازین می‌توانید `OPEN DEMO` را بزنید. برای نمونه MCU، از File > Open مسیر نمونه دوم را وارد کنید.

## ساختار پوشه‌ها

```text
include/proteus/core          مدل مدار، پایه، سیم و Signal
include/proteus/components    تعریف تمام قطعات
include/proteus/simulation    موتور شبیه‌سازی و Firmware
include/proteus/persistence   JSON و Save/Load
include/proteus/ui            رابط SDL
src/...                       پیاده‌سازی کلاس‌ها
tests/core_tests.cpp          تست‌های خودکار
examples                      مدارهای آماده
firmware                      فایل HEX نمونه
scripts                       نصب، Build، اجرا و Package
docs                          مستندات فارسی دفاع و معماری
```

## تست

در ویندوز:

```bat
scripts\build_windows.bat
```

در Linux با SDL2 Development Package:

```bash
cmake -S . -B build -DPROTEUS_BUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

برای تست مستقل هسته بدون SDL:

```bash
scripts/test_core.sh
```

## نکته دفاع

فقط اجرای پروژه کافی نیست. فایل‌های زیر را پیش از دفاع بخوانید:

- `docs/ARCHITECTURE_FA.md`
- `docs/DEFENSE_GUIDE_FA.md`
- `docs/REQUIREMENTS_COVERAGE_FA.md`
- `docs/TEST_REPORT_FA.md`

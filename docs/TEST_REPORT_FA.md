# گزارش تست

## تست‌های خودکار هسته

فایل `tests/core_tests.cpp` شامل ۱۲ تست مستقل است:

1. طبقه‌بندی LOW، HIGH و Undefined
2. Parser فایل Intel HEX و Checksum
3. نگاشت ADC و DAC
4. اجرای دستورهای MCU و Port Output
5. D Flip-Flop لبه‌بالارونده و انتشار Undefined
6. خواندن/نوشتن External Memory
7. حل مدار آنالوگ و قانون اهم
8. تشخیص Floating و اتصال خروجی‌ها
9. تشخیص اتصال دو منبع
10. تشخیص ورودی سیم‌خورده ولی بدون Driver
11. Ownership و حذف سیم وابسته
12. Round Trip کامل Save/Load شامل Property، Wire و Flash MCU

نتیجه آخرین اجرا:

```text
12/12 tests passed.
```

هسته با گزینه‌های زیر کامپایل شده است:

```text
-std=c++20 -Wall -Wextra -Wpedantic -Werror
```

## تست رابط SDL

برنامه کامل با SDL2 کامپایل و در حالت Headless اجرا شده است:

```text
ProteusLabSDL --headless-smoke artifacts/sdl_smoke.png
```

خروجی یک تصویر PNG معتبر ۱۴۴۰ در ۹۰۰ است. در این تست موارد زیر واقعاً اجرا می‌شوند:

- ایجاد Window و Renderer از SDL
- Load فایل Demo
- ساخت تمام Layoutها
- رندر Menu، Toolbar، Library، Canvas، Properties و Log
- محاسبه Pinها و Wireهای ۹۰ درجه
- رندر قطعات و Junctionها
- خواندن Pixelهای Renderer
- تولید فایل PNG

## Sanitizer

هسته و رابط کامل SDL با AddressSanitizer و UndefinedBehaviorSanitizer
کامپایل و اجرا شدند. هر ۱۲ تست و اجرای Headless بدون خطای حافظه یا رفتار تعریف‌نشده
پاس شدند. در محیط اجرای خودکار، LeakSanitizer به‌دلیل محدودیت `ptrace` غیرفعال
بود؛ AddressSanitizer و UndefinedBehaviorSanitizer فعال بودند.

برای تکرار تست در محیطی که SDL2 Development Package و CMake نصب است:

```bash
cmake -S . -B build-san \
  -DPROTEUS_BUILD_GUI=ON \
  -DPROTEUS_BUILD_TESTS=ON \
  -DPROTEUS_ENABLE_SANITIZERS=ON
cmake --build build-san --parallel
ctest --test-dir build-san --output-on-failure
```

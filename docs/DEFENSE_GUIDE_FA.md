# راهنمای دفاع

## معرفی یک‌دقیقه‌ای

«پروژه ما یک محیط طراحی و شبیه‌سازی مدار شبیه Proteus است که با C++20 و SDL2 نوشته شده. رابط گرافیکی از صفر با SDL ساخته شده و Backend هیچ وابستگی‌ای به SDL ندارد. قطعات از یک کلاس پایه Component ارث‌بری می‌کنند. شبیه‌سازی آنالوگ با Modified Nodal Analysis و بخش دیجیتال با صف رخداد زمان‌دار انجام می‌شود. مدار، سیم‌ها، Junctionها و وضعیت اجرا در JSON ذخیره می‌شوند و Undo/Redo هم با Snapshotهای همین مدل انجام می‌شود.»

## اگر پرسیدند چرا SDL؟

SDL یک لایه سطح پایین برای پنجره، Event و Rendering می‌دهد. برخلاف Qt، Widget آماده وجود ندارد؛ بنابراین Button، Panel، Text Input، Canvas، Menu، نمودار و Hit Test همگی در خود پروژه پیاده‌سازی شده‌اند. این موضوع نشان می‌دهد رابط واقعاً SDL است.

برای اثبات، فایل‌های زیر را نشان دهید:

- `include/proteus/ui/Sdl.hpp`
- `src/ui/StudioApp.cpp`
- `src/ui/Input.cpp`
- `src/ui/Render.cpp`
- `src/ui/Geometry.cpp`

در CMake نیز تنها GUI dependency برابر `SDL2::SDL2` است.

## نمایش پیشنهادی در دفاع

1. برنامه را باز کنید و صفحه Welcome را نشان دهید.
2. پروژه Demo را باز کنید.
3. از Library یک قطعه Drag کنید و روی بوم رها کنید.
4. قطعه را حرکت دهید تا تغییر زنده مسیر Wire دیده شود.
5. با `R` بچرخانید و با `H` یا `V` Mirror کنید.
6. ابزار Wire را انتخاب کنید و دو Pin را وصل کنید.
7. دو Wire متقاطع بسازید و با Junction Tool روی تقاطع Dot ایجاد کنید.
8. یک Property مانند Resistance یا Frequency را تغییر دهید.
9. DRC را روی مدار ناقص اجرا کنید و Floating Input را در Log نشان دهید.
10. خطا را رفع کنید و Run را بزنید.
11. تغییر رنگ Wireها را نشان دهید.
12. Probe را روی دو Pin بگذارید و Scope را باز کنید.
13. Time/Div و Volts/Div را تغییر دهید.
14. پروژه را Save کنید، تغییر بدهید، Undo/Redo بزنید و دوباره Load کنید.
15. با PNG خروجی بگیرید.

## سؤال‌های رایج

### سیم ۹۰ درجه چگونه ساخته می‌شود؟

Endpointهای Wire از Pinها گرفته می‌شوند. اگر هر دو مختصات X و Y متفاوت باشند یک Elbow میانی اضافه می‌شود. هنگام حرکت قطعه Endpoint دوباره از موقعیت جدید Pin محاسبه می‌شود و مسیر همچنان Orthogonal باقی می‌ماند.

### چرا تقاطع Wire خودبه‌خود اتصال نیست؟

چون دو مسیر گرافیکی ممکن است فقط از روی هم رد شوند. اتصال الکتریکی فقط وقتی ایجاد می‌شود که یک شیء Junction شامل شناسه هر دو Wire ساخته شود. موتور در زمان ساخت Net فقط Wireهای عضو Junction را Union می‌کند.

### تفاوت Run، Pause و Stop چیست؟

- Run زمان و صف رخدادها را جلو می‌برد.
- Pause زمان را متوقف می‌کند ولی State و Queue را نگه می‌دارد.
- Stop زمان، Queue، Scope و Runtime State قطعات را Reset می‌کند.

### Propagation Delay چگونه مستقل از FPS است؟

رخداد خروجی با `dueTime` در Priority Queue قرار می‌گیرد. موتور بر اساس Simulation Time رخداد را اجرا می‌کند، نه بر اساس تعداد Frameهای SDL.

### تحلیل آنالوگ چگونه است؟

قطعات Stampهای Conductance، Current و Voltage Source تولید می‌کنند. موتور آن‌ها را داخل ماتریس MNA قرار می‌دهد و با حل دستگاه خطی ولتاژ Node و جریان Source را محاسبه می‌کند.

### Undo/Redo چگونه کار می‌کند؟

بعد از هر تغییر، مدل مدار به یک رشته JSON تبدیل و در History ذخیره می‌شود. Undo Snapshot قبلی و Redo Snapshot بعدی را Load می‌کند. این روش قطعه، سیم، Junction و Property را هم‌زمان برمی‌گرداند.

### MCU چه چیزهایی دارد؟

Flash، RAM داخلی ۲۵۶ بایتی، هشت Register، Program Counter و دو Port هشت‌بیتی دارد. فایل Intel HEX ابتدا از نظر Syntax، Byte Count و Checksum کنترل می‌شود. سپس دستورهای MOV، ADD، JMP، SETB، CLR و دستورات I/O اجرا می‌شوند.

## نکته مهم

برای دفاع فقط اسم کلاس‌ها را حفظ نکنید. این چهار مسیر را واقعاً در کد دنبال کنید:

1. Click کاربر تا `addComponent`
2. Wire تا تشکیل Net در `SimulationEngine`
3. Run تا `performTick`
4. Save تا `CircuitSerializer`

